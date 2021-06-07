#include "Network/NetBase.h"

bool NetBase::netInitialize()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return false;
	}

	m_bIsConnected = false;

	return true;
}

bool NetBase::netClose()
{
	return !closesocket(m_Socket);
}
