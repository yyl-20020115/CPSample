// CPSample.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "CPSample.h"
#include <stdio.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <WinSock2.h>
#define MAX_LOADSTRING 100

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

BOOL SetFileToClipboard(LPCSTR lpFileToCopy)
{
    DWORD dwLength = lstrlen(lpFileToCopy) + 1;
    DWORD dwBufferSize = dwLength + 1 + sizeof(DROPFILES);

    char* lpBufferAll = new char[dwBufferSize];

    if (lpBufferAll != NULL)
    {
        memset(lpBufferAll, 0, dwBufferSize);

        DROPFILES* pSlxDropFiles = (DROPFILES*)lpBufferAll;

        pSlxDropFiles->fNC = FALSE;
        pSlxDropFiles->pt.x = 0;
        pSlxDropFiles->pt.y = 0;
        pSlxDropFiles->fWide = FALSE;
        pSlxDropFiles->pFiles = sizeof(DROPFILES);

        LPSTR lpFile = lpBufferAll + sizeof(DROPFILES);

        lstrcpy(lpFile, lpFileToCopy);

        lpFile[dwLength] = '\0';

        HGLOBAL hGblFiles = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, dwBufferSize);

        if (hGblFiles != NULL)
        {
            LPSTR lpGblData = (char*)GlobalLock(hGblFiles);

            if (lpGblData != NULL)
            {
                memcpy(lpGblData, lpBufferAll, dwBufferSize);
                GlobalUnlock(hGblFiles);
            }
        }

        if (OpenClipboard(NULL))
        {
            EmptyClipboard();

            SetClipboardData(CF_HDROP, hGblFiles);

            CloseClipboard();
        }

        delete[]lpBufferAll;

        return TRUE;
    }

    return FALSE;
}
void WriteAllToFile(const char* path, const char* message) {
    
    if (message != 0) {
        FILE* f = 0;
        if (0 == fopen_s(&f, path, "w+b"))
        {
            size_t l = strlen(message);
            size_t w = fwrite(message, sizeof(char), strlen(message), f);
            if (l == w) {
                //OK
            }
            fclose(f);
        }
    }
}
void GetClipboardFormats() {

    OpenClipboard(0);
    const char* names[] = {
        CFSTR_SHELLIDLIST,                 // CF_IDLIST
        CFSTR_SHELLIDLISTOFFSET,                // CF_OBJECTPOSITIONS
        CFSTR_NETRESOURCES,                        // CF_NETRESOURCE
        CFSTR_FILEDESCRIPTORA,                 // CF_FILEGROUPDESCRIPTORA
        CFSTR_FILEDESCRIPTORW,                // CF_FILEGROUPDESCRIPTORW
        CFSTR_FILECONTENTS,                        // CF_FILECONTENTS
        CFSTR_FILENAMEA,                            // CF_FILENAMEA
        CFSTR_FILENAMEW,                           // CF_FILENAMEW
        CFSTR_PRINTERGROUP,                 // CF_PRINTERS
        CFSTR_FILENAMEMAPA,                         // CF_FILENAMEMAPA
        CFSTR_FILENAMEMAPW,                        // CF_FILENAMEMAPW
        CFSTR_SHELLURL,
        CFSTR_INETURLA,
        CFSTR_INETURLW,
        CFSTR_PREFERREDDROPEFFECT,
        CFSTR_PERFORMEDDROPEFFECT,
        CFSTR_PASTESUCCEEDED,
        CFSTR_INDRAGLOOP,
        CFSTR_MOUNTEDVOLUME,
        CFSTR_PERSISTEDDATAOBJECT,
        CFSTR_TARGETCLSID,                         // HGLOBAL with a CLSID of the drop target
        CFSTR_LOGICALPERFORMEDDROPEFFECT,
        CFSTR_AUTOPLAY_SHELLIDLISTS,// (HGLOBAL with LPIDA)
        CFSTR_UNTRUSTEDDRAGDROP,//  DWORD
        CFSTR_FILE_ATTRIBUTES_ARRAY,// (FILE_ATTRIBUTES_ARRAY format on HGLOBAL)
        CFSTR_INVOKECOMMAND_DROPPARAM,// (HGLOBAL with LPWSTR)
        CFSTR_SHELLDROPHANDLER,// (HGLOBAL with CLSID of drop handler)
        CFSTR_DROPDESCRIPTION,// (HGLOBAL with DROPDESCRIPTION)
    };
    UINT rcs[] = {
        RegisterClipboardFormat(CFSTR_SHELLIDLIST),                 // CF_IDLIST
        RegisterClipboardFormat(CFSTR_SHELLIDLISTOFFSET),                // CF_OBJECTPOSITIONS
        RegisterClipboardFormat(CFSTR_NETRESOURCES),                        // CF_NETRESOURCE
        RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA),                 // CF_FILEGROUPDESCRIPTORA
        RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW),                // CF_FILEGROUPDESCRIPTORW
        RegisterClipboardFormat(CFSTR_FILECONTENTS),                        // CF_FILECONTENTS
        RegisterClipboardFormat(CFSTR_FILENAMEA),                            // CF_FILENAMEA
        RegisterClipboardFormat(CFSTR_FILENAMEW),                           // CF_FILENAMEW
        RegisterClipboardFormat(CFSTR_PRINTERGROUP),                 // CF_PRINTERS
        RegisterClipboardFormat(CFSTR_FILENAMEMAPA),                         // CF_FILENAMEMAPA
        RegisterClipboardFormat(CFSTR_FILENAMEMAPW),                        // CF_FILENAMEMAPW
        RegisterClipboardFormat(CFSTR_SHELLURL),
        RegisterClipboardFormat(CFSTR_INETURLA),
        RegisterClipboardFormat(CFSTR_INETURLW),
        RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT),
        RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT),
        RegisterClipboardFormat(CFSTR_PASTESUCCEEDED),
        RegisterClipboardFormat(CFSTR_INDRAGLOOP),
        RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME),
        RegisterClipboardFormat(CFSTR_PERSISTEDDATAOBJECT),
        RegisterClipboardFormat(CFSTR_TARGETCLSID),                         // HGLOBAL with a CLSID of the drop target
        RegisterClipboardFormat(CFSTR_LOGICALPERFORMEDDROPEFFECT),
        RegisterClipboardFormat(CFSTR_AUTOPLAY_SHELLIDLISTS),// (HGLOBAL with LPIDA)
        RegisterClipboardFormat(CFSTR_UNTRUSTEDDRAGDROP),//  DWORD
        RegisterClipboardFormat(CFSTR_FILE_ATTRIBUTES_ARRAY),// (FILE_ATTRIBUTES_ARRAY format on HGLOBAL)
        RegisterClipboardFormat(CFSTR_INVOKECOMMAND_DROPPARAM),// (HGLOBAL with LPWSTR)
        RegisterClipboardFormat(CFSTR_SHELLDROPHANDLER),// (HGLOBAL with CLSID of drop handler)
        RegisterClipboardFormat(CFSTR_DROPDESCRIPTION),// (HGLOBAL with DROPDESCRIPTION)

    };

    //CFSTR_PERSISTEDDATAOBJECT

    char fn[MAX_PATH] = { 0 };
    FILE* f = 0;
    if (0 == fopen_s(&f, "result.txt", "w+")) {
        UINT clipboard_format = EnumClipboardFormats(0); //C009: DataObject
        fprintf(f, "Current Format = %08X \n", clipboard_format);
        HGLOBAL global_memory = GetClipboardData(clipboard_format);

        LPCSTR clipboard_data = (LPCSTR)GlobalLock(global_memory);
        if (clipboard_data != NULL)
        {
            fprintf(f,"Clipboard Data Address = 0x%x\n", global_memory);

            DWORD data_size = GlobalSize(global_memory);
            fprintf(f,"Data Size = %d\n", data_size);

            fprintf(f,"Data: ");
            for (DWORD i = 0; i < data_size; i++)
            {
                if (i % 8 == 0) putchar(' ');
                if (i % 16 == 0) putchar('\n');
                fprintf(f,"%02x ", (UCHAR)clipboard_data[i]);
            }

            fprintf(f,"\n");
            for (DWORD i = 0; i < data_size; i++)
            {
                fprintf(f,"%c ", (UCHAR)clipboard_data[i]);
            }

            
            GlobalUnlock(global_memory);
        }




        for (int i = 0; i < sizeof(rcs) / sizeof(UINT); i++) {
            fprintf(f, "index= %d, name = %s, value = %08X\n", i, names[i], rcs[i]);
        }
        fclose(f);
    }
    CloseClipboard();
    
    //if (::OpenClipboard(NULL) && ::IsClipboardFormatAvailable(CF_HDROP))
    //{
    //    HDROP hDrop = (HDROP)::GetClipboardData(CF_HDROP);
    //    if (hDrop != NULL)
    //    {
    //        TCHAR filename[MAX_PATH];
    //        int fileCount = ::DragQueryFile(hDrop, 0xFFFFFFFF, filename, MAX_PATH);
    //        for (int i = 0; i < fileCount; ++i)
    //        {
    //            char fn[MAX_PATH] = { 0 };
    //            _snprintf(fn, sizeof(fn), "%d.txt", i);
    //            WriteAllToFile(fn, filename);
    //        }
    //    }
    //}
}
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
#if 1
    GetClipboardFormats();


    WriteAllToFile("sample.txt", "This is a sample file for miniftp server");
    //SetFileToClipboard("ftp://127.0.0.1:8989/sample.txt");

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
                SetFileToClipboard("ftp://127.0.0.1:8989/sample.txt");
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
