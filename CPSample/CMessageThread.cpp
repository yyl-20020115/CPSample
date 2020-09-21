#include "CMessageThread.h"
#include <tchar.h>
HRESULT FtpPutIntoClipboardWithSource(HWND hWnd, const wchar_t* url, const wchar_t* source_id, const wchar_t** file_names, int count);

CMessageThread CMessageThread::Singleton;

DWORD __stdcall CMessageThread::MessageThread(LPVOID parameter)
{
    return Singleton.Loop(parameter);
}
LRESULT CALLBACK CMessageThread::MessageThreadWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return Singleton.WndProc(hWnd, message, wParam, lParam);
}

ATOM CMessageThread::MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = { 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = MessageThreadWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"MessageThreadClass";
    wcex.hIconSm = NULL;

    return RegisterClassExW(&wcex);
}

HWND CMessageThread::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    MyRegisterClass(hInstance);

    HWND hWnd = CreateWindowW(L"MessageThreadClass", L"", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (hWnd == 0)
    {
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return hWnd;
}


CMessageThread::CMessageThread()
    : hThread(INVALID_HANDLE_VALUE)
    , bStarted(FALSE)
    , hEvent(INVALID_HANDLE_VALUE)
    , hWnd(NULL)
{
    this->hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
}

CMessageThread::~CMessageThread()
{
    if (this->hEvent != INVALID_HANDLE_VALUE) {
        //::SetEvent(this->hEvent);
        CloseHandle(this->hEvent);
        this->hEvent = INVALID_HANDLE_VALUE;
    }
    this->Stop();
}

HWND CMessageThread::GetWindowHandle()
{
    return this->hWnd;
}

BOOL CMessageThread::Start()
{
    if (!this->bStarted) {
        DWORD TID = 0;
        this->hThread = ::CreateThread(NULL, 4096, MessageThread, this, 0, &TID);
        return this->hThread != INVALID_HANDLE_VALUE;
    }
    return this->bStarted;
}

BOOL CMessageThread::Stop()
{
    if (this->bStarted) {
        ::PostMessage(this->hWnd,WM_QUIT,0,0);
        ::WaitForSingleObject(this->hThread, INFINITE);
        ::CloseHandle(this->hThread);
        this->hThread = INVALID_HANDLE_VALUE;
    }
    return !this->bStarted;
}

void CMessageThread::Signal()
{
    if (this->hEvent != INVALID_HANDLE_VALUE) {
        ::SetEvent(this->hEvent);
    }
}



int CMessageThread::Loop(LPVOID parameter)
{
    HRESULT hr = OleInitialize(0);
    if (hr != S_OK) return -1;
    this->bStarted = TRUE;
    this->hWnd = this->InitInstance(GetModuleHandle(NULL), SW_HIDE);

    MSG msg = { 0 };
    HANDLE handles[] = {this->hEvent};
    while (true)
    {
        DWORD r = MsgWaitForMultipleObjects(1, handles, FALSE, 100, QS_ALLEVENTS);

        if (r == WAIT_OBJECT_0) {

            const wchar_t* ss[] = { L"sample1.txt",L"sample2.txt" };
            FtpPutIntoClipboardWithSource(hWnd, L"ftp://127.0.0.1/",L"winxp", ss, 2);
            ::ResetEvent(this->hEvent);
        }
        else if (r == WAIT_OBJECT_0 + 1){
            if (GetMessage(&msg, 0, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                break;
            }
        }
    }


    DestroyWindow(this->hWnd);
    this->bStarted = FALSE;
    OleUninitialize();
    return 0;
}

LRESULT CMessageThread::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_PAINT:
    {
    }
    break;
    case WM_COPY:
    {
    }
    break;
    case WM_PASTE:
    {

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
