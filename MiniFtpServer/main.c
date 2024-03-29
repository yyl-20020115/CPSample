#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "common.h"
#include "sysutil.h"
#include "session.h"
#include "configure.h"
#include "parse_conf.h"
#include "ftp_nobody.h"
#include "priv_sock.h"
#include "ftp_codes.h"
#include "session_manager.h"

void log(const char* path, const char* message) {

    if (message != 0) {
        FILE* f = 0;
        if ((f = fopen(path, "a"))!=0)
        {
            fprintf(f, "%s\n", message);
            fclose(f);
        }
    }
}


void print_conf()
{
    printf("tunable_pasv_enable=%d\n", tunable_pasv_enable);
    printf("tunable_port_enable=%d\n", tunable_port_enable);

    printf("tunable_listen_port=%u\n", tunable_listen_port);
    printf("tunable_max_clients=%u\n", tunable_max_clients);
    printf("tunable_max_per_ip=%u\n", tunable_max_per_ip);
    printf("tunable_accept_timeout=%u\n", tunable_accept_timeout);
    printf("tunable_connect_timeout=%u\n", tunable_connect_timeout);
    printf("tunable_idle_session_timeout=%u\n", tunable_idle_session_timeout);
    printf("tunable_data_connection_timeout=%u\n", tunable_data_connection_timeout);
    printf("tunable_local_umask=0%o\n", tunable_local_umask);
    printf("tunable_upload_max_rate=%u\n", tunable_upload_max_rate);
    printf("tunable_download_max_rate=%u\n", tunable_download_max_rate);

    if (tunable_listen_address == NULL)
        printf("tunable_listen_address=NULL\n");
    else
        printf("tunable_listen_address=%s\n", tunable_listen_address);
}

#define H1 "There are too many connected users, please try later."
#define H2 "There are too many connections from your internet address."

void limit_num_clients(Session_t* session)
{
    if (tunable_max_clients > 0 && session->curr_clients > tunable_max_clients)
    {
        //421 There are too many connected users, please try later.
        ftp_reply(session, FTP_TOO_MANY_USERS, H1);
        exit_with_error(H1);
        return;
    }

    if (tunable_max_per_ip > 0 && session->curr_ip_clients > tunable_max_per_ip)
    {
        //421 There are too many connections from your internet address.
        ftp_reply(session, FTP_IP_LIMIT, H2);
        exit_with_error(H2);
        return;
    }
}

int s_timeout() {
    return errno == ETIMEDOUT;
}
int s_close(SOCKET* s) {
    int r = 0;
    if (*s != -1) {
#ifndef _WIN32
        r = close(*s);
#else
        r = closesocket(*s);
#endif
        * s = -1;
    }
    return r;
}
static int quit = 0;
static int code = 0;
int should_exit()
{
    return quit;
}
int exit_with_code(int c) 
{
    return (code = c);
}
int exit_with_error(const char* format, ...)
{
    char buffer[1024] = { 0 };
    va_list arg;
    int done;

    va_start(arg, format);
    done = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);

    //fprintf(stderr, buffer);
    char output[2048] = { 0 };
    _snprintf(output, sizeof(output), "ERROR  :%s\n", buffer);

    log("log.txt", output);

    return exit_with_code(EXIT_FAILURE);
}
int exit_with_message(const char* format, ...)
{
    char buffer[1024] = { 0 };
    va_list arg;
    int done = 0;
    va_start(arg, format);
    done = vsnprintf(buffer,sizeof(buffer),format, arg);
    va_end(arg);

    char output[2048] = { 0 };
    _snprintf(output, sizeof(output), "SUCCESS:%s\n", buffer);

    log("log.txt", output);

    return exit_with_code(EXIT_SUCCESS);
}
int start_private(Session_t* l_sess, void** ph);

#ifndef _WIN32
static int session_thread(void* lp)
#else
static DWORD WINAPI session_thread(void* lp)
#endif
{
    int r = 0;
    void* h = 0;
    tid_t t = 0;

    Session_t* session = (Session_t*)lp;

    limit_num_clients(session);
    priv_sock_init(session);
    if ((t = start_private(session, &h)) < 0) 
    {
        exit_with_error("failed to start private thread");
    }
    if (t >= 0) {
        r = handle_nobody(session);
        if (h != INVALID_HANDLE_VALUE) {
#ifndef _WIN32
            //
#else
            //waiting for private thread to quit
            WaitForSingleObject(h, INFINITE);
            CloseHandle(h);
#endif
        }
    }

    session_free(session);

    return r;
}
tid_t start_session(Session_t* session, void** ph)
{
    tid_t tid = -1;
#ifndef _WIN32
    //PTHREAD
#else
    * ph = CreateThread(NULL, 0, session_thread, session, 0, (DWORD*)&tid);
#endif
    return tid;
}

static SOCKET listen_fd = 0;

#ifndef _WIN32
int loop_thread(void* lp)
#else
DWORD WINAPI loop_thread(void* lp) 
#endif
{
    int e = 0;
    int t = 0;
    init_session_manager();

    listen_fd = tcp_server(tunable_listen_address, tunable_listen_port);

    while (!should_exit())
    {
        struct sockaddr_in addr = { 0 };
        SOCKET peer_fd = accept_timeout(listen_fd, &addr, tunable_accept_timeout,&t);

        if (peer_fd == INVALID_SOCKET && 
#ifndef _WIN32
            (errno == ETIMEDOUT)
#else
            (((e = WSAGetLastError()) == WSAETIMEDOUT) || t!=0)
#endif
            ) {
            continue;
        }
        else if (peer_fd == INVALID_SOCKET) {
            exit_with_error("accept_failed or threaad exited");
            break;
        }

        uint32_t ip = addr.sin_addr.s_addr;
        Session_t* session = (Session_t*)malloc(sizeof(Session_t));

        if (session == 0)
        {
            s_close(&peer_fd);
            exit_with_error("unable to start session!");
            break;
        }
        else
        {
            session_init(session);
            session->peer_fd = peer_fd;
            session->ip = ip;

            void* handle = 0;
            tid_t tid = start_session(session, &handle);
            if (tid >= 0)
            {
                session->handle = handle;
            }
            else
            {
                session_free(session);
                exit_with_error("unable to start session!");
                break;
            }
        }
    }
    s_close(&listen_fd);

    wait_sessions();

    destroy_session_manager();

    return code;
}

void exit_loop() {
    s_close(&listen_fd);
    quit = 1;
}

void* start_loop(int looping) {
    DWORD ec = -1;
    DWORD tid = -1;
    HANDLE handle = CreateThread(NULL, 0, &loop_thread, 0, 0, &tid);
    
    if (handle != INVALID_HANDLE_VALUE) {
        if (looping) {
            while (!should_exit())
            {
#ifndef _WIN32
                usleep(10 * 1000);
#else
                Sleep(10);
#endif
            }
            WaitForSingleObject(handle, INFINITE);
        }
    }
    return handle;
}
#ifndef AS_LIBRARY
int startup_socket() {
#ifndef _WIN32
    return 0;
#else
    WSADATA wsa = { 0 };
    int r = WSAStartup(MAKEWORD(2, 2), &wsa);
    return r == 0;
#endif
}
int cleanup_socket() {
#ifndef _WIN32
    return 0;
#else
    int r = WSACleanup();
    return r == 0;
#endif
}

#ifdef _WIN32

BOOL WINAPI ctrlhandler(DWORD fdwctrltype)
{
    switch (fdwctrltype)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        exit_loop();
        return TRUE;
    default:
        return FALSE;
    }
}

#endif

int miniftp_start() {
    int r = 0;
    load_config("ftpserver.conf");
    {
        if (startup_socket())
        {
            void* h = start_loop(1);
            if (h != INVALID_HANDLE_VALUE) {
                DWORD ec = 0;
                if (GetExitCodeThread(h, &ec)) {
                    r = ec;
                }
                else {
                    r = 1;
                }
            }
            cleanup_socket();
        }
    }
    free_config();
    return r;
}
int main(int argc, const char* argv[])
{
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(120);
#endif
    
#ifdef _WIN32
    SetConsoleCtrlHandler(ctrlhandler, TRUE);
#endif
    return miniftp_start();
}
#endif
