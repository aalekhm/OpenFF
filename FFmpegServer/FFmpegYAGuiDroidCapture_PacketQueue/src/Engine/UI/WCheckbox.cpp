#ifdef USE_YAGUI
#include "Engine/UI/WCheckbox.h"
#include "Engine/UI/WContainer.h"
#include "Engine/UI/WWidgetManager.h"

#define CHECKBOX_STRING_STARTX		25
#define CHECKBOX_TOP_GUTTER			2

unsigned int WCheckbox::CHECKBOX_TEXT_HEIGHT;

WCheckbox::WCheckbox() 
{
}

H_WND WCheckbox::Create(	const char* lpClassName, 
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
	WCheckbox* pWCheckbox = new WCheckbox();
	((WContainer*)pWCheckbox)->Create(	lpClassName, 
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

	return pWCheckbox;
}

void WCheckbox::onCreateEx(LPVOID lpVoid) 
{
	m_pTitle = getWindowName();

	CHECKBOX_TEXT_HEIGHT = WWidgetManager::CHARACTER_HEIGHT + (CHECKBOX_TOP_GUTTER << 1);

	m_bChecked = false;
	m_State = NORMAL;

	m_pCBCheckedStateNameNormal = "CheckBox_Normal_Checked";

	m_pCBCheckedStateNameHighlighted = "CheckBox_Highlighted_Checked";

	m_pCBCheckedStateNamePushed = "CheckBox_Pushed_Checked";

	m_pCBCheckedStateNameDisabled = "CheckBox_Inactive_Checked";

	m_pCBUnCheckedStateNameNormal = "CheckBox_Normal";

	m_pCBUnCheckedStateNameHighlighted = "CheckBox_Highlighted";

	m_pCBUnCheckedStateNamePushed = "CheckBox_Pushed";

	m_pCBUnCheckedStateNameDisabled = "CheckBox_Inactive";
}

void WCheckbox::deactivate() 
{
	m_State = INACTIVE;
}

void WCheckbox::activate() 
{
	m_State = NORMAL;
}

void WCheckbox::onUpdate(float deltaTimeMs) 
{
}

void WCheckbox::onRender() 
{	
	std::string sCheckButtonStateName;

	if(m_bChecked) 
	{
		switch(m_State) 
		{
			case NORMAL:
				sCheckButtonStateName = m_pCBCheckedStateNameNormal;
			break;
			case HIGHLIGHTED:
				sCheckButtonStateName = m_pCBCheckedStateNameHighlighted;
			break;
			case PUSHED:
				sCheckButtonStateName = m_pCBCheckedStateNamePushed;
			break;
			case INACTIVE:
				sCheckButtonStateName = m_pCBCheckedStateNameDisabled;
			break;
		}
	}
	else 
	{
		switch(m_State) 
		{
			case NORMAL:
				sCheckButtonStateName = m_pCBUnCheckedStateNameNormal;
			break;
			case HIGHLIGHTED:
				sCheckButtonStateName = m_pCBUnCheckedStateNameHighlighted;
			break;
			case PUSHED:
				sCheckButtonStateName = m_pCBUnCheckedStateNamePushed;
			break;
			case INACTIVE:
				sCheckButtonStateName = m_pCBUnCheckedStateNameDisabled;
			break;
		}
	}
	
	WIDGET* widget = WWidgetManager::getWidget(sCheckButtonStateName.c_str());
	RectF renderRect(getLeft(), getTop() + (CHECKBOX_TEXT_HEIGHT /2) - (widget->widgetSize.height / 2), getWidth(), getHeight());
	WWidgetManager::renderWidget(sCheckButtonStateName.c_str(), &renderRect);
	
	WWidgetManager::setColor(0, 0, 0, 1.0f);//ABGR
	WWidgetManager::drawStringFont(m_pTitle.c_str(), getLeft() + CHECKBOX_STRING_STARTX, getTop() + CHECKBOX_TOP_GUTTER, 0);
	if (m_bChecked)//Make it look BOLD
	{
		WWidgetManager::drawStringFont(m_pTitle.c_str(), getLeft() + CHECKBOX_STRING_STARTX + 1, getTop() + CHECKBOX_TOP_GUTTER, 0);
	}

	WWidgetManager::resetColor();
}

void WCheckbox::onMouseDownEx(int x, int y, int iButton) 
{
	if (m_State == INACTIVE)
	{
		return;
	}

	m_State = PUSHED;
}

void WCheckbox::onMouseUpEx(int x, int y, int iButton) 
{
	if (m_State == INACTIVE)
	{
		return;
	}
	
	if(isPointInside(x, y)) 
	{
		m_bChecked = !m_bChecked;	
		WWidgetManager::onEvent(this, m_bChecked ? WM_CBX_CHECKED : WM_CBX_UNCHECKED, getComponentID(), 0);
	}

	m_State = NORMAL;
}

void WCheckbox::onMouseEnterEx(int mCode, int x, int y, int prevX, int prevY) 
{
	m_State = HIGHLIGHTED;
}

void WCheckbox::onMouseHoverEx(int mCode, int x, int y, int prevX, int prevY) 
{
	m_State = HIGHLIGHTED;
}

void WCheckbox::onMouseLeaveEx(int mCode, int x, int y, int prevX, int prevY) 
{
	m_State = NORMAL;
}

void WCheckbox::onMouseMoveEx(int mCode, int x, int y, int prevX, int prevY) 
{
	if(	!(	m_State == INACTIVE
			||
			m_State == PUSHED
		)
	)
		m_State = HIGHLIGHTED;
}

void WCheckbox::onMouseWheelEx(WPARAM wParam, LPARAM lParam) 
{

}

void WCheckbox::onKeyBDownEx(unsigned int iVirtualKeycode, unsigned short ch) 
{
	
}

void WCheckbox::onKeyBUpEx(unsigned int iVirtualKeycode, unsigned short ch) 
{

}

void WCheckbox::onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{

}

LRESULT WCheckbox::OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch(msg) 
	{
		case WM__GETTEXTLENGTH:
		{
			return m_pTitle.size();
		}
		break;
		case WM__GETTEXT:
		{
			std::string* str = (std::string*)wParam;
			*str = m_pTitle.c_str();

			return m_pTitle.size();
		}
		break;
		case WM__SETTEXT:
		{
			char* str = (char*)lParam;
			m_pTitle = str;

			return 1;
		}
		break;
		case WM__GETRECT:
		{
			Rect* listRect = (Rect*)lParam;
			listRect->X = getLeft();
			listRect->Y = getTop();
			listRect->Width = getWidth();
			listRect->Height = getHeight();

			return WM__OKAY;
		}
		break;
		case WM__GETPOS:
		{
			DWORD* dwPos = (DWORD*)lParam;
			*dwPos |= (getLeft() & 0xffff);
			*dwPos |= ((getTop() & 0xffff) << 16);

			return WM__OKAY;
		}
		break;
		case WM__SETPOS:
		{
			DWORD dwPos = (DWORD)lParam;
			setPosition((dwPos & 0xffff), ((dwPos >> 16) & 0xffff));
			return WM__OKAY;
		}
		break;
		case BM__CLICK:
		{
			Rect* bounds = (Rect*)lParam;
			WWidgetManager* pWWidgetManager = WWidgetManager::getInstance();
			{
				pWWidgetManager->simulateMouseMove(bounds->X + (bounds->Width >> 1), bounds->Y + (bounds->Height >> 1));
				pWWidgetManager->simulateMousePress(GLFW_MOUSE_BUTTON_LEFT);
				pWWidgetManager->simulateMouseRelease(GLFW_MOUSE_BUTTON_LEFT);
			}

			return WM__OKAY;
		}
		break;
		case BM__ENABLE:
		{
			activate();
			return 1;
		}
		break;
		case BM__DISABLE:
		{
			deactivate();
			return 1;
		}
		break;
		case BM__GET_STATE:
		{
			return (m_State == INACTIVE)?0:1;
		}
		break;
	}

	return -1;
}

WCheckbox::~WCheckbox() 
{
}
#endif