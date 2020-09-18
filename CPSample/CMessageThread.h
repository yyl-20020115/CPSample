#pragma once
#include <Windows.h>
class CMessageThread
{
public:
    static const int LOOP_TIMEOUT = 10;

public:
    static CMessageThread Singleton;

protected:
    static ATOM MyRegisterClass(HINSTANCE hInstance);
    static LRESULT CALLBACK MessageThreadWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI MessageThread(LPVOID parameter);

protected:

    CMessageThread();
    ~CMessageThread();

public:
    
    HWND GetWindowHandle();

    BOOL Start();

    BOOL Stop();

public:

    void Signal();

protected:

    HWND InitInstance(HINSTANCE hInstance, int nCmdShow);

    virtual int Loop(LPVOID parameter);
    virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


protected:
    BOOL bStarted;
    HANDLE hThread;
    HANDLE hEvent;
    HWND hWnd;
};

