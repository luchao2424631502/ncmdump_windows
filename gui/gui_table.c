#include "gui.h"

// Convert ANSI to Unicode for display
static wchar_t* ansi_to_wchar(const char *str)
{
    int needed = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    if (needed == 0) return NULL;
    
    wchar_t *wide = (wchar_t *)malloc(needed * sizeof(wchar_t));
    if (!wide) return NULL;
    
    MultiByteToWideChar(CP_ACP, 0, str, -1, wide, needed);
    return wide;
}

int AddFileToTable(FileTable *table, const char *filepath)
{
    pthread_mutex_lock(&table->lock);

    if (table->count >= MAX_FILES) {
        pthread_mutex_unlock(&table->lock);
        return -1;
    }

    // 检查文件是否已存在
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->items[i].filepath, filepath) == 0) {
            pthread_mutex_unlock(&table->lock);
            return -1;  // 文件已存在
        }
    }

    // 添加新文件
    FileItem *item = &table->items[table->count];
    strcpy(item->filepath, filepath);
    
    // 提取文件名
    const char *filename = strrchr(filepath, '\\');
    if (filename) {
        strcpy(item->filename, filename + 1);
    } else {
        strcpy(item->filename, filepath);
    }
    
    item->status = STATUS_WAIT;
    strcpy(item->error_msg, "");

    table->count++;
    pthread_mutex_unlock(&table->lock);

    return table->count - 1;
}

void ClearFileTable(FileTable *table)
{
    pthread_mutex_lock(&table->lock);
    table->count = 0;
    memset(table->items, 0, sizeof(table->items));
    pthread_mutex_unlock(&table->lock);
}

void UpdateFileStatus(FileTable *table, int index, ConversionStatus status, const char *msg)
{
    pthread_mutex_lock(&table->lock);

    if (index >= 0 && index < table->count) {
        table->items[index].status = status;
        if (msg) {
            strncpy(table->items[index].error_msg, msg, sizeof(table->items[index].error_msg) - 1);
        }
    }

    pthread_mutex_unlock(&table->lock);
}

const char *GetStatusString(ConversionStatus status)
{
    switch (status) {
        case STATUS_WAIT:
            return "等待中";
        case STATUS_CONVERTING:
            return "转换中";
        case STATUS_SUCCESS:
            return "成功";
        case STATUS_FAILED:
            return "失败";
        default:
            return "未知";
    }
}

// 返回宽字符状态字符串，用于 ListView 显示
const wchar_t *GetStatusStringW(ConversionStatus status)
{
    switch (status) {
        case STATUS_WAIT:
            return L"等待中";
        case STATUS_CONVERTING:
            return L"转换中";
        case STATUS_SUCCESS:
            return L"成功";
        case STATUS_FAILED:
            return L"失败";
        default:
            return L"未知";
    }
}

void RefreshTable(HWND hwnd_table, FileTable *table)
{
    if (!hwnd_table) return;

    pthread_mutex_lock(&table->lock);

    // 清空表格
    SendMessageW(hwnd_table, LVM_DELETEALLITEMS, 0, 0);

    // 重新插入所有项
    for (int i = 0; i < table->count; i++) {
        FileItem *item = &table->items[i];
        
        wchar_t *filename_w = ansi_to_wchar(item->filename);
        wchar_t *filepath_w = ansi_to_wchar(item->filepath);
        const wchar_t *status_w = GetStatusStringW(item->status);
        
        if (!filename_w) filename_w = L"";
        if (!filepath_w) filepath_w = L"";
        if (!status_w) status_w = L"";
        
        LVITEMW lvi;
        memset(&lvi, 0, sizeof(lvi));
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = filename_w;

        int idx = (int)SendMessageW(hwnd_table, LVM_INSERTITEMW, 0, (LPARAM)&lvi);

        // 设置路径列
        LVITEMW lvi_path;
        memset(&lvi_path, 0, sizeof(lvi_path));
        lvi_path.mask = LVIF_TEXT;
        lvi_path.iItem = idx;
        lvi_path.iSubItem = 1;
        lvi_path.pszText = filepath_w;
        SendMessageW(hwnd_table, LVM_SETITEMW, 0, (LPARAM)&lvi_path);

        // 设置状态列
        LVITEMW lvi_status;
        memset(&lvi_status, 0, sizeof(lvi_status));
        lvi_status.mask = LVIF_TEXT;
        lvi_status.iItem = idx;
        lvi_status.iSubItem = 2;
        lvi_status.pszText = (wchar_t *)status_w;
        SendMessageW(hwnd_table, LVM_SETITEMW, 0, (LPARAM)&lvi_status);
        
        if (filename_w && (void*)filename_w != (void*)item->filename) free(filename_w);
        if (filepath_w && (void*)filepath_w != (void*)item->filepath) free(filepath_w);
    }

    pthread_mutex_unlock(&table->lock);
}
