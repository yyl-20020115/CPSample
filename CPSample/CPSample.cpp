// CPSample.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "CPSample.h"
#include <stdio.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <WinSock2.h>
#define MAX_LOADSTRING 100
wchar_t* EncodingConvert(const char* s) {
    int sl = (int)strlen(s);
    int wl = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s, sl, 0, 0);
    wchar_t* w = (wchar_t*)malloc((wl + 1) * sizeof(wchar_t));
    if (w != 0) {
        int rl = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s, sl, w, wl);
        if (rl != wl) {
            //What?
        }
        w[wl] = L'\0';
    }
    return w;
}
//url: "ftp://192.168.1.103:2121/"
//file: "test.txt"
HRESULT FtpPutIntoClipboard(HWND hWnd, const char* url, const char* file_name) {
    STRRET r = { 0 };
    HRESULT hr = S_OK;
    ULONG ret = 0, f = 0;
    IShellFolder* desktop = 0;
    IDataObject* fo = 0;
    IShellFolder* ftp_root = 0;
    IContextMenu* context_menu = 0;
    IEnumIDList* pl = 0;
    SHFILEINFO sfi = { 0 };
    PITEMID_CHILD pc = 0;
    PIDLIST_ABSOLUTE pidl = 0;
    wchar_t* w_url = EncodingConvert(url);
    wchar_t* w_file_name = EncodingConvert(file_name);
    CMINVOKECOMMANDINFO cmd = { 0 };
    cmd.cbSize = sizeof(cmd);
    cmd.lpVerb = "copy";

    if (S_OK != (hr = SHGetDesktopFolder(&desktop))) goto exit_me;
    if (S_OK != (hr = SHParseDisplayName(w_url, NULL, &pidl, SFGAO_FOLDER, &ret))) goto exit_me;
    if (S_OK != (hr = desktop->BindToObject(pidl, 0, IID_IShellFolder, (void**)&ftp_root))) goto exit_me;
    if (S_OK != (hr = ftp_root->EnumObjects(hWnd, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &pl))) goto exit_me;

    while (S_OK == (hr = pl->Next(1, &pc, &f)))
    {
        memset(&r, 0, sizeof(r));
        if (S_OK != (hr = ftp_root->GetDisplayNameOf(pc, SHGDN_NORMAL| SHGDN_FORPARSING, &r))) goto exit_me;
        BOOL done = FALSE;
        if (strstr(r.cStr,"ftp://")!=0) {
            done = strstr(r.cStr, file_name) != 0;
        }
        else 
        {
            done = wcsstr(r.pOleStr, w_file_name) != 0;
        }

        if (done) 
        {
            LPCITEMIDLIST b[] = { pc };
            if (S_OK != (hr = ftp_root->GetUIObjectOf(
                hWnd, 1, b, IID_IContextMenu, NULL, (void**)&context_menu))) goto exit_me;
            if (S_OK != (hr = context_menu->InvokeCommand(&cmd))) goto exit_me;
            context_menu->Release();
        }
    }
    hr = S_OK;

exit_me:
    if (ftp_root != 0) ftp_root->Release();
    if (desktop != 0) desktop->Release();
    if (w_url != 0) free(w_url);
    if (w_file_name != 0) free(w_file_name);

    return hr;
}

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int StartSocket() {
    WSADATA wsa = { 0 };
    int r = WSAStartup(MAKEWORD(2, 2), &wsa);
    return r == 0;
}
int StopSocket() {
    int r = WSACleanup();
    return r == 0;
}
extern "C" void* start_loop(int looping);
extern "C" void exit_loop();

void WriteAllToFile(const char* path, const char* message) {
    
    if (message != 0) {
        FILE* f = 0;
        if (0 == fopen_s(&f, path, "rb")) 
        {
            //file exits
            fclose(f);
        }
        else if (0 == fopen_s(&f, path, "wb"))
        {
            fprintf(f, message);
            fclose(f);
        }
    }
}
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);


#if 1
    if (S_OK != OleInitialize(0)) return -1;

    WriteAllToFile("sample.txt", "This is a sample file for miniftp server");   
    StartSocket();
    void * handle = start_loop(0);
#endif
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CPSAMPLE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        StopSocket();
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CPSAMPLE));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

#if 1
    exit_loop();
    if (handle != INVALID_HANDLE_VALUE) {
        WaitForSingleObject(handle, INFINITE);
        CloseHandle(handle);
    }
    StopSocket();

    CoUninitialize();
#endif
    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CPSAMPLE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CPSAMPLE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
#if 1
                FtpPutIntoClipboard(hWnd,"ftp://127.0.0.1:8989/","sample.txt");
#endif
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
