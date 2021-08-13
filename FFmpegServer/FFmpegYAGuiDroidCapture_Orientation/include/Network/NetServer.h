#pragma once
#include "Network/NetBase.h"

class NetServer : public NetBase
{
	public:
		bool				netListen(uint32_t iAddr, uint16_t iPort, int32_t iBacklog);

		virtual size_t		netSendAll(uint8_t* pBuf, size_t iLen);
		virtual size_t		netRecv(void* pBuf, size_t iLen);
		virtual size_t		netRecvAll(void* pBuf, size_t iLen);
	protected:
	private:
		SOCKET				m_ClientSocket;
};
