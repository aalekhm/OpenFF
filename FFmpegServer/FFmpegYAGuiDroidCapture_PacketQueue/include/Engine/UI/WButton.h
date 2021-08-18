#pragma once
#ifdef USE_YAGUI

#include "Engine/UI/WComponent.h"
#include "Engine/UI/widgetdef.h"

struct WButton : public WComponent 
{
	private:
		enum ButtonState 
		{
			NORMAL,
			HOVER,
			PUSHED,
			INACTIVE
		};
		
		std::string				m_pTitle;
		ButtonState				m_State;

		std::string				m_pButtonStateNameNormal;
		std::string				m_pButtonStateNameHighlighted;
		std::string				m_pButtonStateNamePushed;
		std::string				m_pButtonStateNameDisabled;

		virtual void			onCreateEx(LPVOID lpVoid);
		virtual void			frameRender();
		virtual void			onUpdate(float deltaTimeMs);
		virtual void			onRender();

		virtual void			onMouseDownEx(int x, int y, int iButton);
		virtual void			onMouseUpEx(int x, int y, int iButton);
		virtual void			onMouseEnterEx(int mCode, int x, int y, int prevX, int prevY);
		virtual void			onMouseHoverEx(int mCode, int x, int y, int prevX, int prevY);
		virtual void			onMouseLeaveEx(int mCode, int x, int y, int prevX, int prevY);
		virtual void			onMouseMoveEx(int mCode, int x, int y, int prevX, int prevY);
		virtual void			onMouseWheelEx(WPARAM wParam, LPARAM lParam);

		virtual void			onKeyBDownEx(unsigned int iVirtualKeycode, unsigned short ch);
		virtual void			onKeyBUpEx(unsigned int iVirtualKeycode, unsigned short ch);
		virtual void			onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		WButton();
		~WButton();
	public:
		static H_WND __stdcall	Create(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int width, int height, H_WND hwndParent, HMENU hMenu, LPVOID lpParam);
	
		LRESULT					OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam);

		void					setTitle(const char* sTitle) { m_pTitle = sTitle; }
		void					setState(ButtonState bs) { m_State = bs; }
		ButtonState				getState() { return m_State; }
		void					deactivate();
		void					activate();

		static unsigned int		BUTTON_TEXT_HEIGHT;
};
#endif