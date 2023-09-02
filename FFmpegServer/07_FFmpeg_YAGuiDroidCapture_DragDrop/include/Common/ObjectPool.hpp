#pragma once
#include "Object.hpp"

#define LOCKGUARD_MUTEX(__mutex__)		std::lock_guard<std::mutex> muLockObj(__mutex__);
#define LOCK_MUTEX(__mutex__)			__mutex__.lock();
#define UNLOCK_MUTEX(__mutex__)			__mutex__.unlock();

template<typename T>
class CObjectPool
{
	public:
		SPacket<T>* GetPacketFromPool()
		{
			SPacket<T>* pReturnPacket = nullptr;

			LOCK_MUTEX(m_MutexPacketPool)
			{	
				if (m_vPacketPool.size() == 0)
				{
					SPacket<T>* pPacket = (SPacket<T>*)std::malloc(sizeof(SPacket<T>));
					if (pPacket)
					{
						m_vPacketPool.push_back(pPacket);
					}
				}

				pReturnPacket = m_vPacketPool.back();
				m_vPacketPool.pop_back();
			}
			UNLOCK_MUTEX(m_MutexPacketPool)

			return pReturnPacket;
		}

		void ReturnPacketToPool(SPacket<T>* pPacket)
		{
			LOCK_MUTEX(m_MutexPacketPool)
			{
				if (pPacket != nullptr)
				{
					m_vPacketPool.push_back(pPacket);
				}
			}
			UNLOCK_MUTEX(m_MutexPacketPool)
		}

		void Clear()
		{
			for (SPacket<T>* pDataPacket : m_vPacketPool)
			{
				if (pDataPacket)
				{
					std::free(pDataPacket);
				}
			}

			m_vPacketPool.clear();
		}
	protected:
	private:
		std::vector<SPacket<T>*>		m_vPacketPool;
		std::mutex						m_MutexPacketPool;
};