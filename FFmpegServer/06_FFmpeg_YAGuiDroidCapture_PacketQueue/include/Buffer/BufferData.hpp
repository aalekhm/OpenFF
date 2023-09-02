#pragma once
#include "Common/Defines.h"
#include "Common/RandomAccessFile.h"

class BufferData
{
	public:
		BufferData()
		: m_iBufferSize(-1)
		, m_pBuffer(nullptr)
		{ }

		BufferData(size_t iBufferSize)
		{
			setSize(iBufferSize);
		}

		void operator=(BufferData& pBufferData)
		{
			cleanup();

			setSize(pBufferData.m_iBufferSize);
			memcpy_s(m_pBuffer, m_iBufferSize, pBufferData.m_pBuffer, pBufferData.m_iBufferSize);
		}

		void setSize(size_t iBufferSize)
		{
			if (iBufferSize != m_iBufferSize)
			{
				cleanup();
			}

			m_iBufferSize = iBufferSize;
			m_pBuffer = (uint8_t*)std::malloc(iBufferSize);
		}

		virtual ~BufferData()
		{
			cleanup();
		}

		void cleanup()
		{
			m_iBufferSize = -1;
			if (m_pBuffer != nullptr)
			{
				std::free(m_pBuffer);
				m_pBuffer = nullptr;
			}
		}

		void saveToDisk(std::string sFileName)
		{
			RandomAccessFile* raf = new RandomAccessFile();
			if (raf->openForWrite(sFileName.c_str()))
			{
				LOG_CONSOLE("Saving buffer to disk! size = " << m_iBufferSize);
				raf->write((char*)m_pBuffer, 0, m_iBufferSize);
				raf->close();
			}
		}

		uint8_t*		m_pBuffer;
		size_t			m_iBufferSize;
	protected:
	private:
};
