#include "Engine/Timer.h"
#include <iomanip> // put_time

void Timer::start()
{
	m_tpStart = std::chrono::high_resolution_clock::now();
}

void Timer::stop()
{
	m_Duration = (std::chrono::high_resolution_clock::now() - m_tpStart);
}

double Timer::duration()
{
	return m_Duration.count();
}

uint64_t Timer::duration(DURATION_TYPE eDURATION_TYPE)
{
	if (eDURATION_TYPE == DURATION_TYPE::nanoseconds)
		return std::chrono::duration_cast<std::chrono::nanoseconds>(m_Duration).count();
	else
	if (eDURATION_TYPE == DURATION_TYPE::microseconds)
		return std::chrono::duration_cast<std::chrono::microseconds>(m_Duration).count();
	else
	if (eDURATION_TYPE == DURATION_TYPE::milliseconds)
		return std::chrono::duration_cast<std::chrono::milliseconds>(m_Duration).count();
	else
	if (eDURATION_TYPE == DURATION_TYPE::seconds)
		return std::chrono::duration_cast<std::chrono::seconds>(m_Duration).count();
	else
	if (eDURATION_TYPE == DURATION_TYPE::minutes)
		return std::chrono::duration_cast<std::chrono::minutes>(m_Duration).count();
	else
	if (eDURATION_TYPE == DURATION_TYPE::hours)
		return std::chrono::duration_cast<std::chrono::hours>(m_Duration).count();
}

std::string Timer::getCurrentTimeAndDate()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %T");
	return ss.str();
}
