#pragma once
#include "Common/Defines.h"
#include <chrono>

class Timer
{
	public:
		void				start();
		void				stop();
		double				duration();
		uint64_t			duration(DURATION_TYPE eDURATION_TYPE);

		static std::string	getCurrentTimeAndDate();
	protected:
	private:
		std::chrono::high_resolution_clock::time_point		m_tpStart;
		std::chrono::duration<double>						m_Duration;
};