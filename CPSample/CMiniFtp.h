#pragma once
#include <WinSock2.h>

class IMiniFtpCallback {
public:
	virtual int SendGetListInfoQuery(const char* src_path) = 0;
	virtual int SendGetFileInfoQuery(const char* src_path) = 0;
	virtual int SendGetDataInfoQuery(const char* src_path,
		long long offset, long long length) = 0;
	//Format of List items 
	//----------    1        0        0   836096  2020-09-02_07:45:58 CPSample.exe
	//----------    1        0        0      706  2020-09-02_20:35:13 log.txt
	//----------    1        0        0       40  2020-09-02_06:07:04 sample.txt
};

class CMiniFtp
{
public:
	static CMiniFtp Singleton;
public:
	static char* DoGetListCallback(const char* src_path, int list);
	static long long DoGetSizeCallback(const char* src_path);
	static int DoDownloadDataCallback(const char* src_path, SOCKET datafd, long long* offset, long long blocksize);
	static int MSWaitTimeOut;
private:
	CMiniFtp();
	~CMiniFtp();

public:
	int SetSourcePathIntoClipboard(const char** src_paths, int count);

public:
	int StartLoop();
	int StopLoop();


public:
	virtual int SendGetListInfoQuery(const char* src_path);
	virtual int SendGetFileInfoQuery(const char* src_path);
	virtual int SendGetDataInfoQuery(const char* src_path, 
		long long offset, long long length);
public:
	void SetFtpCallback(IMiniFtpCallback* cb);
public:
	int OnReceivedListInfo(const char* info_list, size_t count);
	int OnReceivedFileInfo(const char* src_path, long long length);
	int OnReceivedData(const char* buffer, long long length);
public:

	bool GetAsyncMode();
	void SetAsyncMode(bool asyncMode);

protected:
	//caller will free result list
	char* DoGetList(const char* src_path, int list);
	long long DoGetSize(const char* src_path);
	int DoDownloadData(const char* src_path, SOCKET datafd, long long* offset, long long blocksize);

protected:
	IMiniFtpCallback* m_ftpcallback;
	char* EncodePath(const char* path);
	char* DecodePath(const char* path);
	char* info_list;
	void* handle;

	long long rfs;
	long long ofs;
	GUID guid;
	char* buffer;
	HANDLE hEvent;
	bool bAsyncMode;
};

