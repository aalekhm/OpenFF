#pragma once
#include "Common/Singleton.h"
#include <bitset>
#include <vector>
#include <functional>

typedef std::function<void(int32_t iAction, int32_t iKey, double dXPos, double dYPos)> MOUSE_FUNC_CALLBACK;

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
	protected:
	private:
		void				onKeyPressed(uint32_t iKey);
		void				onKeyReleased(uint32_t iKey);

		void				onMousePressed(uint32_t iKey);
		void				onMouseReleased(uint32_t iKey);

		void				onMouseMoved(double xPos, double yPos);

		bool				isMouseListenerInList(MOUSE_FUNC_CALLBACK* fMouseCallbackList);

		std::vector<MOUSE_FUNC_CALLBACK*>	m_fMouseCallbackList;

		std::bitset<512>	m_KeyboardBits;
		std::bitset<8>		m_MouseBits;

		double				m_dMouseX;
		double				m_dMouseY;

		int32_t				m_iMouseKeyDown = -1;
};