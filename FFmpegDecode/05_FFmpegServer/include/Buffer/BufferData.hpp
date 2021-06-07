#pragma once
#include "Common/Defines.h"

class BufferData
{
	public:
		BufferData() = delete;
		BufferData(size_t iBufferSize)
		: m_iBufferSize(iBufferSize)
		{
			m_pBuffer = (uint8_t*)std::malloc(iBufferSize);
		}

		virtual ~BufferData()
		{
			if (m_pBuffer != nullptr)
			{
				std::free(m_pBuffer);
			}
		}

		uint8_t*		m_pBuffer;
		size_t			m_iBufferSize;
	protected:
	private:
};
