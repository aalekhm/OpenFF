#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

//#define IPV4_LOCALHOST	0x7F000001	// 127.0.0.1
#define IPV4_LOCALHOST	0xC0A80103	// 192.168.1.3
#define PORT			5555

typedef SOCKET socket_t;