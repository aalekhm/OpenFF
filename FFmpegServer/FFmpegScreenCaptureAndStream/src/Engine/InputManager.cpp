#include "Engine/InputManager.h"
#include "GLFW/glfw3.h"

bool InputManager::isKeyPressed(int32_t iKey)
{
	return m_KeyboardBits.test(iKey);
}

bool InputManager::isMousePressed(int32_t iKey)
{
	return m_MouseBits.test(iKey);
}

void InputManager::getMousePos(double& xPos, double& yPos)
{
	xPos = m_dMouseX;
	yPos = m_dMouseY;
}

double InputManager::getMouseX()
{
	return m_dMouseX;
}

double InputManager::getMouseY()
{
	return m_dMouseY;
}

void InputManager::onKeyPressed(uint32_t iKey)
{
	m_KeyboardBits.set(iKey, 1);
}

void InputManager::onKeyReleased(uint32_t iKey)
{
	m_KeyboardBits.reset(iKey);
}
	 
void InputManager::onMousePressed(uint32_t iKey)
{
	m_MouseBits.set(iKey, 1);

	for (auto fCallback : m_fMouseCallbackList)
	{
		(*fCallback)(GLFW_PRESS, iKey, m_dMouseX, m_dMouseY);
	}
}

void InputManager::onMouseReleased(uint32_t iKey)
{
	m_MouseBits.reset(iKey);

	for (auto fCallback : m_fMouseCallbackList)
	{
		(*fCallback)(GLFW_RELEASE, iKey, m_dMouseX, m_dMouseY);
	}
}
	 
void InputManager::onMouseMoved(double xPos, double yPos)
{
	m_dMouseX = xPos;
	m_dMouseY = yPos;

	for (auto fCallback : m_fMouseCallbackList)
	{
		(*fCallback)(GLFW_REPEAT, m_MouseBits.to_ulong(), m_dMouseX, m_dMouseY);
	}
}

void InputManager::addMouseListener(MOUSE_FUNC_CALLBACK* fMouseCallbackList)
{
	if (isMouseListenerInList(fMouseCallbackList) >= 0)
	{
		m_fMouseCallbackList.push_back(fMouseCallbackList);
	}
}

void InputManager::removeMouseListener(MOUSE_FUNC_CALLBACK* fMouseCallbackList)
{
	int32_t iPos = isMouseListenerInList(fMouseCallbackList);
	if (iPos >= 0)
	{
		m_fMouseCallbackList.erase(m_fMouseCallbackList.begin() + iPos);
	}
}

bool InputManager::isMouseListenerInList(MOUSE_FUNC_CALLBACK* fMouseCallbackList)
{
	int32_t iPos = -1;
	for (auto fCallback : m_fMouseCallbackList)
	{
		iPos++;
		if (fCallback == fMouseCallbackList)
		{
			iPos;
		}
	}

	return -1;
}