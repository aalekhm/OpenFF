#include "Network/NetServer.h"

bool NetServer::netListen(uint32_t iAddr, uint16_t iPort, int32_t iBacklog)
{
	// 1.	Create a socket descriptor, an integer (like a file-handle).
	if((m_Socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		perror("Invalid socket.");
		return false;
	}
	LOG_CONSOLE("Socket created");

	// 2.	Forcefully attaching socket to the port 8080  
	int iReuse = 1;
	if (setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&iReuse, sizeof(iReuse)))
	{
		perror("Error setsockopt(SO_REUSEADDR).");
	}
	LOG_CONSOLE("Socket attached");

	SOCKADDR_IN sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(iAddr);
	sin.sin_port = htons(iPort);

	// 3.	Bind the socket to the address and port number specified in addr.
	if (bind(m_Socket, (SOCKADDR*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		perror("Error binding.");
		netClose();
		return false;
	}
	LOG_CONSOLE("Socket 'bind' successfull");

	// 4.	Put the server socket in a passive mode, where it listens for the client to approach the server to make a connection.
	//		The backlog, defines the maximum length to which the queue of pending connections for sockfd may grow.
	if (listen(m_Socket, iBacklog) == SOCKET_ERROR)
	{
		perror("Error listening.");
		netClose();
		return false;
	}
	
	LOG_CONSOLE("Socket 'listen'ing for incoming connection...");

	SOCKADDR_IN csin;
	socklen_t sinsize = sizeof(csin);
	if ((m_ClientSocket = accept(m_Socket, (struct sockaddr*)&csin, &sinsize)) < 0)
	{
		perror("Error accepting.");
		netClose();
		return false;
	}

	LOG_CONSOLE("Connected to Client");

	m_bIsConnected = true;
	return true;
}

size_t NetServer::netSendAll(uint8_t* pBuf, size_t iLen)
{
	size_t w = 0;

	if (pBuf != nullptr)
	{
		while (iLen > 0)
		{
			w = send(m_ClientSocket, (const char*)pBuf, iLen, 0);
			if (w == -1) 
			{
				return -1;
			}
			iLen -= w;
			pBuf = pBuf + w;
		}
	}

	return w;
}

size_t NetServer::netRecv(void* pBuf, size_t iLen)
{
	return recv(m_ClientSocket, (char*)pBuf, iLen, 0);
}

size_t NetServer::netRecvAll(void* pBuf, size_t iLen)
{
	return recv(m_ClientSocket, (char*)pBuf, iLen, MSG_WAITALL);
}