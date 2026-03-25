#include "gui.h"
#include "dump.h"
#include <shlobj.h>

extern AppContext g_ctx;

// 浏览文件夹对话框
BOOL BrowseForFolder(HWND hwnd, char *buffer, size_t size)
{
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    memset(buffer, 0, size);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "All Files\0*.*\0NCM Files\0*.ncm\0\0";
    ofn.nFilterIndex = 2;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = size;
    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (GetOpenFileNameA(&ofn)) {
        return TRUE;
    }
    return FALSE;
}

// 浏览目录对话框
BOOL BrowseForDirectory(HWND hwnd, char *buffer, size_t size)
{
    BROWSEINFOA bi;
    PIDLIST_ABSOLUTE pidl;

    memset(&bi, 0, sizeof(bi));
    bi.hwndOwner = hwnd;
    bi.pszDisplayName = buffer;
    bi.lpszTitle = "Select Folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    pidl = SHBrowseForFolderA(&bi);
    if (pidl != 0) {
        BOOL result = SHGetPathFromIDListA(pidl, buffer);
        CoTaskMemFree(pidl);
        return result;
    }
    return FALSE;
}

// 获取文件夹中的所有 .ncm 文件
void GetNCMFilesFromFolder(const char *folder, FileTable *table)
{
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    char search_path[MAX_PATH];
    char full_path[MAX_PATH];

    // 构建搜索路径
    snprintf(search_path, sizeof(search_path), "%s\\*.ncm", folder);

    find_handle = FindFirstFileA(search_path, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        // 跳过目录
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            snprintf(full_path, sizeof(full_path), "%s\\%s", folder, find_data.cFileName);
            AddFileToTable(table, full_path);
        }
    } while (FindNextFileA(find_handle, &find_data));

    FindClose(find_handle);
}

void OnFileDrop(HWND hwnd, HDROP hdrop, AppContext *ctx)
{
    char path[MAX_PATH];
    UINT count = DragQueryFileA(hdrop, 0xFFFFFFFF, NULL, 0);

    for (UINT i = 0; i < count; i++) {
        DragQueryFileA(hdrop, i, path, sizeof(path));

        DWORD attrib = GetFileAttributesA(path);
        if (attrib != INVALID_FILE_ATTRIBUTES && attrib & FILE_ATTRIBUTE_DIRECTORY) {
            GetNCMFilesFromFolder(path, &ctx->file_table);
        } else if (strstr(path, ".ncm") != NULL || strstr(path, ".NCM") != NULL) {
            AddFileToTable(&ctx->file_table, path);
        }
    }

    DragFinish(hdrop);
    RefreshTable(ctx->hwnd_table, &ctx->file_table);
}

void OnBrowseFolder(HWND hwnd, AppContext *ctx)
{
    char folder[MAX_PATH];

    if (BrowseForDirectory(hwnd, folder, sizeof(folder))) {
        GetNCMFilesFromFolder(folder, &ctx->file_table);
        RefreshTable(ctx->hwnd_table, &ctx->file_table);
    }
}

void OnBrowseOutput(HWND hwnd, AppContext *ctx)
{
    char folder[MAX_PATH];

    if (BrowseForDirectory(hwnd, folder, sizeof(folder))) {
        strcpy(ctx->output_dir, folder);
        SetWindowTextA(ctx->hwnd_output_path, folder);
    }
}

void OnClearTable(HWND hwnd, AppContext *ctx)
{
    if (ctx->is_converting) {
        MessageBoxW(hwnd, L"转换进行中，无法清除表格", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    ClearFileTable(&ctx->file_table);
    RefreshTable(ctx->hwnd_table, &ctx->file_table);
}

// 转换线程包装结构
typedef struct {
    AppContext *ctx;
    HWND hwnd_main;
} ConversionThreadArg;

void *ConversionWorkerThread(void *arg)
{
    ConversionThreadArg *thread_arg = (ConversionThreadArg *)arg;
    AppContext *ctx = thread_arg->ctx;
    HWND hwnd_main = thread_arg->hwnd_main;

    pthread_mutex_lock(&ctx->file_table.lock);
    int total = ctx->file_table.count;
    pthread_mutex_unlock(&ctx->file_table.lock);

    for (int i = 0; i < total; i++) {
        pthread_mutex_lock(&ctx->file_table.lock);
        
        if (ctx->file_table.items[i].status != STATUS_WAIT) {
            pthread_mutex_unlock(&ctx->file_table.lock);
            continue;
        }

        ctx->file_table.items[i].status = STATUS_CONVERTING;
        char filepath[MAX_PATH];
        strcpy(filepath, ctx->file_table.items[i].filepath);
        
        pthread_mutex_unlock(&ctx->file_table.lock);

        // 确定输出目录：优先使用用户设置的输出目录，否则使用文件所在目录
        char output_dir[MAX_PATH];
        if (strlen(ctx->output_dir) > 0 && strcmp(ctx->output_dir, ".\\" ) != 0) {
            strcpy(output_dir, ctx->output_dir);
        } else {
            // 使用文件所在目录
            strcpy(output_dir, filepath);
            char *last_backslash = strrchr(output_dir, '\\');
            if (last_backslash) {
                *last_backslash = '\0';
            } else {
                strcpy(output_dir, ".");
            }
        }

        // 获取文件名（不带路径）
        char *filename = strrchr(filepath, '\\');
        if (!filename) filename = filepath;
        else filename++;

        // 构建输出文件路径
        char output_file[MAX_PATH];
        snprintf(output_file, sizeof(output_file), "%s\\%s.mp3", output_dir, filename);
        
        // 移除 .ncm 扩展名
        int len = strlen(output_file);
        if (len > 4 && strcasecmp(output_file + len - 8, ".ncm.mp3") == 0) {
            strcpy(output_file + len - 8, ".mp3");
        }

        // 调用转换函数
        int result = work_convert_windows_with_output(filepath, output_file, 0);
        
        if (result == 0) {
            UpdateFileStatus(&ctx->file_table, i, STATUS_SUCCESS, "");
        } else {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "转换失败 (错误码: %d)", result);
            UpdateFileStatus(&ctx->file_table, i, STATUS_FAILED, error_msg);
        }

        RefreshTable(ctx->hwnd_table, &ctx->file_table);
        Sleep(500);  // 稍微延迟以显示转换过程
    }

    ctx->is_converting = 0;
    PostMessageW(hwnd_main, WM_APP, 0, 0);  // 通知转换完成
    free(thread_arg);
    return NULL;
}

void OnConvertFiles(HWND hwnd, AppContext *ctx)
{
    pthread_mutex_lock(&ctx->file_table.lock);
    int count = ctx->file_table.count;
    pthread_mutex_unlock(&ctx->file_table.lock);

    if (count == 0) {
        MessageBoxW(hwnd, L"请先添加要转换的文件", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    if (ctx->is_converting) {
        MessageBoxW(hwnd, L"正在转换中，请稍候", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    // 如果未选择输出目录，将使用文件所在目录（在转换线程中）

    ctx->is_converting = 1;

    ConversionThreadArg *arg = (ConversionThreadArg *)malloc(sizeof(ConversionThreadArg));
    arg->ctx = ctx;
    arg->hwnd_main = hwnd;

    pthread_t tid;
    pthread_create(&tid, NULL, ConversionWorkerThread, arg);
    pthread_detach(tid);

    SendMessageW(ctx->hwnd_status_bar, SB_SETTEXT, 0, (LPARAM)L"转换中...");
}
