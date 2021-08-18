#pragma once
#include "ObjectPool.hpp"

template<typename T>
class ObjectQueue
{
	private:
		SPacket<T>*		m_pFirst;
		SPacket<T>*		m_pLast;

		CObjectPool<T>	m_ObjectPool;
	public:
		int32_t			m_iNumberOfPackets;
		int32_t			m_iSize;

		void PacketQueueInit()
		{
			
		}

		int32_t PacketQueuePut(const T* pSrcPacket)
		{
			SPacket<T>* pPacket;
			int32_t iError = -1;
			{
				// Get a AVPacketList from the 'Packet Pool'
				pPacket = m_ObjectPool.GetPacketFromPool();
				if (pPacket != nullptr)
				{
					// ***** IMPORTANT *****
					// Even if the 'Destination pPacket->m_pData.data' size is less than the 'Source pSrcPacket.data' size, 
					// once the packet is assigned, the Destination AVPacket adjusts its container & increases its buffer data size to match the Source. 
					// And that is the reason why we can use a Packet Pool here!
					// ***** IMPORTANT *****
					pPacket->m_pData = *pSrcPacket;
					pPacket->m_pNext = nullptr;
				}

				// Add the 'AVPacketList' to our Packet Queue
				if (m_pLast == nullptr)
				{
					m_pFirst = pPacket;
				}
				else
				{
					m_pLast->m_pNext = pPacket;
				}

				m_pLast = pPacket;
				m_iNumberOfPackets++;

				iError = 0;
			}

			return iError;
		}

		int32_t PacketQueueGet(T* pDestPacket)
		{
			SPacket<T>* pPacket;
			int32_t iError = -1;

			if (pDestPacket != nullptr)
			{
				pPacket = m_pFirst;
				if (pPacket)
				{
					m_pFirst = pPacket->m_pNext;
					if (m_pFirst == nullptr)
						m_pLast = nullptr;

					m_iNumberOfPackets--;
					*pDestPacket = pPacket->m_pData;

					// Return AVPacketList back to the 'Packet Pool'
					m_ObjectPool.ReturnPacketToPool(pPacket);

					iError = 1;
				}
				else
				{
					iError = 0;
				}
			}

			return iError;
		}

		void Clear()
		{
			m_ObjectPool.Clear();
		}
};