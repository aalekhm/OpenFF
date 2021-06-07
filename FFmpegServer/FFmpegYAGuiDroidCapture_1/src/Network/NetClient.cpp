#include "Network/NetClient.h"

bool NetClient::netConnect(uint32_t iAddr, uint16_t iPort)
{
	// 1.	Create a socket descriptor, an integer (like a file-handle).
	if ((m_Socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		perror("Invalid socket.");
		return false;
	}

	LOG_CONSOLE("Socket created");

	SOCKADDR_IN sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(iAddr);
	sin.sin_port = htons(iPort);

	// 2. Connect to remote server
	if (connect(m_Socket, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		perror("Invalid connection.");
		return false;
	}

	LOG_CONSOLE("Connected to Server");

	m_bIsConnected = true;
	return true;
}

size_t NetClient::netSendAll(uint8_t* pBuf, size_t iLen)
{
	size_t w = 0;

	// Send the buffer without delay.
	//int flag = 1;
	//setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

	if (pBuf != nullptr)
	{
		while (iLen > 0)
		{
			w = send(m_Socket, (const char*)pBuf, iLen, 0);
			if (w == -1) {
				return -1;
			}
			iLen -= w;
			pBuf = pBuf + w;
		}
	}

	return w;
}

size_t NetClient::netRecv(void* pBuf, size_t iLen)
{
	return recv(m_Socket, (char*)pBuf, iLen, 0);
}

size_t NetClient::netRecvAll(void* pBuf, size_t iLen)
{
	return recv(m_Socket, (char*)pBuf, iLen, MSG_WAITALL);
}