#pragma once
#include "Common/Singleton.h"
#include <bitset>
#include <vector>
#include <functional>

typedef std::function<void(int32_t iAction, int32_t iKey, double dXPos, double dYPos)>		MOUSE_FUNC_CALLBACK;
typedef std::function<void(int32_t iAction, int32_t iKey)>									KEY_FUNC_CALLBACK;
typedef std::function<void(int32_t iAction, int32_t iKey, int32_t iAndroidMetaModifier)>	ANDROID_KEY_FUNC_CALLBACK;

class InputManager : public Singleton<InputManager>
{
	public:
		friend class GameEngine;

		bool				isKeyPressed(int32_t iKey);

		bool				isMousePressed(int32_t iKey);
		void				getMousePos(double& xPos, double& yPos);
		double				getMouseX();
		double				getMouseY();

		void				addMouseListener(MOUSE_FUNC_CALLBACK* fMouseCallbackList);
		void				removeMouseListener(MOUSE_FUNC_CALLBACK* fMouseCallbackList);

		void				addKeyboardListener(KEY_FUNC_CALLBACK* fKeyboardCallback);
		void				removeKeyboardListener(KEY_FUNC_CALLBACK* fKeyboardCallback);

		void				addAndroidKeyboardListener(ANDROID_KEY_FUNC_CALLBACK* fKeyboardCallback);
		void				removeAndroidKeyboardListener(ANDROID_KEY_FUNC_CALLBACK* fKeyboardCallback);
	protected:
	private:
		void				onKeyPressed(uint32_t iKey);
		void				onKeyReleased(uint32_t iKey);

		void				onMousePressed(uint32_t iKey);
		void				onMouseReleased(uint32_t iKey);

		void				onMouseMoved(double xPos, double yPos);

		bool				isMouseListenerInList(MOUSE_FUNC_CALLBACK* fMouseCallbackList);
		bool				isKeyboardListenerInList(KEY_FUNC_CALLBACK* fKeyboardCallback);
		bool				isAndroidKeyboardListenerInList(ANDROID_KEY_FUNC_CALLBACK* fAndroidKeyboardCallback);

		int32_t				mapToAndroidKeyCode(int32_t iAscii);
		bool				alterAndroidExtraMetaModifiers(bool bSet, int32_t iAsciiKeyCode);
		bool				alterAndroidMetaModifiers(bool bSet, int32_t iAsciiKeyCode);

		bool				isNumLock();

		std::vector<MOUSE_FUNC_CALLBACK*>			m_fMouseCallbackList;
		std::vector<KEY_FUNC_CALLBACK*>				m_fKeyboardCallbackList;
		std::vector<ANDROID_KEY_FUNC_CALLBACK*>		m_fAndroidKeyboardCallbackList;

		std::bitset<512>	m_KeyboardBits;
		std::bitset<8>		m_MouseBits;
		std::bitset<32>		m_AndroidMetaBits;

		double				m_dMouseX;
		double				m_dMouseY;

		int32_t				m_iMouseKeyDown = -1;
};