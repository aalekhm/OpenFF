#pragma once
#include "Network/NetDefines.h"
#include "Common/Defines.h"

class NetBase
{
	public:
		static std::string	getDynamicIPAddress(OUT ULONG& ulIPAddress);
		bool				netInitialize();
		bool				netClose();

		virtual size_t		netSendAll(uint8_t* pBuf, size_t iLen) = 0;
		virtual size_t		netRecv(void* pBuf, size_t iLen) = 0;
		virtual size_t		netRecvAll(void* pBuf, size_t iLen) = 0;

		ULONG				m_iIPAddress;
		int32_t				m_iPort;

		bool				m_bIsConnected;
	protected:
		SOCKET				m_Socket;
	private:
};
