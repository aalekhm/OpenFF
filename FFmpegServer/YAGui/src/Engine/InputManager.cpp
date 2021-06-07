#include "Engine/InputManager.h"
#include "Common/Defines.h"

#define ANDROID_KEYCODE_A		29
#define ANDROID_KEYCODE_0		7


#define ANDROID_KEY_CAPS_LOCK	1048576
#define ANDROID_KEY_NUM_LOCK	2097152
#define ANDROID_KEY_SCROLL_LOCK	4194304

#define ANDROID_KEYCODE_LEFT_BRACKET		71
#define ANDROID_KEYCODE_RIGHT_BRACKET		72
#define ANDROID_KEYCODE_BACKSLASH			73

#define ANDROID_KEYCODE_SEMICOLON			74
#define ANDROID_KEYCODE_APOSTROPHE			75

#define ANDROID_KEYCODE_COMMA				55 
#define ANDROID_KEYCODE_PERIOD				56 
#define ANDROID_KEYCODE_SLASH				76 

#define ANDROID_KEYCODE_MINUS				69 
#define ANDROID_KEYCODE_EQUALS				70 

#define ANDROID_KEYCODE_DEL					67
#define ANDROID_KEYCODE_FORWARD_DEL			112

#define ANDROID_KEYCODE_STAR				17
#define ANDROID_KEYCODE_PLUS				81
#define ANDROID_KEYCODE_ENTER				66

#define ANDROID_KEYCODE_POWER				26
#define ANDROID_KEYCODE_SPACE				62

bool InputManager::isKeyPressed(int32_t iKey)
{
	return m_KeyboardBits.test(iKey);
}

bool InputManager::isMousePressed(int32_t iKey)
{
	return m_MouseBitsPress.test(iKey);
}

bool InputManager::isMouseReleased(int32_t iKey)
{
	return m_MouseBitsRelease.test(iKey);
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

bool InputManager::isNumLock()
{
	return m_AndroidMetaBits.test(21); // ANDROID_KEY_NUM_LOCK
}

int32_t InputManager::mapToAndroidKeyCode(int32_t iAsciiKeyCode)
{
	int iOffset = 0;

	if(iAsciiKeyCode >= 'A' && iAsciiKeyCode <= 'Z')
	{
		iOffset = 'A' - ANDROID_KEYCODE_A;
		return (iAsciiKeyCode - iOffset);
	}
	if (iAsciiKeyCode >= '0' && iAsciiKeyCode <= '9')
	{
		iOffset = '0' - ANDROID_KEYCODE_0;
		return (iAsciiKeyCode - iOffset);
	}

	if (iAsciiKeyCode == '[') return ANDROID_KEYCODE_LEFT_BRACKET;
	if (iAsciiKeyCode == ']') return ANDROID_KEYCODE_RIGHT_BRACKET;
	if (iAsciiKeyCode == '\\') return ANDROID_KEYCODE_BACKSLASH;

	if (iAsciiKeyCode == ';') return ANDROID_KEYCODE_SEMICOLON;
	if (iAsciiKeyCode == '\'') return ANDROID_KEYCODE_APOSTROPHE;

	if (iAsciiKeyCode == ',') return ANDROID_KEYCODE_COMMA;
	if (iAsciiKeyCode == '.') return ANDROID_KEYCODE_PERIOD;
	if (iAsciiKeyCode == '/') return ANDROID_KEYCODE_SLASH;

	if (iAsciiKeyCode == '-') return ANDROID_KEYCODE_MINUS;
	if (iAsciiKeyCode == '=') return ANDROID_KEYCODE_EQUALS;

	if (iAsciiKeyCode == GLFW_KEY_BACKSPACE) return ANDROID_KEYCODE_DEL;
	if (iAsciiKeyCode == GLFW_KEY_DELETE) return ANDROID_KEYCODE_FORWARD_DEL;

	if (iAsciiKeyCode == GLFW_KEY_KP_0)	return isNumLock() ? ANDROID_KEYCODE_0 + 0 : -1;
	if (iAsciiKeyCode == GLFW_KEY_KP_1) return isNumLock() ? ANDROID_KEYCODE_0 + 1 : -1;
	if (iAsciiKeyCode == GLFW_KEY_KP_2)	return isNumLock() ? ANDROID_KEYCODE_0 + 2 : -1;
	if (iAsciiKeyCode == GLFW_KEY_KP_3)	return isNumLock() ? ANDROID_KEYCODE_0 + 3 : -1;
	if (iAsciiKeyCode == GLFW_KEY_KP_4)	return isNumLock() ? ANDROID_KEYCODE_0 + 4 : -1;
	if (iAsciiKeyCode == GLFW_KEY_KP_5)	return isNumLock() ? ANDROID_KEYCODE_0 + 5 : -1;
	if (iAsciiKeyCode == GLFW_KEY_KP_6)	return isNumLock() ? ANDROID_KEYCODE_0 + 6 : -1;
	if (iAsciiKeyCode == GLFW_KEY_KP_7)	return isNumLock() ? ANDROID_KEYCODE_0 + 7 : -1;
	if (iAsciiKeyCode == GLFW_KEY_KP_8)	return isNumLock() ? ANDROID_KEYCODE_0 + 8 : -1;
	if (iAsciiKeyCode == GLFW_KEY_KP_9)	return isNumLock() ? ANDROID_KEYCODE_0 + 9 : -1;
	
	if (iAsciiKeyCode == GLFW_KEY_KP_DECIMAL)		return isNumLock() ? ANDROID_KEYCODE_PERIOD: ANDROID_KEYCODE_FORWARD_DEL;
	if (iAsciiKeyCode == GLFW_KEY_KP_DIVIDE)		return ANDROID_KEYCODE_SLASH;
	if (iAsciiKeyCode == GLFW_KEY_KP_MULTIPLY)		return ANDROID_KEYCODE_STAR;
	if (iAsciiKeyCode == GLFW_KEY_KP_SUBTRACT)		return ANDROID_KEYCODE_MINUS;
	if (iAsciiKeyCode == GLFW_KEY_KP_ADD)			return ANDROID_KEYCODE_PLUS;
	if (iAsciiKeyCode == GLFW_KEY_KP_ENTER 
		|| 
		iAsciiKeyCode == GLFW_KEY_ENTER)			return ANDROID_KEYCODE_ENTER;

	if (iAsciiKeyCode == GLFW_KEY_TAB)				return ANDROID_KEYCODE_POWER;
	if (iAsciiKeyCode == GLFW_KEY_SPACE)			return ANDROID_KEYCODE_SPACE;

	return -1;
}

bool InputManager::alterAndroidExtraMetaModifiers(bool bSet, int32_t iAsciiKeyCode)
{
	int32_t iAndroidMetaKey = 0;
	int32_t iBit = 0;

	if (iAsciiKeyCode == GLFW_KEY_CAPS_LOCK)
	{
		iAndroidMetaKey = ANDROID_KEY_CAPS_LOCK;
		iBit = 20;
	}
	if (iAsciiKeyCode == GLFW_KEY_SCROLL_LOCK)
	{
		iAndroidMetaKey = ANDROID_KEY_SCROLL_LOCK;
		iBit = 22;
	}
	if (iAsciiKeyCode == GLFW_KEY_NUM_LOCK)
	{
		iAndroidMetaKey = ANDROID_KEY_NUM_LOCK;
		iBit = 21;
	}

	if (bSet && iBit > 0)
	{
		bool bIsBitSet = m_AndroidMetaBits.test(iBit);
		m_AndroidMetaBits.set(iBit, bIsBitSet ? 0 : 1);

		return true;
	}

	return false;
}

bool InputManager::alterAndroidMetaModifiers(bool bSet, int32_t iAsciiKeyCode)
{
	int32_t iAndroidMetaKey = 0;
	int32_t iBit = 0;

	if(NOT alterAndroidExtraMetaModifiers(bSet, iAsciiKeyCode))
	{
		if (iAsciiKeyCode == GLFW_KEY_LEFT_SHIFT)
		{
			iAndroidMetaKey = 64;
			iBit = 6;
		}
		if (iAsciiKeyCode == GLFW_KEY_RIGHT_SHIFT)
		{
			iAndroidMetaKey = 128;
			iBit = 7;
		}
		if (iAsciiKeyCode == GLFW_KEY_LEFT_CONTROL)
		{
			iAndroidMetaKey = 8192;
			iBit = 13;
		}
		if (iAsciiKeyCode == GLFW_KEY_RIGHT_CONTROL)
		{
			iAndroidMetaKey = 16384;
			iBit = 14;
		}
		if (iAsciiKeyCode == GLFW_KEY_LEFT_ALT)
		{
			iAndroidMetaKey = 16;
			iBit = 4;
		}
		if (iAsciiKeyCode == GLFW_KEY_RIGHT_ALT)
		{
			iAndroidMetaKey = 32;
			iBit = 5;
		}
		
		m_AndroidMetaBits.set(iBit, bSet ? 1 : 0);
		//LOG_CONSOLE("B = " << iBit << " == " << (1 << iBit));
		//LOG_CONSOLE("M = " << m_AndroidMetaBits << " == " << m_AndroidMetaBits.to_ulong());
	}

	return (iBit > 0);
}

void InputManager::onUnicodeChar(unsigned int iUnicodeChar)
{
	for (auto fCallback : m_fUnicodeCharCallbackList)
	{
		(*fCallback)(iUnicodeChar);
	}
}

void InputManager::onUnicodeCharModifier(unsigned int iUnicodeChar, int iModifier)
{
	for (auto fCallback : m_fUnicodeCharModifierCallbackList)
	{
		(*fCallback)(iUnicodeChar, iModifier);
	}
}

bool InputManager::isModifierKeyDown(int32_t iModifierKey)
{
	return ((m_iModifierKeys & iModifierKey) > 0);
}

void InputManager::onKeyPressed(int iKey, int iScanCode, int iAction, int iModifierKeys)
{
	m_KeyboardBits.set(iKey, 1);
	m_iModifierKeys = iModifierKeys;

	for (auto fCallback : m_fKeyboardCallbackList)
	{
		(*fCallback)(GLFW_PRESS, iKey, iScanCode, iModifierKeys);
	}

	if (NOT alterAndroidMetaModifiers(true, iKey))
	{
		int32_t iAndroidKeyCode = mapToAndroidKeyCode(iKey);

		for (auto fCallback : m_fAndroidKeyboardCallbackList)
		{
			(*fCallback)(GLFW_PRESS, iAndroidKeyCode, m_AndroidMetaBits.to_ulong());
		}
	}
}

void InputManager::onKeyReleased(int iKey, int iScanCode, int iAction, int iModifierKeys)
{
	m_KeyboardBits.reset(iKey);

	for (auto fCallback : m_fKeyboardCallbackList)
	{
		(*fCallback)(GLFW_RELEASE, iKey, iScanCode, iModifierKeys);
	}

	if (NOT alterAndroidMetaModifiers(false, iKey))
	{
		int32_t iAndroidKeyCode = mapToAndroidKeyCode(iKey);

		for (auto fCallback : m_fAndroidKeyboardCallbackList)
		{
			(*fCallback)(GLFW_RELEASE, iAndroidKeyCode, m_AndroidMetaBits.to_ulong());
		}
	}
}
	 
void InputManager::onMousePressed(uint32_t iKey)
{
	m_MouseBitsPress.set(iKey, 1);
	m_MouseBitsRelease.reset(iKey);

	for (auto fCallback : m_fMouseCallbackList)
	{
		(*fCallback)(GLFW_PRESS, iKey, m_dMouseX, m_dMouseY);
	}
}

void InputManager::onMouseReleased(uint32_t iKey)
{
	m_MouseBitsPress.reset(iKey);
	m_MouseBitsRelease.set(iKey, 1);

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
		(*fCallback)(GLFW_REPEAT, m_MouseBitsPress.to_ulong(), m_dMouseX, m_dMouseY);
	}
}

void InputManager::addMouseListener(MOUSE_FUNC_CALLBACK* fMouseCallback)
{
	if (isMouseListenerInList(fMouseCallback) >= 0)
	{
		m_fMouseCallbackList.push_back(fMouseCallback);
	}
}

void InputManager::removeMouseListener(MOUSE_FUNC_CALLBACK* fMouseCallback)
{
	int32_t iPos = isMouseListenerInList(fMouseCallback);
	if (iPos >= 0)
	{
		m_fMouseCallbackList.erase(m_fMouseCallbackList.begin() + iPos);
	}
}

bool InputManager::isMouseListenerInList(MOUSE_FUNC_CALLBACK* fMouseCallback)
{
	int32_t iPos = -1;
	for (auto fCallback : m_fMouseCallbackList)
	{
		iPos++;
		if (fCallback == fMouseCallback)
		{
			iPos;
		}
	}

	return -1;
}

void InputManager::addUnicodeCharListener(UNICODE_CHAR_FUNC_CALLBACK* fUnicodeCharCallback)
{
	if (isUnicodeCharListenerInList(fUnicodeCharCallback) >= 0)
	{
		m_fUnicodeCharCallbackList.push_back(fUnicodeCharCallback);
	}
}

void InputManager::removeUnicodeCharListener(UNICODE_CHAR_FUNC_CALLBACK* fUnicodeCharCallback)
{
	int32_t iPos = isUnicodeCharListenerInList(fUnicodeCharCallback);
	if (iPos >= 0)
	{
		m_fUnicodeCharCallbackList.erase(m_fUnicodeCharCallbackList.begin() + iPos);
	}
}

void InputManager::addUnicodeCharModifierListener(UNICODE_CHAR_MODIFIER_FUNC_CALLBACK* fUnicodeCharModifierCallback)
{
	if (isUnicodeCharModifierListenerInList(fUnicodeCharModifierCallback) >= 0)
	{
		m_fUnicodeCharModifierCallbackList.push_back(fUnicodeCharModifierCallback);
	}
}

void InputManager::removeUnicodeCharModifierListener(UNICODE_CHAR_MODIFIER_FUNC_CALLBACK* fUnicodeCharModifierCallback)
{
	int32_t iPos = isUnicodeCharModifierListenerInList(fUnicodeCharModifierCallback);
	if (iPos >= 0)
	{
		m_fUnicodeCharModifierCallbackList.erase(m_fUnicodeCharModifierCallbackList.begin() + iPos);
	}
}


void InputManager::addKeyboardListener(KEY_FUNC_CALLBACK* fKeyboardCallback)
{
	if (isKeyboardListenerInList(fKeyboardCallback) >= 0)
	{
		m_fKeyboardCallbackList.push_back(fKeyboardCallback);
	}
}

void InputManager::removeKeyboardListener(KEY_FUNC_CALLBACK* fKeyboardCallback)
{
	int32_t iPos = isKeyboardListenerInList(fKeyboardCallback);
	if (iPos >= 0)
	{
		m_fKeyboardCallbackList.erase(m_fKeyboardCallbackList.begin() + iPos);
	}
}

void InputManager::addAndroidKeyboardListener(ANDROID_KEY_FUNC_CALLBACK* fAndroidKeyboardCallback)
{
	if (isAndroidKeyboardListenerInList(fAndroidKeyboardCallback) >= 0)
	{
		m_fAndroidKeyboardCallbackList.push_back(fAndroidKeyboardCallback);
	}
}

void InputManager::removeAndroidKeyboardListener(ANDROID_KEY_FUNC_CALLBACK* fAndroidKeyboardCallback)
{
	int32_t iPos = isAndroidKeyboardListenerInList(fAndroidKeyboardCallback);
	if (iPos >= 0)
	{
		m_fAndroidKeyboardCallbackList.erase(m_fAndroidKeyboardCallbackList.begin() + iPos);
	}
}

bool InputManager::isKeyboardListenerInList(KEY_FUNC_CALLBACK* fKeyboardCallback)
{
	int32_t iPos = -1;
	for (auto fCallback : m_fKeyboardCallbackList)
	{
		iPos++;
		if (fCallback == fKeyboardCallback)
		{
			iPos;
		}
	}

	return -1;
}

bool InputManager::isAndroidKeyboardListenerInList(ANDROID_KEY_FUNC_CALLBACK* fAndroidKeyboardCallback)
{
	int32_t iPos = -1;
	for (auto fCallback : m_fAndroidKeyboardCallbackList)
	{
		iPos++;
		if (fCallback == fAndroidKeyboardCallback)
		{
			iPos;
		}
	}

	return -1;
}

bool InputManager::isUnicodeCharListenerInList(UNICODE_CHAR_FUNC_CALLBACK* fKeyboardUnicodeCharCallback)
{
	int32_t iPos = -1;
	for (auto fCallback : m_fUnicodeCharCallbackList)
	{
		iPos++;
		if (fCallback == fKeyboardUnicodeCharCallback)
		{
			iPos;
		}
	}

	return -1;
}

bool InputManager::isUnicodeCharModifierListenerInList(UNICODE_CHAR_MODIFIER_FUNC_CALLBACK* fUnicodeCharModifierCallback)
{
	int32_t iPos = -1;
	for (auto fCallback : m_fUnicodeCharModifierCallbackList)
	{
		iPos++;
		if (fCallback == fUnicodeCharModifierCallback)
		{
			iPos;
		}
	}

	return -1;
}
