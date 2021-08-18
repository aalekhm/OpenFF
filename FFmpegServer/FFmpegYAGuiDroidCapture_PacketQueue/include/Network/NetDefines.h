#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <string>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

#define LOCALHOST		0x7F000001
#define PORT			8888

typedef SOCKET socket_t;

#define MALLOC(x)       HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x)         HeapFree(GetProcessHeap(), 0, (x))