#ifndef __GUI_H
#define __GUI_H

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// 最大文件数
#define MAX_FILES 1000
// 表格列数
#define TABLE_COLUMNS 3

// 转换状态枚举
typedef enum {
    STATUS_WAIT = 0,
    STATUS_CONVERTING,
    STATUS_SUCCESS,
    STATUS_FAILED
} ConversionStatus;

// 文件项结构
typedef struct {
    char filepath[MAX_PATH];        // 完整文件路径
    char filename[MAX_PATH];        // 文件名
    ConversionStatus status;        // 转换状态
    char error_msg[256];           // 错误信息
} FileItem;

// 表格数据结构
typedef struct {
    FileItem items[MAX_FILES];
    int count;
    pthread_mutex_t lock;
} FileTable;

// 全局应用上下文
typedef struct {
    HWND hwnd_main;                // 主窗口句柄
    HWND hwnd_table;               // 表格控件句柄
    HWND hwnd_status_bar;          // 状态栏句柄
    HWND hwnd_drag_area;           // 拖拽区域
    HWND hwnd_output_path;         // 输出路径显示
    
    FileTable file_table;          // 文件表
    char output_dir[MAX_PATH];     // 输出目录
    int is_converting;             // 是否正在转换
    int total_files;               // 总文件数
    int converted_count;           // 已转换数
} AppContext;

// 窗口函数声明
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TableWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 初始化函数
int InitAppContext(AppContext *ctx);
void CleanupAppContext(AppContext *ctx);

// 窗口创建函数
HWND CreateMainWindow(AppContext *ctx);
void CreateGuiLayout(HWND parent, AppContext *ctx);
HWND CreateFileTable(HWND parent, AppContext *ctx);
HWND CreateStatusBar(HWND parent, AppContext *ctx);

// 事件处理函数
void OnFileDrop(HWND hwnd, HDROP hdrop, AppContext *ctx);
void OnBrowseFolder(HWND hwnd, AppContext *ctx);
void OnBrowseOutput(HWND hwnd, AppContext *ctx);
void OnConvertFiles(HWND hwnd, AppContext *ctx);
void OnClearTable(HWND hwnd, AppContext *ctx);

// 表格操作函数
int AddFileToTable(FileTable *table, const char *filepath);
void ClearFileTable(FileTable *table);
void UpdateFileStatus(FileTable *table, int index, ConversionStatus status, const char *msg);
const wchar_t *GetStatusStringW(ConversionStatus status);
void RefreshTable(HWND hwnd_table, FileTable *table);

// 后台转换函数
void *ConversionWorkerThread(void *arg);

// 资源 ID 定义
#define IDC_BTN_BROWSE_FOLDER    101
#define IDC_BTN_BROWSE_OUTPUT    102
#define IDC_BTN_CONVERT          103
#define IDC_BTN_CLEAR            104
#define IDC_STATIC_DRAG_AREA     105
#define IDC_STATIC_OUTPUT_LABEL  106
#define IDC_STATIC_OUTPUT_PATH   107
#define IDC_LISTVIEW_FILES       108
#define IDC_STATUSBAR            109

#define IDM_FILE_EXIT            201
#define IDM_HELP_ABOUT           202

#endif // __GUI_H
