#pragma once

#ifdef USE_YAGUI
#include "Engine/UI/WComponent.h"
#include "Engine/UI/widgetdef.h"

struct WCheckbox : public WComponent 
{
	private:
		enum CheckButtonState 
		{
			NORMAL,
			HIGHLIGHTED,
			PUSHED,
			INACTIVE
		};
		
		std::string				m_pTitle;
		CheckButtonState		m_State;
		bool					m_bChecked;

		std::string				m_pCBCheckedStateNameNormal;
		std::string				m_pCBCheckedStateNameHighlighted;
		std::string				m_pCBCheckedStateNamePushed;
		std::string				m_pCBCheckedStateNameDisabled;

		std::string				m_pCBUnCheckedStateNameNormal;
		std::string				m_pCBUnCheckedStateNameHighlighted;
		std::string				m_pCBUnCheckedStateNamePushed;
		std::string				m_pCBUnCheckedStateNameDisabled;
						WCheckbox();
						~WCheckbox();
	public:
		static H_WND __stdcall	Create(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int width, int height, H_WND hwndParent, HMENU hMenu, LPVOID lpParam);
		
		LRESULT					OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam);

		void					setTitle(const char* sTitle) { m_pTitle = sTitle; }
		void					setChecked(bool bCheck) { m_bChecked = bCheck; }
		bool					isChecked() { return m_bChecked; }

		void					activate();
		void					deactivate();
		
		virtual void			onCreateEx(LPVOID lpVoid);
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
		
		static unsigned int		CHECKBOX_TEXT_HEIGHT;
};
#endif