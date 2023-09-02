#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

//#define IPV4_LOCALHOST	0x7F000001	// 127.0.0.1
#define LOCALHOST		0x7F000001
#define IPV4_LOCALHOST	0xC0A82B68	// 192.168.1.2
#define PORT			8888

typedef SOCKET socket_t;