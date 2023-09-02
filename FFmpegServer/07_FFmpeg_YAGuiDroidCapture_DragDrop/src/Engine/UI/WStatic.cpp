#ifdef USE_YAGUI
#include "Engine/UI/WStatic.h"
#include "Engine/UI/WContainer.h"
#include "Engine/UI/WWidgetManager.h"

WStatic::WStatic() 
{
	setComponentAsChild(false);
}

H_WND WStatic::Create(	const char* lpClassName, 
						const char* lpWindowName, 
						DWORD dwStyle, 
						int x, 
						int y, 
						int width, 
						int height, 
						H_WND hwndParent, 
						HMENU hMenu, 
						LPVOID lpVoid
) {
	WStatic* pWStatic = new WStatic();
	((WContainer*)pWStatic)->Create(	lpClassName, 
										lpWindowName, 
										dwStyle, 
										x, 
										y, 
										width, 
										height, 
										hwndParent, 
										hMenu, 
										lpVoid,
										false, 
										true);

	return pWStatic;
}

void WStatic::onCreateEx(LPVOID lpVoid) 
{
	m_pText= getWindowName();

	m_State = NORMAL;
}

void WStatic::onUpdate(float deltaTimeMs) 
{

}

void WStatic::frameRender() 
{
	WComponent::frameRender();
}

void WStatic::deactivate() 
{
	m_State = INACTIVE;
}

void WStatic::activate() 
{
	m_State = NORMAL;
}

void WStatic::onRender() 
{
	if(NOT m_pText.empty()) 
	{
		WWidgetManager::setColor(0, 0, 0, 1.0f);
		WWidgetManager::drawStringFont(m_pText.c_str(), getLeft(), getTop()+(getHeight()>>1)-6, 0);
		WWidgetManager::resetColor();
	}
}

void WStatic::onMouseDownEx(int x, int y, int iButton) 
{
	if(m_pParent) 
	{
		m_pParent->onMessage((H_WND)this, MOUSE_DOWN, getComponentID(), 0);
	}
}

void WStatic::onMouseUpEx(int x, int y, int iButton) 
{
	if(m_pParent) 
	{
		m_pParent->onMessage((H_WND)this, MOUSE_UP, getComponentID(), 0);
	}
}

void WStatic::onMouseMoveEx(int mCode, int x, int y, int prevX, int prevY) 
{
	if(m_pParent) 
	{
		int dwDiffX = (-(prevX-x) & 0xffff);
		int dwDiffY = (-(prevY-y) & 0xffff);
		DWORD dwDiff = (dwDiffX <<16) | dwDiffY;
		m_pParent->onMessage((H_WND)this, MOUSE_MOVE, (mCode<<16)|(getComponentID()), (LPARAM)&dwDiff);
	}
}

void WStatic::onMouseWheelEx(WPARAM wParam, LPARAM lParam) 
{

}

void WStatic::onKeyBDownEx(unsigned int iVirtualKeycode, unsigned short ch) 
{
	
}

void WStatic::onKeyBUpEx(unsigned int iVirtualKeycode, unsigned short ch) 
{

}

void WStatic::onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{

}

LRESULT WStatic::OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch(msg) 
	{
		case WM__GETTEXTLENGTH:
		{
			return m_pText.size();
		}
		break;
		case WM__GETTEXT:
		{
			std::string* str = (std::string*)wParam;
			*str = m_pText.c_str();

			return m_pText.size();
		}
		break;
		case WM__SETTEXT:
		{
			m_pText = (char*)lParam;
			return WM__OKAY;
		}
		break;
	}

	return -1;
}

WStatic::~WStatic() 
{
}
#endif