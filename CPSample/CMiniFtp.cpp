#include "framework.h"
#include "CMiniFtp.h"
#include "CPSample.h"
#include <stdio.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <WinSock2.h>
extern "C" 
{
    void* start_loop(int looping);
    void exit_loop();
    typedef char* (*get_list_ptr)(const char* fn,int list);
    int SetGetDirListPtr(get_list_ptr trcp);
    int GetDirListPtr(get_list_ptr* ptrcp);

    typedef long long (*get_file_size_ptr)(const char* fn);
    int SetGetFileSizePtr(get_file_size_ptr gfsp);
    int GetGetFileSizePtr(get_file_size_ptr* pgfsp);

    typedef int (*download_ptr)(const char* fn, SOCKET datafd, long long* offset, long long blocksize);
    int SetDownloadDataPtr(download_ptr dp);
    int GetDownloadPtr(download_ptr * pdp);
}

wchar_t* EncodingConvert(const char* s) {
    int sl = (int)strlen(s);
    int wl = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s, sl, 0, 0);
    wchar_t* w = (wchar_t*)malloc((wl + 1) * sizeof(wchar_t));
    if (w != 0) {
        int rl = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s, sl, w, wl);
        if (rl != wl) {
        }
        w[wl] = L'\0';
    }
    return w;
}
//url: "ftp://192.168.1.103:2121/"
//file: "test.txt"
HRESULT FtpPutIntoClipboard(HWND hWnd, const char* url, const char** file_names, int count) {
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
    wchar_t* w_file_name = 0;
    
    wchar_t* w_url = EncodingConvert(url);
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
        if (S_OK != (hr = ftp_root->GetDisplayNameOf(pc, SHGDN_NORMAL | SHGDN_FORPARSING, &r))) goto exit_me;
        BOOL done = FALSE;
        for (int i = 0; i < count; i++) {
            w_file_name = EncodingConvert(file_names[i]);
            if (strstr(r.cStr, "ftp://") != 0) {
                done = strstr(r.cStr, file_names[i]) != 0;
            }
            else
            {
                done = wcsstr(r.pOleStr, w_file_name) != 0;
            }
            if (w_file_name != 0) free(w_file_name);
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

    return hr;
}


CMiniFtp CMiniFtp::Singleton;

int CMiniFtp::MSWaitTimeOut = 3000;

char* CMiniFtp::DoGetListCallback(const char* src_path, int list)
{
    return Singleton.DoGetList(src_path, list);
}

long long CMiniFtp::DoGetSizeCallback(const char* src_path)
{
    return Singleton.DoGetSize(src_path);
}

int CMiniFtp::DoDownloadDataCallback(const char* src_path, SOCKET datafd, long long* offset, long long blocksize)
{
    return Singleton.DoDownloadData(src_path, datafd, offset, blocksize);
}

CMiniFtp::CMiniFtp()
    : rfs(-1LL)
    , ofs(0)
    , buffer()
    , guid()
    , handle(INVALID_HANDLE_VALUE)
    , info_list()
    , hEvent(INVALID_HANDLE_VALUE)
{
    ::CoCreateGuid(&this->guid);
    this->hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    SetGetDirListPtr(DoGetListCallback);
    SetGetFileSizePtr(DoGetSizeCallback);
    SetDownloadDataPtr(DoDownloadDataCallback);
}

CMiniFtp::~CMiniFtp()
{
    if (this->hEvent != INVALID_HANDLE_VALUE) {
        SetEvent(this->hEvent);
        CloseHandle(this->hEvent);
        this->hEvent = INVALID_HANDLE_VALUE;
    }
}

int CMiniFtp::SetSourcePathIntoClipboard(const char** paths, int count)
{
    int ret = 0;
    //Call PutFtP
    ret = FtpPutIntoClipboard(0, "ftp://127.0.0.1:8989/", paths,count);
    
    return ret;
}

int CMiniFtp::StartLoop()
{
    if (this->handle == INVALID_HANDLE_VALUE) {
        //only threading
        this->handle = start_loop(false);
    }
	return 0;
}

int CMiniFtp::StopLoop()
{
    if (this->handle != INVALID_HANDLE_VALUE) {
        exit_loop();
        WaitForSingleObject(this->handle, INFINITE);
        CloseHandle(this->handle);
        this->handle = INVALID_HANDLE_VALUE;
    }

	return 0;
}

int CMiniFtp::SendGetListInfoQuery(const char* src_path)
{
    return this->m_ftpcallback == 0 ? 0 : this->m_ftpcallback->SendGetListInfoQuery(src_path);
}

int CMiniFtp::SendGetFileInfoQuery(const char* src_path)
{
    return this->m_ftpcallback == 0 ? 0 : this->m_ftpcallback->SendGetFileInfoQuery(src_path);;
}

int CMiniFtp::SendGetDataInfoQuery(const char* src_path, long long offset, long long length)
{
    return this->m_ftpcallback == 0 ? 0 :this->m_ftpcallback->SendGetDataInfoQuery(src_path
        ,offset,length);
}

void CMiniFtp::SetFtpCallback(IMiniFtpCallback* cb)
{
    this->m_ftpcallback = cb;
}

int CMiniFtp::OnReceivedListInfo(const char* info_list, size_t count)
{
    this->info_list = _strdup(info_list);
    SetEvent(this->hEvent);
    return 0;
}

int CMiniFtp::OnReceivedFileInfo(const char* src_path, long long length)
{
    this->rfs = length;
    SetEvent(this->hEvent);
    return 0;
}

int CMiniFtp::OnReceivedData(const char* buffer, long long length)
{
    if (length <= sizeof(this->buffer)) {
        memcpy(this->buffer, buffer, (int)length);
    }
    SetEvent(this->hEvent);
    return 0;
}

char* CMiniFtp::DoGetList(const char* src_path, int list)
{
    char* decoded = this->DecodePath(src_path);
    if (decoded == 0) return 0;
    this->SendGetListInfoQuery(decoded);

    free(decoded);
    if (WAIT_TIMEOUT == WaitForSingleObject(this->hEvent, MSWaitTimeOut))
    {
        return 0;
    }

    return this->info_list;
}

long long CMiniFtp::DoGetSize(const char* src_path)
{
    char* decoded = this->DecodePath(src_path);
    this->SendGetFileInfoQuery(decoded);
    if (decoded == 0) return 0;
    free(decoded);
    if (WAIT_TIMEOUT == WaitForSingleObject(this->hEvent, MSWaitTimeOut))
    {
        return -1LL;
    }
    return this->rfs;
}

int CMiniFtp::DoDownloadData(const char* src_path, SOCKET datafd, long long* offset, long long blocksize)
{
    this->rfs = -1LL;
    this->ofs = 0LL;
    if (offset != 0) {
        this->ofs = *offset;
    }
    char* decoded = this->DecodePath(src_path);
    if (decoded == 0) return 0;

    this->SendGetDataInfoQuery(decoded, this->ofs, blocksize);
    free(decoded);
    if (WAIT_TIMEOUT == WaitForSingleObject(this->hEvent, MSWaitTimeOut))
    {
        return -1;
    }
    int d = 0;
    this->buffer = (char*)malloc((size_t)blocksize);
    if (this->buffer == 0) return 0;
    if (WAIT_TIMEOUT == WaitForSingleObject(this->hEvent, MSWaitTimeOut))
    {
        return -1;
    }
    //Send data
    d = send(datafd, this->buffer, (int)blocksize, 0);
    this->ofs += d;

    free(this->buffer);
    return d;
}

char* CMiniFtp::EncodePath(const char* path)
{
    char buffer[4096] = { 0 };

    snprintf(
        buffer, 
        sizeof(buffer), 
        "%s_%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X", 
        path,
        this->guid.Data1, 
        this->guid.Data2, 
        this->guid.Data3,
        this->guid.Data4[0],
        this->guid.Data4[1],
        this->guid.Data4[2],
        this->guid.Data4[3],
        this->guid.Data4[4],
        this->guid.Data4[5],
        this->guid.Data4[6],
        this->guid.Data4[7]
    );

    return _strdup(buffer);
}

char* CMiniFtp::DecodePath(const char* path)
{
    char buffer[4096] = { 0 };
    GUID gx = { 0 };
    int n = sscanf(
        path, 
        "%s_%08X%hX%hX%hhX%hhX%hhX%hhX%hhX%hhX%hhX%hhX",
        &buffer,
        &gx.Data1,
        &gx.Data2,
        &gx.Data3,
        &gx.Data4[0],
        &gx.Data4[1],
        &gx.Data4[2], 
        &gx.Data4[3], 
        &gx.Data4[4], 
        &gx.Data4[5], 
        &gx.Data4[6],
        &gx.Data4[7]        
    );
    
    return (n == 12 &&(memcmp(&gx,&this->guid,sizeof(GUID))==0)) ? _strdup(buffer) : 0;
}
