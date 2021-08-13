#include "Buffer/BufferedStreamReader.h"

BufferedStreamReader::BufferedStreamReader(uint8_t* pBuffer)
: m_pBuffer(pBuffer)
{
}

uint8_t BufferedStreamReader::readByte()
{
	if (m_pBuffer != NULL)
	{
		int8_t iValue = (*m_pBuffer & 0xff);
		m_pBuffer++;

		return iValue;
	}

	return -1;
}

uint16_t BufferedStreamReader::readShort()
{
	if (m_pBuffer != NULL)
	{
		return (readByte() | (readByte() << 8));
	}

	return -1;
}

uint32_t BufferedStreamReader::readInt()
{
	if (m_pBuffer != NULL)
	{
		return (readShort() | (readShort() << 16));
	}

	return -1;
}

uint64_t BufferedStreamReader::readLong()
{
	if (m_pBuffer != NULL)
	{
		return ((readInt() << 32) | readInt());
	}

	return -1;
}