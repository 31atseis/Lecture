#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
typedef SOCKET netsock;
#ifndef TCP_NODELAY
#define TCP_NODELAY 1
#endif

typedef int (WSAAPI *fn_startup)(WORD, LPWSADATA);
typedef int (WSAAPI *fn_cleanup)(void);
typedef SOCKET (WSAAPI *fn_socket)(int, int, int);
typedef int (WSAAPI *fn_bind)(SOCKET, const struct sockaddr *, int);
typedef int (WSAAPI *fn_listen)(SOCKET, int);
typedef SOCKET (WSAAPI *fn_accept)(SOCKET, struct sockaddr *, int *);
typedef int (WSAAPI *fn_connect)(SOCKET, const struct sockaddr *, int);
typedef int (WSAAPI *fn_send)(SOCKET, const char *, int, int);
typedef int (WSAAPI *fn_recv)(SOCKET, char *, int, int);
typedef int (WSAAPI *fn_close)(SOCKET);
typedef int (WSAAPI *fn_opt)(SOCKET, int, int, const char *, int);
typedef unsigned long (WSAAPI *fn_addr)(const char *);

static fn_startup fp_startup;
static fn_cleanup fp_cleanup;
static fn_socket fp_socket;
static fn_bind fp_bind;
static fn_listen fp_listen;
static fn_accept fp_accept;
static fn_connect fp_connect;
static fn_send fp_send;
static fn_recv fp_recv;
static fn_close fp_close;
static fn_opt fp_opt;
static fn_addr fp_addr;

static int load_ws(void)
{
    HMODULE dll;

    if (fp_socket)
        return 0;
    dll = LoadLibraryA("ws2_32.dll");
    if (!dll)
        return -1;
    fp_startup = (fn_startup)GetProcAddress(dll, "WSAStartup");
    fp_cleanup = (fn_cleanup)GetProcAddress(dll, "WSACleanup");
    fp_socket = (fn_socket)GetProcAddress(dll, "socket");
    fp_bind = (fn_bind)GetProcAddress(dll, "bind");
    fp_listen = (fn_listen)GetProcAddress(dll, "listen");
    fp_accept = (fn_accept)GetProcAddress(dll, "accept");
    fp_connect = (fn_connect)GetProcAddress(dll, "connect");
    fp_send = (fn_send)GetProcAddress(dll, "send");
    fp_recv = (fn_recv)GetProcAddress(dll, "recv");
    fp_close = (fn_close)GetProcAddress(dll, "closesocket");
    fp_opt = (fn_opt)GetProcAddress(dll, "setsockopt");
    fp_addr = (fn_addr)GetProcAddress(dll, "inet_addr");
    if (!fp_startup || !fp_cleanup || !fp_socket || !fp_bind || !fp_listen
        || !fp_accept || !fp_connect || !fp_send || !fp_recv || !fp_close
        || !fp_opt || !fp_addr)
        return -1;
    return 0;
}

#define socket fp_socket
#define bind fp_bind
#define listen fp_listen
#define accept fp_accept
#define connect fp_connect
#define send fp_send
#define recv fp_recv
#define closesocket fp_close
#define setsockopt fp_opt
#define inet_addr fp_addr

static unsigned short to_be16(unsigned short x)
{
    return (unsigned short)((x << 8) | (x >> 8));
}

static unsigned long to_be32(unsigned long x)
{
    return ((x & 0x000000ffUL) << 24) | ((x & 0x0000ff00UL) << 8)
        | ((x & 0x00ff0000UL) >> 8) | ((x & 0xff000000UL) >> 24);
}

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
typedef int netsock;
#define INVALID_SOCKET (-1)
#define closesocket close
#define to_be16 htons
#define to_be32 htonl
#endif

#include "net.h"
#include "con.h"

static int started;

static int net_start(void)
{
#ifdef _WIN32
    WSADATA w;

    if (started)
        return 0;
    if (load_ws() < 0)
        return -1;
    if (fp_startup(MAKEWORD(2, 2), &w) != 0)
        return -1;
#else
    if (started)
        return 0;
#endif
    started = 1;
    return 0;
}

static void nodelay(netsock s)
{
    int on;

    on = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof on);
}

void net_stop(void)
{
#ifdef _WIN32
    if (started && fp_cleanup)
        fp_cleanup();
#endif
    started = 0;
}

void net_close(int sock)
{
    if (sock >= 0)
        closesocket((netsock)sock);
}

int net_send_line(int sock, const char *line)
{
    char buf[512];
    int len, sent, n, flags;

    if (strlen(line) + 2 > sizeof buf)
        return -1;
    len = sprintf(buf, "%s\n", line);
    sent = 0;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#else
    flags = 0;
#endif
    while (sent < len) {
        n = (int)send((netsock)sock, buf + sent, len - sent, flags);
        if (n <= 0)
            return -1;
        sent += n;
    }
    return 0;
}

int net_recv_line(int sock, char *buf, int cap)
{
    int i, n;
    char ch;

    if (cap < 2)
        return -1;
    i = 0;
    while (1) {
        n = (int)recv((netsock)sock, &ch, 1, 0);
        if (n <= 0)
            return -1;
        if (ch == '\n')
            break;
        if (ch == '\r')
            continue;
        if (i < cap - 1)
            buf[i++] = ch;
    }
    buf[i] = '\0';
    return 0;
}

void net_show_me(int port)
{
    int count;

    count = 0;
    printf("\nСкинь другу:\n");
#ifdef _WIN32
    {
        HMODULE dll;
        typedef ULONG (WINAPI *fn_info)(PIP_ADAPTER_INFO, PULONG);
        fn_info getinfo;
        IP_ADAPTER_INFO *info, *it;
        IP_ADDR_STRING *addr;
        ULONG sz;

        dll = LoadLibraryA("iphlpapi.dll");
        getinfo = NULL;
        info = NULL;
        if (dll)
            getinfo = (fn_info)GetProcAddress(dll, "GetAdaptersInfo");
        sz = 0;
        if (getinfo)
            getinfo(NULL, &sz);
        if (getinfo && sz > 0)
            info = (IP_ADAPTER_INFO *)malloc(sz);
        if (info && getinfo(info, &sz) == 0) {
            for (it = info; it; it = it->Next) {
                for (addr = &it->IpAddressList; addr; addr = addr->Next) {
                    if (addr->IpAddress.String[0] == '\0')
                        continue;
                    if (strcmp(addr->IpAddress.String, "0.0.0.0") == 0)
                        continue;
                    if (strcmp(addr->IpAddress.String, "127.0.0.1") == 0)
                        continue;
                    if (strncmp(addr->IpAddress.String, "169.254.", 8) == 0)
                        continue;
                    printf("IP: %s\n", addr->IpAddress.String);
                    count++;
                }
            }
        }
        free(info);
    }
#else
    {
        struct ifaddrs *list, *it;
        char text[64];

        list = NULL;
        if (getifaddrs(&list) == 0) {
            for (it = list; it; it = it->ifa_next) {
                struct sockaddr_in *in;
                if (!it->ifa_addr || it->ifa_addr->sa_family != AF_INET)
                    continue;
                in = (struct sockaddr_in *)it->ifa_addr;
                if (in->sin_addr.s_addr == to_be32(INADDR_LOOPBACK))
                    continue;
                inet_ntop(AF_INET, &in->sin_addr, text, sizeof text);
                printf("IP: %s\n", text);
                count++;
            }
            freeifaddrs(list);
        }
    }
#endif
    if (count == 0)
        printf("IP не нашёлся, посмотри ipconfig\n");
    printf("Порт: %d\n\n", port);
}

int net_host(int port)
{
    netsock srv, cli;
    struct sockaddr_in addr;
    int on;

    if (net_start() < 0)
        return -1;
    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == INVALID_SOCKET)
        return -1;
    on = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof on);
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = to_be32(INADDR_ANY);
    addr.sin_port = to_be16((unsigned short)port);
    if (bind(srv, (struct sockaddr *)&addr, sizeof addr) != 0) {
        closesocket(srv);
        return -1;
    }
    if (listen(srv, 1) != 0) {
        closesocket(srv);
        return -1;
    }
    net_show_me(port);
    printf("Жду, пока друг подключится...\n");
    cli = accept(srv, NULL, NULL);
    closesocket(srv);
    if (cli == INVALID_SOCKET)
        return -1;
    nodelay(cli);
    printf("Второй игрок подключился\n");
    return (int)cli;
}

int net_join(const char *ip, int port)
{
    netsock s;
    struct sockaddr_in addr;

    if (net_start() < 0)
        return -1;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
        return -1;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = to_be16((unsigned short)port);
    if (strcmp(ip, "localhost") == 0)
        addr.sin_addr.s_addr = to_be32(INADDR_LOOPBACK);
    else
        addr.sin_addr.s_addr = inet_addr(ip);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        closesocket(s);
        return -1;
    }
    if (connect(s, (struct sockaddr *)&addr, sizeof addr) != 0) {
        closesocket(s);
        return -1;
    }
    nodelay(s);
    return (int)s;
}
