#include "gui.h"


#define ROW1_Y 10
#define ROW1_HEIGHT 40
#define ROW2_Y (ROW1_Y + ROW1_HEIGHT + 5)
#define ROW2_HEIGHT 35
#define TABLE_START_Y (ROW2_Y + ROW2_HEIGHT + 5)
#define STATUSBAR_HEIGHT 20
#define BUTTON_ROW_HEIGHT 50

extern AppContext g_ctx;

// UTF-8 to Unicode conversion
static wchar_t* utf8_to_wchar(const char *utf8_str)
{
    int needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    if (needed == 0) return NULL;
    
    wchar_t *wide_str = (wchar_t *)malloc(needed * sizeof(wchar_t));
    if (!wide_str) return NULL;
    
    if (MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wide_str, needed) == 0) {
        free(wide_str);
        return NULL;
    }
    
    return wide_str;
}

HWND CreateMainWindow(AppContext *ctx)
{
    wchar_t title[] = L"NCM 文件转换工具";
    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"NCMDumpGUIClass",
        title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 900, 600,
        NULL, NULL, GetModuleHandleA(NULL), NULL
    );

    return hwnd;
}


// 新布局：分行控件
void CreateGuiLayout(HWND parent, AppContext *ctx) {
    RECT rect;
    GetClientRect(parent, &rect);
    int width = rect.right - rect.left;

    // 第一行：批量添加按钮（大且醒目）
    HWND hAddBatch = CreateWindowW(
        L"BUTTON", L"批量添加NCM文件",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
        10, ROW1_Y, width - 20, ROW1_HEIGHT,
        parent, (HMENU)IDC_BTN_BROWSE_FOLDER,
        GetModuleHandleA(NULL), NULL
    );

    // 第二行：输出目录选择（可选）
    CreateWindowW(
        L"STATIC", L"输出目录(可选):",
        WS_CHILD | WS_VISIBLE,
        10, ROW2_Y + 7, 110, 20,
        parent, (HMENU)IDC_STATIC_OUTPUT_LABEL,
        GetModuleHandleA(NULL), NULL
    );
    ctx->hwnd_output_path = CreateWindowW(
        L"STATIC", L"未选择",
        WS_CHILD | WS_VISIBLE | WS_BORDER | SS_LEFT,
        125, ROW2_Y + 5, width - 250, 25,
        parent, (HMENU)IDC_STATIC_OUTPUT_PATH,
        GetModuleHandleA(NULL), NULL
    );
    CreateWindowW(
        L"BUTTON", L"浏览",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        width - 110, ROW2_Y + 5, 90, 25,
        parent, (HMENU)IDC_BTN_BROWSE_OUTPUT,
        GetModuleHandleA(NULL), NULL
    );

    // 底部：两个大按钮（转换、清除）
    int btnW = (width - 30) / 2;
    HWND hConvert = CreateWindowW(
        L"BUTTON", L"开始转换",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
        10, rect.bottom - BUTTON_ROW_HEIGHT - STATUSBAR_HEIGHT - 5, btnW, BUTTON_ROW_HEIGHT,
        parent, (HMENU)IDC_BTN_CONVERT,
        GetModuleHandleA(NULL), NULL
    );
    HWND hClear = CreateWindowW(
        L"BUTTON", L"清除表格",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
        20 + btnW, rect.bottom - BUTTON_ROW_HEIGHT - STATUSBAR_HEIGHT - 5, btnW, BUTTON_ROW_HEIGHT,
        parent, (HMENU)IDC_BTN_CLEAR,
        GetModuleHandleA(NULL), NULL
    );
    // 可在 WM_DRAWITEM 里自定义按钮颜色
}


HWND CreateFileTable(HWND parent, AppContext *ctx)
{
    RECT rect;
    GetClientRect(parent, &rect);
    int table_top = TABLE_START_Y;
    int table_height = rect.bottom - table_top - BUTTON_ROW_HEIGHT - STATUSBAR_HEIGHT - 15;
    HWND hwnd_table = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWA,
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_VSCROLL | WS_HSCROLL,
        10, table_top, rect.right - 20, table_height,
        parent, (HMENU)IDC_LISTVIEW_FILES,
        GetModuleHandleA(NULL), NULL
    );

    ListView_SetExtendedListViewStyle(hwnd_table, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

    LVCOLUMNW lvc;
    memset(&lvc, 0, sizeof(lvc));
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 400;
    lvc.pszText = L"文件名";
    SendMessageW(hwnd_table, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);
    lvc.cx = 350;
    lvc.pszText = L"路径";
    SendMessageW(hwnd_table, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);
    lvc.cx = 100;
    lvc.pszText = L"状态";
    SendMessageW(hwnd_table, LVM_INSERTCOLUMNW, 2, (LPARAM)&lvc);

    ctx->hwnd_table = hwnd_table;
    return hwnd_table;
}

HWND CreateStatusBar(HWND parent, AppContext *ctx)
{
    int status_parts[1];
    
    HWND hwnd_status = CreateWindowExA(
        0,
        "msctls_statusbar32",
        "",
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        parent, (HMENU)IDC_STATUSBAR,
        GetModuleHandleA(NULL), NULL
    );

    // 设置状态栏分区
    status_parts[0] = -1;
    SendMessageA(hwnd_status, SB_SETPARTS, 1, (LPARAM)status_parts);
    SendMessageA(hwnd_status, SB_SETTEXT, 0, (LPARAM)"Ready");

    ctx->hwnd_status_bar = hwnd_status;
    return hwnd_status;
}
