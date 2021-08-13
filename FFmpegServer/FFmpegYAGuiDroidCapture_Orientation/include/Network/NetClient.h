#pragma once
#include "Network/NetBase.h"

class NetClient : public NetBase
{
	public:
		bool				netConnect(uint32_t iAddr, uint16_t iPort);

		virtual size_t		netSendAll(uint8_t* pBuf, size_t iLen);
		virtual size_t		netRecv(void* pBuf, size_t iLen);
		virtual size_t		netRecvAll(void* pBuf, size_t iLen);
	protected:
	private:
};
