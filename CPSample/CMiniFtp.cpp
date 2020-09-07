#include "CMiniFtp.h"
#include "framework.h"
#include "CPSample.h"
#include <stdio.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <WinSock2.h>
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
        if (S_OK != (hr = ftp_root->GetDisplayNameOf(pc, SHGDN_NORMAL | SHGDN_FORPARSING, &r))) goto exit_me;
        BOOL done = FALSE;
        if (strstr(r.cStr, "ftp://") != 0) {
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

CMiniFtp::CMiniFtp()
    :rfs(-1LL)
    ,ofs(0)
    ,drt(0)
    ,buffer()
{
}

CMiniFtp::~CMiniFtp()
{
}

int CMiniFtp::SetSourcePathIntoClipboard(const char* path)
{
	//Call PutFtP
	return FtpPutIntoClipboard(0,"ftp://127.0.0.1:8989/",path);
}

int CMiniFtp::StartLoop()
{
	return 0;
}

int CMiniFtp::StopLoop()
{
	return 0;
}

int CMiniFtp::SendGetListInfoQuery(const char* src_path)
{
    return this->m_ftpcallback->SendGetListInfoQuery(src_path);
}

int CMiniFtp::SendGetFileInfoQuery(const char* src_path)
{
    return this->m_ftpcallback->SendGetFileInfoQuery(src_path);;
}

int CMiniFtp::SendGetDataInfoQuery(const char* src_path, long long offset, long long length)
{
    return this->m_ftpcallback->SendGetDataInfoQuery(src_path
        ,offset,length);
}

void CMiniFtp::SetFtpCallback(IMiniFtpCallback* cb)
{
    this->m_ftpcallback = cb;
}

int CMiniFtp::OnReceivedListInfo(const char* info_list, size_t count)
{
    //TODO:Return Info to the ftpserver
    return S_OK;
}

int CMiniFtp::OnReceivedFileInfo(const char* src_path, long long length)
{
    this->rfs = length;
    return S_OK;
}

int CMiniFtp::OnReceivedData(const char* buffer, long long length)
{
    if (length <= sizeof(this->buffer)) {
        memcpy(this->buffer, buffer, length);
        this->drt = 1;
    }
    return S_OK;
}

int CMiniFtp::DoGetList(const char* src_path)
{
    this->lrt = 0;
    this->SendGetListInfoQuery(src_path);
    while (!this->lrt) Sleep(10);
    //TODO:send list to mimiftpserver
    this->lrt = 0;

    return S_OK;
}

int CMiniFtp::DoDownloadFile(const char* src_path)
{
    this->rfs = -1LL;
    this->ofs = 0LL;
    this->drt = 0;
    this->SendGetFileInfoQuery(src_path);
    //ftpReply:    
    while (this->rfs == -1LL) Sleep(10);
    while (this->ofs < this->rfs) {
        this->SendGetDataInfoQuery(src_path, this->ofs, sizeof(this->buffer));
        while (!this->drt) Sleep(10);
        //TODO:send data to miniftpserver
        this->drt = 0;
    }
    return S_OK;
}

int CMiniFtp::LoopFunction()
{
    //TODO:Do the loop
    return S_OK;
}

char* CMiniFtp::EncodePath(const char* path)
{
    //TODO: add suffix
    return nullptr;
}
