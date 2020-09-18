#include "framework.h"
#include "CMiniFtp.h"
#include "CPSample.h"
#include <stdio.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <WinSock2.h>
#include <WinInet.h>
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

void Log(const TCHAR* message) {
    FILE* f = _tfopen(_T("log.txt"), _T("a+"));
    if (f != nullptr) {
        _ftprintf(f, _T("%s\n"), message);
        fclose(f);
    }
}

int RemoveCachedItems(const wchar_t* url)
{
    int ret = 0;
    int nCount = 0;
    DWORD MAX_CACHE_ENTRY_INFO_SIZE = 4096;
    DWORD  dwEntrySize = MAX_CACHE_ENTRY_INFO_SIZE;
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntry = NULL;
    HANDLE hCacheDir = INVALID_HANDLE_VALUE;

    lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFO) new char[dwEntrySize];
    lpCacheEntry->dwStructSize = dwEntrySize;

again:

    hCacheDir = FindFirstUrlCacheEntry(NULL,
        lpCacheEntry,
        &dwEntrySize);
    if (!hCacheDir)
    {
        delete[]lpCacheEntry;
        switch (GetLastError())
        {
        case ERROR_NO_MORE_ITEMS:
            FindCloseUrlCache(hCacheDir);
            goto quit;
        case ERROR_INSUFFICIENT_BUFFER:
            lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFO)
                new char[dwEntrySize];
            lpCacheEntry->dwStructSize = dwEntrySize;
            goto again;
        }
    }
    else 
    {
        if (wcsstr(lpCacheEntry->lpszSourceUrlName, url) != 0)
        {
            if (DeleteUrlCacheEntry(lpCacheEntry->lpszSourceUrlName)) {
                ret++;
            }
        }
    }

    nCount++;
    delete (lpCacheEntry);

    do
    {
        dwEntrySize = MAX_CACHE_ENTRY_INFO_SIZE;
        lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFO) new char[dwEntrySize];
        lpCacheEntry->dwStructSize = dwEntrySize;

    retry:
        if (!FindNextUrlCacheEntry(hCacheDir,
            lpCacheEntry,
            &dwEntrySize))
        {
            delete[]lpCacheEntry;
            switch (GetLastError())
            {
            case ERROR_NO_MORE_ITEMS:
                FindCloseUrlCache(hCacheDir);
                goto quit;
            case ERROR_INSUFFICIENT_BUFFER:
                lpCacheEntry =
                    (LPINTERNET_CACHE_ENTRY_INFO)
                    new char[dwEntrySize];
                lpCacheEntry->dwStructSize = dwEntrySize;
                goto retry;
            }
        }
        else 
        {
            if (wcsstr(lpCacheEntry->lpszSourceUrlName, url) != 0) 
            {
                if (DeleteUrlCacheEntry(lpCacheEntry->lpszSourceUrlName)) {
                    ret++;
                }
            }
        }

        nCount++;
        delete[] lpCacheEntry;
    } while (TRUE);
quit:

    return ret;
}

wchar_t* EncodingConvertM2U(const char* s) {
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
char* EncodingConvertU2M(const wchar_t* ws) {
    int wsl = (int)wcslen(ws);
    BOOL ub = FALSE;
    char bf[1] = { 0 };
    int csl = WideCharToMultiByte(CP_ACP, 0, ws, wsl, 0, 0,0,&ub);
    char* cs = (char*)malloc((csl + 1) * sizeof(char));
    if (cs != 0) {
        int rl = WideCharToMultiByte(CP_ACP, 0, ws, wsl, cs, csl, 0, &ub);
        if (rl != csl) {
        }
        cs[csl] = '\0';
    }
    return cs;
}

HRESULT FtpPutIntoClipboard(HWND hWnd, const wchar_t* url, const wchar_t** file_names, int count) {

    int n = RemoveCachedItems(url);
    
    STRRET r = { 0 };
    HRESULT hr = S_OK;
    ULONG ret = 0, f = 0;
    IShellFolder* desktop = 0;
    IDataObject* fo = 0;
    IShellFolder* ftp_root = 0;
    IShellView* ftp_view = 0;
    IContextMenu* item_context_menu = 0;
    IEnumIDList* pl = 0;
    SHFILEINFO sfi = { 0 };
    PITEMID_CHILD pc = 0;
    PIDLIST_ABSOLUTE pidl = 0;
    

    CMINVOKECOMMANDINFO cmd = { 0 };
    cmd.cbSize = sizeof(cmd);
    cmd.lpVerb = "copy";
    //NOTICE: this is used for clear cache (so that each time we can reload list from ftp server)
    SFGAOF u = SFGAO_VALIDATE;
    int ppc_index = 0;
    LPCITEMIDLIST* ppcs = new LPCITEMIDLIST[count];

    if (S_OK != (hr = SHGetDesktopFolder(&desktop))) goto exit_me;
    if (S_OK != (hr = SHParseDisplayName(url, NULL, &pidl, SFGAO_FOLDER, &ret))) goto exit_me;
    if (S_OK != (hr = desktop->BindToObject(pidl, 0, IID_IShellFolder, (void**)&ftp_root))) goto exit_me;
    if (S_OK != (hr = ftp_root->GetAttributesOf(0,0,&u))) goto exit_me; 
    if (S_OK != (hr = ftp_root->CreateViewObject(hWnd, IID_IShellView, (void**)&ftp_view))) goto exit_me;
  
    if (S_OK != (hr = ftp_root->EnumObjects(hWnd, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &pl))) goto exit_me;

    while (S_OK == (hr = pl->Next(1, &pc, &f)))
    {
        memset(&r, 0, sizeof(r));
        if (S_OK != (hr = ftp_root->GetDisplayNameOf(pc, SHGDN_NORMAL | SHGDN_FORPARSING, &r))) goto exit_me;
        BOOL done = FALSE;
        for (int i = 0; i < count; i++) {
            char* fn = EncodingConvertU2M(file_names[i]);
            //for xp, they use cStr instead of pOleStr
            if (fn!=0 && strstr(r.cStr, "ftp://") != 0) {
                done = strstr(r.cStr, fn) != 0;
            }
            else
            {
                done = wcsstr(r.pOleStr, file_names[i]) != 0;
            }
            if (fn != 0) {
                free(fn);
            }
            if (done) break;
        }
        if (done)
        {
            ppcs[ppc_index++] = pc;
        }
    }
    if (ppc_index > 0) {
        if (S_OK != (hr = ftp_root->GetUIObjectOf(
            hWnd, ppc_index, ppcs, IID_IContextMenu, NULL, (void**)&item_context_menu))) goto exit_me;
        if (S_OK != (hr = item_context_menu->InvokeCommand(&cmd))) goto exit_me;
        item_context_menu->Release();

        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }
exit_me:
    if (ftp_view != 0) ftp_view->Release();
    if (ftp_root != 0) ftp_root->Release();
    if (desktop != 0) desktop->Release();
    if (ppcs != 0) delete[] ppcs;

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
    , blocksize(0)
    , buffer()
    , guid()
    , info_list()
    , handle(INVALID_HANDLE_VALUE)
    , hEvent(INVALID_HANDLE_VALUE)
    , bAsyncMode(true)
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

int CMiniFtp::SetSourcePathIntoClipboard(const wchar_t** paths, int count)
{
    return FtpPutIntoClipboard(0, L"ftp://127.0.0.1:8989/", paths,count);
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
        WaitForSingleObject(this->handle, MSWaitTimeOut);
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
bool CMiniFtp::GetAsyncMode()
{
    return this->bAsyncMode;
}

void CMiniFtp::SetAsyncMode(bool _async)
{
    this->bAsyncMode = _async;
}

int CMiniFtp::OnReceivedListInfo(const char* info_list, size_t count)
{
    this->info_list = _strdup(info_list);
    if (this->bAsyncMode) {
        SetEvent(this->hEvent);
    }
    return 0;
}

int CMiniFtp::OnReceivedFileInfo(const char* src_path, long long length)
{
    this->rfs = length;
    if (this->bAsyncMode) {
        SetEvent(this->hEvent);
    }
    return 0;
}

int CMiniFtp::OnReceivedData(const char* buffer, long long length)
{
    if (length <= this->blocksize) {
        memcpy(this->buffer, buffer, (int)length);
    }
    if (this->bAsyncMode) {
        SetEvent(this->hEvent);
    }
    return 0;
}


char* CMiniFtp::DoGetList(const char* src_path, int list)
{
    //char* decoded = this->DecodePath(src_path);
    //if (decoded == 0) return 0;
    this->SendGetListInfoQuery(src_path);

    //free(decoded);
    if (this->bAsyncMode) {
        if (WAIT_OBJECT_0 != WaitForSingleObject(this->hEvent, MSWaitTimeOut))
        {
            return 0;
        }
    }
    return this->info_list;
}

long long CMiniFtp::DoGetSize(const char* src_path)
{
    //char* decoded = this->DecodePath(src_path);
    this->SendGetFileInfoQuery(src_path);
    //if (decoded == 0) return 0;
    //free(decoded);
    if (this->bAsyncMode) {
        if (WAIT_OBJECT_0 != WaitForSingleObject(this->hEvent, MSWaitTimeOut))
        {
            return -1LL;
        }
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

    int d = 0;
    this->blocksize = blocksize;
    this->buffer = (char*)malloc((size_t)this->blocksize);
    if (this->buffer == 0) return 0;

    //char* decoded = this->DecodePath(src_path);
    //if (decoded == 0) return 0;

    this->SendGetDataInfoQuery(src_path, this->ofs, this->blocksize);
    //free(decoded);

    if (this->bAsyncMode) {

        if (WAIT_OBJECT_0 != WaitForSingleObject(this->hEvent, MSWaitTimeOut))
        {
            return -1;
        }
    }
    //Send data
    d = send(datafd, this->buffer, (int)this->blocksize, 0);
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
