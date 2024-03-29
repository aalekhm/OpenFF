#pragma once

#ifdef USE_YAGUI
#include "Engine/UI/UIDefines.h"
#include "Engine/UI/widgetdef.h"
#include "Engine/UI/WContainer.h"
#include "Engine/UI/WButton.h"

struct WCanvas : public WContainer 
{
	friend class Node;
	friend class Camera;
	
	public:
		static H_WND __stdcall		Create(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int width, int height, H_WND hwndParent, HMENU hMenu, LPVOID lpParam);
		LRESULT						OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam);

		//Camera*					getCamera();
	private:
		bool						m_bResizable;
		
									WCanvas();
		virtual						~WCanvas();

		void						setResizable(bool bResizable)	{ m_bResizable = bResizable; }
		void						setBorderVisibility(bool bHasBorder)	{ m_bShowBorder = bHasBorder; }
		bool						getBorderVisibility()					{ return m_bShowBorder; }
		
		void						onResize(int width, int height);
		void						resizeWidth(int iDiffWidth);
		void						resizeHeight(int iDiffHeight);

		virtual void				onCreateEx(LPVOID lpVoid);
		virtual void				onUpdate(float deltaTimeMs);
		virtual void				onRender();
		virtual void				postRenderEx();

		virtual void				onMouseDownEx(int x, int y, int iButton);
		virtual void				onMouseMoveEx(int mCode, int x, int y, int prevx, int prevy);
		virtual void				onMouseUpEx(int x, int y, int iButton);
		virtual void				onMouseWheelEx(WPARAM wParam, LPARAM lParam);

		virtual void				onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		virtual bool				isPtInside(int x, int y);
		
		WIDGET*						m_DummyWidget;

		WButton*					m_ButtonWResizeLeft;
		WButton*					m_ButtonWResizeRight;

		int							m_minX;
		int							m_maxX;
		int							m_minY;
		int							m_maxY;

		bool						m_bShowBorder;
		int							m_iMaxWidthPixels;
		int							m_iMaxHeightPixels;

		int							m_iResizingX;
};
#endif