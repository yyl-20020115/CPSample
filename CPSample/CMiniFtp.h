#pragma once

class IMiniFtpCallback {
public:
	virtual int SendGetListInfoQuery(const char* src_path) = 0;
	virtual int SendGetFileInfoQuery(const char* src_path) = 0;
	virtual int SendGetDataInfoQuery(const char* src_path,
		long long offset, long long length) = 0;

};
class CMiniFtp
{
public:
	CMiniFtp();
	~CMiniFtp();

public:
	int SetSourcePathIntoClipboard(const char* src_path);

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
protected:
	int DoGetList(const char* src_path);
	int DoDownloadFile(const char* src_path);

	int LoopFunction();

protected:
	IMiniFtpCallback* m_ftpcallback;
	char* EncodePath(const char* path);

	long long rfs;
	long long ofs;
	int drt;
	int lrt;
	char buffer[4096];

};

