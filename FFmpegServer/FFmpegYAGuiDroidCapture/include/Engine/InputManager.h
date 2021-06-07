#pragma once
#include "Common/Singleton.h"
#include <bitset>
#include <vector>
#include <functional>

typedef std::function<void(int32_t iAction, int32_t iKey, double dXPos, double dYPos)>					MOUSE_FUNC_CALLBACK;
typedef std::function<void(int32_t iAction, int32_t iKey, int32_t iScanCode, int32_t iModifierKeys)>	KEY_FUNC_CALLBACK;
typedef std::function<void(uint32_t iUnicodeChar)>														UNICODE_CHAR_FUNC_CALLBACK;
typedef std::function<void(uint32_t iUnicodeChar, int32_t iModifier)>									UNICODE_CHAR_MODIFIER_FUNC_CALLBACK;
typedef std::function<void(int32_t iAction, int32_t iKey, int32_t iAndroidMetaModifier)>				ANDROID_KEY_FUNC_CALLBACK;

class InputManager : public Singleton<InputManager>
{
	public:
		friend class GameEngine;

		bool				isKeyPressed(int32_t iKey);

		bool				isMousePressed(int32_t iKey);
		bool				isMouseReleased(int32_t iKey);
		void				getMousePos(double& xPos, double& yPos);
		double				getMouseX();
		double				getMouseY();

		void				addMouseListener(MOUSE_FUNC_CALLBACK* fMouseCallbackList);
		void				removeMouseListener(MOUSE_FUNC_CALLBACK* fMouseCallbackList);

		void				addKeyboardListener(KEY_FUNC_CALLBACK* fKeyboardCallback);
		void				removeKeyboardListener(KEY_FUNC_CALLBACK* fKeyboardCallback);

		void				addAndroidKeyboardListener(ANDROID_KEY_FUNC_CALLBACK* fKeyboardCallback);
		void				removeAndroidKeyboardListener(ANDROID_KEY_FUNC_CALLBACK* fKeyboardCallback);

		void				addUnicodeCharListener(UNICODE_CHAR_FUNC_CALLBACK* fKeyboardUnicodeCharCallback);
		void				removeUnicodeCharListener(UNICODE_CHAR_FUNC_CALLBACK* fKeyboardUnicodeCharCallback);

		void				addUnicodeCharModifierListener(UNICODE_CHAR_MODIFIER_FUNC_CALLBACK* fUnicodeCharModifierCallback);
		void				removeUnicodeCharModifierListener(UNICODE_CHAR_MODIFIER_FUNC_CALLBACK* fUnicodeCharModifierCallback);

		bool				isModifierKeyDown(int32_t iModifierKey);

		void				onMousePressed(uint32_t iKey);
		void				onMouseReleased(uint32_t iKey);
		void				onMouseMoved(double xPos, double yPos);
	protected:
	private:
		void				onUnicodeChar(unsigned int iUnicodeChar);
		void				onUnicodeCharModifier(unsigned int iUnicodeChar, int iModifier);
		void				onKeyPressed(int iKey, int iScanCode, int iAction, int iModifierKeys);
		void				onKeyReleased(int iKey, int iScanCode, int iAction, int iModifierKeys);

		bool				isMouseListenerInList(MOUSE_FUNC_CALLBACK* fMouseCallbackList);
		bool				isKeyboardListenerInList(KEY_FUNC_CALLBACK* fKeyboardCallback);
		bool				isAndroidKeyboardListenerInList(ANDROID_KEY_FUNC_CALLBACK* fAndroidKeyboardCallback);
		bool				isUnicodeCharListenerInList(UNICODE_CHAR_FUNC_CALLBACK* fUnicodeCharCallback);
		bool				isUnicodeCharModifierListenerInList(UNICODE_CHAR_MODIFIER_FUNC_CALLBACK* fUnicodeCharModifierCallback);

		int32_t				mapToAndroidKeyCode(int32_t iAscii);
		bool				alterAndroidExtraMetaModifiers(bool bSet, int32_t iAsciiKeyCode);
		bool				alterAndroidMetaModifiers(bool bSet, int32_t iAsciiKeyCode);

		bool				isNumLock();

		std::vector<MOUSE_FUNC_CALLBACK*>					m_fMouseCallbackList;
		std::vector<KEY_FUNC_CALLBACK*>						m_fKeyboardCallbackList;
		std::vector<UNICODE_CHAR_FUNC_CALLBACK*>			m_fUnicodeCharCallbackList;
		std::vector<UNICODE_CHAR_MODIFIER_FUNC_CALLBACK*>	m_fUnicodeCharModifierCallbackList;
		std::vector<ANDROID_KEY_FUNC_CALLBACK*>				m_fAndroidKeyboardCallbackList;


		std::bitset<512>	m_KeyboardBits;
		std::bitset<8>		m_MouseBitsPress;
		std::bitset<8>		m_MouseBitsRelease;
		std::bitset<32>		m_AndroidMetaBits;
		uint16_t			m_iModifierKeys;

		double				m_dMouseX;
		double				m_dMouseY;

		int32_t				m_iMouseKeyDown = -1;
};