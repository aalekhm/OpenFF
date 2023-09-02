#pragma once

#ifdef USE_YAGUI

#include "Engine/UI/WComponent.h"
#include "Engine/UI/widgetdef.h"

struct WStatic : public WComponent 
{
	private:
										WStatic();
										~WStatic();
		enum StaticState
		{
			NORMAL,
			INACTIVE
		};
		
		std::string						m_pText;
		StaticState						m_State;
		
		virtual void					onCreateEx(LPVOID lpVoid);
		virtual void					frameRender();
		virtual void					onUpdate(float deltaTimeMs);
		virtual void					onRender();
		virtual void					onMouseDownEx(int x, int y, int iButton);
		virtual void					onMouseUpEx(int x, int y, int iButton);
		virtual void					onMouseMoveEx(int mCode, int x, int y, int prevX, int prevY);
		virtual void					onMouseWheelEx(WPARAM wParam, LPARAM lParam);
		virtual void					onKeyBDownEx(unsigned int iVirtualKeycode, unsigned short ch);
		virtual void					onKeyBUpEx(unsigned int iVirtualKeycode, unsigned short ch);
		virtual void					onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	public:		
		static H_WND __stdcall			Create(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int width, int height, H_WND hwndParent, HMENU hMenu, LPVOID lpParam);

		LRESULT							OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam);

		void							setText(const char* sText) { m_pText = sText; }
		void							setState(StaticState bs) { m_State = bs; }
		StaticState						getState() { return m_State; }
		void							deactivate();
		void							activate();
};
#endif