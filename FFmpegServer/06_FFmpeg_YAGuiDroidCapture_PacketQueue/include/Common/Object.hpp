#pragma once

template<typename T>
struct SPacket
{
	T			m_pData;
	SPacket*	m_pNext;
};
