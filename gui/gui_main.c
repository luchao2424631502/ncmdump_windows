#include "gui.h"

AppContext g_ctx = {0};

int main(int argc, char *argv[])
{
    HWND hwnd;
    MSG msg;
    WNDCLASSA wc;

    // 初始化通用控件
    InitCommonControls();

    // 初始化应用上下文
    if (!InitAppContext(&g_ctx)) {
        MessageBoxW(NULL, L"初始化失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 注册主窗口类
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "NCMDumpGUIClass";

    if (!RegisterClassA(&wc)) {
        MessageBoxW(NULL, L"注册窗口类失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 创建主窗口
    hwnd = CreateMainWindow(&g_ctx);
    if (!hwnd) {
        MessageBoxW(NULL, L"创建主窗口失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    g_ctx.hwnd_main = hwnd;

    // 显示窗口
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // 消息循环
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    // 清理资源
    CleanupAppContext(&g_ctx);

    return (int)msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 自定义大按钮外观
    if (msg == WM_DRAWITEM) {
        DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT*)lParam;
        int id = (int)dis->CtlID;
        if (id == IDC_BTN_BROWSE_FOLDER || id == IDC_BTN_CONVERT || id == IDC_BTN_CLEAR) {
            // 填充背景色
            COLORREF bg = RGB(0x2d,0x8c,0xff); // 蓝色
            if (id == IDC_BTN_CLEAR) bg = RGB(0xff,0x4d,0x4f); // 红色
            if (id == IDC_BTN_BROWSE_FOLDER) bg = RGB(0x22,0xc5,0x5e); // 绿色
            HBRUSH hBrush = CreateSolidBrush(bg);
            FillRect(dis->hDC, &dis->rcItem, hBrush);
            DeleteObject(hBrush);

            // 设置字体
            LOGFONTW lf = {0};
            lf.lfHeight = 22;
            lf.lfWeight = FW_BOLD;
            wcscpy(lf.lfFaceName, L"微软雅黑");
            HFONT hFont = CreateFontIndirectW(&lf);
            HFONT hOldFont = (HFONT)SelectObject(dis->hDC, hFont);

            // 字体颜色
            SetTextColor(dis->hDC, RGB(255,255,255));
            SetBkMode(dis->hDC, TRANSPARENT);

            // 按钮文字
            wchar_t *text = L"";
            if (id == IDC_BTN_BROWSE_FOLDER) text = L"批量添加NCM文件";
            else if (id == IDC_BTN_CONVERT) text = L"开始转换";
            else if (id == IDC_BTN_CLEAR) text = L"清除表格";
            DrawTextW(dis->hDC, text, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SelectObject(dis->hDC, hOldFont);
            DeleteObject(hFont);

            // 边框
            FrameRect(dis->hDC, &dis->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
            return TRUE;
        }
    }

    AppContext *ctx = &g_ctx;

    switch (msg) {
        case WM_CREATE: {
            // 创建工具栏
            CreateGuiLayout(hwnd, ctx);
            
            // 创建拖拽区域和表格
            CreateFileTable(hwnd, ctx);
            
            // 创建状态栏
            CreateStatusBar(hwnd, ctx);
            
            // 注册拖拽
            DragAcceptFiles(hwnd, TRUE);
            break;
        }

        case WM_DROPFILES: {
            HDROP hdrop = (HDROP)wParam;
            OnFileDrop(hwnd, hdrop, ctx);
            break;
        }

        case WM_COMMAND: {
            int cmd_id = LOWORD(wParam);
            switch (cmd_id) {
                case IDC_BTN_BROWSE_FOLDER:
                    OnBrowseFolder(hwnd, ctx);
                    break;
                case IDC_BTN_BROWSE_OUTPUT:
                    OnBrowseOutput(hwnd, ctx);
                    break;
                case IDC_BTN_CONVERT:
                    OnConvertFiles(hwnd, ctx);
                    break;
                case IDC_BTN_CLEAR:
                    OnClearTable(hwnd, ctx);
                    break;
                case IDM_FILE_EXIT:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                case IDM_HELP_ABOUT:
                    MessageBoxW(hwnd, L"NCM 转换工具 v1.0\n\n将网易云音乐 NCM 文件转换为 MP3/FLAC",
                               L"关于", MB_OK | MB_ICONINFORMATION);
                    break;
            }
            break;
        }

        case WM_SIZE: {
            // 重新调整控件大小 - 将在 gui_window.c 中具体实现
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case WM_APP: {
            // 转换完成通知
            SendMessageW(ctx->hwnd_status_bar, SB_SETTEXT, 0, (LPARAM)L"转换完成");
            MessageBoxW(hwnd, L"文件转换完成", L"提示", MB_OK | MB_ICONINFORMATION);
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    return 0;
}

int InitAppContext(AppContext *ctx)
{
    memset(ctx, 0, sizeof(AppContext));
    pthread_mutex_init(&ctx->file_table.lock, NULL);
    strcpy(ctx->output_dir, ".\\");  // 默认输出目录
    return 1;
}

void CleanupAppContext(AppContext *ctx)
{
    pthread_mutex_destroy(&ctx->file_table.lock);
    memset(ctx, 0, sizeof(AppContext));
}
