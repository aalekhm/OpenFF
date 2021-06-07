#pragma once
#include "Buffer/BufferedStream.h"

class BufferedStreamReader : public BufferedStream
{
	public:
					BufferedStreamReader(uint8_t* pBuffer);

		uint8_t		readByte();
		uint16_t	readShort();
		uint32_t	readInt();
		uint64_t	readLong();
	protected:
	private:
		uint8_t*	m_pBuffer;
};