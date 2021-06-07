#include "Buffer/BufferedStreamWriter.h"

BufferedStreamWriter::BufferedStreamWriter(uint8_t* pBuffer)
: m_pBuffer(pBuffer)
{
}

void BufferedStreamWriter::writeByte(uint8_t byte)
{
	if (m_pBuffer != NULL)
	{
		*m_pBuffer = byte;
		m_pBuffer++;
	}
}

void BufferedStreamWriter::writeShort(uint16_t shortValue)
{
	if (m_pBuffer != NULL)
	{
		writeByte(shortValue & 0xff);
		writeByte((shortValue >> 8) & 0xff);
	}
}

void BufferedStreamWriter::writeInt(uint32_t intValue)
{
	if (m_pBuffer != nullptr)
	{
		writeShort(intValue & 0xffff);
		writeShort((intValue >> 16) & 0xffff);
	}
}

void BufferedStreamWriter::writeLong(uint64_t longValue)
{
	if (m_pBuffer != nullptr)
	{
		writeInt(longValue & 0xffffffff);
		writeInt((longValue >> 32) & 0xffffffff);
	}
}