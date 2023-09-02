#pragma once
#include "Buffer/BufferedStream.h"

class BufferedStreamWriter : public BufferedStream
{
	public:
					BufferedStreamWriter(uint8_t* pBuffer);

		void		writeByte(uint8_t byte);
		void		writeShort(uint16_t shortValue);
		void		writeInt(uint32_t intValue);
		void		writeLong(uint64_t longValue);
	protected:
	private:
		uint8_t*	m_pBuffer;
};