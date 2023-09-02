#ifdef USE_YAGUI
#include "Engine/UI/WCanvas.h"
#include "Engine/UI/WComponentFactory.h"
#include "Engine/UI/WWidgetManager.h"

#define FR_RIGHT_GUTTER			3
#define FR_BOTTOM_GUTTER		3
#define FR_RESIZE_GAP			5

WCanvas::WCanvas()
: m_bResizable(true)
, m_ButtonWResizeLeft(NULL)
, m_ButtonWResizeRight(NULL)
, m_iResizingX(0)
{
}

WCanvas::~WCanvas() 
{
}

H_WND WCanvas::Create(	const char* lpClassName, 
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
	WCanvas* pWCanvas = new WCanvas();

	((WContainer*)pWCanvas)->Create(	lpClassName, 
										lpWindowName, 
										dwStyle, 
										x, 
										y, 
										width, 
										height, 
										hwndParent, 
										hMenu, 
										lpVoid,
										true, 
										true);

	return pWCanvas;
}

void WCanvas::onCreateEx(LPVOID lpVoid) 
{
	RectF wndRect(m_iLeft, m_iTop, getWidth(), getHeight()), destRect;
	m_DummyWidget = WWidgetManager::getWidget("CanvasWindow");
	///////////////////////////////////////////////////
	if (m_ButtonWResizeLeft == nullptr)
	{
		m_ButtonWResizeLeft = (WButton*)WWidgetManager::addComponentFromWidget(		"WButton",
																					"CanvasWindow", 
																					"WindowResizeLeft", 
																					wndRect, 
																					this, 
																					HMENU(ID_RESIZE_LEFT),
																					LPVOID("WindowResizeLeft"),
																					[=](CHILD* pChildWidget, RectF destRect, RectF& childRect)
																					{
																						childRect.X = destRect.X - m_iLeft;
																						childRect.Y = destRect.Y - m_iTop;
																						childRect.Width = destRect.Width;
																						childRect.Height = destRect.Height;
																					});
	}
	destRect = WWidgetManager::getDestinationRectFromWidgetForChild("CanvasWindow", "WindowResizeLeft", wndRect);
	m_ButtonWResizeLeft->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
	m_ButtonWResizeLeft->setSize(destRect.Width, destRect.Height);
	m_ButtonWResizeLeft->setPostRender(true);
	m_ButtonWResizeLeft->setMovable(false);
	m_ButtonWResizeLeft->setAsIntegral(true);

	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	if (m_ButtonWResizeRight == nullptr)
	{
		m_ButtonWResizeRight = (WButton*)WWidgetManager::addComponentFromWidget("WButton",
																				"CanvasWindow", 
																				"WindowResizeRight", 
																				wndRect, 
																				this, 
																				HMENU(ID_RESIZE_RIGHT),
																				LPVOID("WindowResizeRight"),
																				[=](CHILD* pChildWidget, RectF destRect, RectF& childRect)
																				{
																					childRect.X = destRect.X - m_iLeft;
																					childRect.Y = destRect.Y - m_iTop;
																					childRect.Width = destRect.Width;
																					childRect.Height = destRect.Height;
																				});
	}
	destRect = WWidgetManager::getDestinationRectFromWidgetForChild("CanvasWindow", "WindowResizeLeft", wndRect);
	m_ButtonWResizeRight->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
	m_ButtonWResizeRight->setSize(destRect.Width, destRect.Height);
	m_ButtonWResizeRight->setPostRender(true);
	m_ButtonWResizeRight->setMovable(false);
	m_ButtonWResizeRight->setAsIntegral(true);
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	updateClientRect("CanvasWindow", wndRect);
	///////////////////////////////////////////
}

void WCanvas::onUpdate(float deltaTimeMs) 
{

}

void WCanvas::onRender() 
{
	if(m_bShowBorder)
	{
		WWidgetManager* renderer =  WWidgetManager::getInstance();

		RectF thisWndRect(getLeft(), getTop(), getWidth(), getHeight());
			
		//CHILD* cWnd = 0;
		//cWnd = m_DummyWidget->getChild("TextArea");
		//renderer->renderChild(m_DummyWidget, cWnd, &thisWndRect);
		//renderer->renderClientArea(m_DummyWidget, 0, &thisWndRect);
	}
}

void WCanvas::postRenderEx() 
{
	WWidgetManager* renderer =  WWidgetManager::getInstance();
	RectF thisWndRect(getLeft(), getTop(), getWidth(), getHeight());

	for(size_t i = 0; i < m_pBaseSkinChilds.size(); i++) 
	{
		CHILD* pC = m_pBaseSkinChilds[i];
		renderer->renderChild(m_DummyWidget, pC, &thisWndRect);
	}
}

void WCanvas::resizeWidth(int iDiffWidth) 
{
	if(m_iResizingX == 1) 
	{
		if (iDiffWidth < 0 && getWidth() < m_DummyWidget->widgetSize.width)
		{
			return;
		}

		setWidth(getWidth() + iDiffWidth);
	}
	else 
	{
		if (iDiffWidth > 0 && getWidth() < m_DummyWidget->widgetSize.width)
		{
			return;
		}

		if((H_WND)this == WWidgetManager::getInstance()->GetWindow(0)) 
		{
			setLeft(iDiffWidth);
		}
		else 
		{
			setPositionX(getOffsetX() + iDiffWidth);
		}

		setWidth(getWidth() - iDiffWidth);
	}

	onResize(getWidth(), getHeight());
}

void WCanvas::resizeHeight(int iDiffHeight) 
{
	if (iDiffHeight < 0 && getHeight() < m_DummyWidget->widgetSize.height)
	{
		return;
	}

	setHeight(getHeight() + iDiffHeight);
	onResize(getWidth(), getHeight());
}

void WCanvas::onMouseDownEx(int x, int y, int iButton) 
{
}

void WCanvas::onMouseMoveEx(int mCode, int x, int y, int prevx, int prevy) 
{
	bool LEFT_MOUSE_DOWN = WWidgetManager::getInstance()->isMousePressed(GLFW_MOUSE_BUTTON_LEFT);

	// If click was on the title bar, drag window
	// If resizing, resize window

	// Drag window around:
	int diffX = (x - prevx);
	int diffY = (y - prevy);

	if(LEFT_MOUSE_DOWN)
	{
		if(m_pParent != NULL) 
		{
			setPosition(getOffsetX() + diffX, getOffsetY() + diffY);
		}
		else 
		if((H_WND)this == WWidgetManager::getInstance()->GetWindow(0)) 
		{
			setLeft(diffX);
			setTop(diffY);
		}
	}
}

void WCanvas::onMouseUpEx(int x, int y, int iButton) 
{
}

void WCanvas::onMouseWheelEx(WPARAM wParam, LPARAM lParam) 
{
}

bool WCanvas::isPtInside(int x, int y) 
{
	// x,y in local coordinates!

	// Check title bar and related gap!
	//TODO

	if (x < m_iLeft || x > m_iRight || y < m_iTop || y > m_iBottom)
	{
		return false;
	}

	return true;
}

void WCanvas::onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch(msg) 
	{
		case MOUSE_DOWN:
		{
			int buttonID = wParam;
			switch(wParam) 
			{
				case ID_RESIZE_LEFT:
				{
					m_iResizingX = -1;
					WWidgetManager::getInstance()->setCursor(IDC__SIZENESW);
				}
				break;
				case ID_RESIZE_RIGHT:
				{
					m_iResizingX = 1;
					WWidgetManager::getInstance()->setCursor(IDC__SIZENWSE);
				}
				break;
			}
		}
		break;
		case MOUSE_UP:
		{
			int buttonID = wParam;
			switch(wParam) 
			{
				case ID_RESIZE_LEFT:
				case ID_RESIZE_RIGHT:
				{
					m_iResizingX = 0;
				}
				break;
			}
		}
		break;
		case MOUSE_MOVE:
		{
			int code = (wParam>>16)&0xffff;
			int buttonID = (wParam&0xffff);

			DWORD* dwDiff = (DWORD*)(lParam);

			short dwDiffX = (*dwDiff >> 16) & 0xffff;
			short dwDiffY = (*dwDiff & 0xffff);
			switch(buttonID) 
			{
				case ID_RESIZE_LEFT:
					WWidgetManager::getInstance()->setCursor(IDC__SIZENESW);

					resizeWidth(dwDiffX);
					resizeHeight(dwDiffY);
				break;
				case ID_RESIZE_RIGHT:
					WWidgetManager::getInstance()->setCursor(IDC__SIZENWSE);

					resizeWidth(dwDiffX);
					resizeHeight(dwDiffY);
				break;
			}
		}
		break;
		case MOUSE_HOVER:
		{
			int code = (wParam>>16)&0xffff;
			int buttonID = (wParam&0xffff);

			DWORD* dwDiff = (DWORD*)(lParam);

			short dwDiffX = (*dwDiff >> 16) & 0xffff;
			short dwDiffY = (*dwDiff & 0xffff);
			switch(buttonID) 
			{
				case ID_RESIZE_LEFT:
					WWidgetManager::getInstance()->setCursor(IDC__SIZENESW);
				break;
				case ID_RESIZE_RIGHT:
					WWidgetManager::getInstance()->setCursor(IDC__SIZENWSE);
				break;
			}
		}
		break;
	}
}

void WCanvas::onResize(int width, int height) 
{
	updateComponentPosition();
	onCreateEx((LPVOID)0);
}

LRESULT WCanvas::OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch(msg) 
	{
		case WM__GETTEXTLENGTH:
		{
			return strlen(getWindowName());
		}
		break;
		case WM__GETTEXT:
		{
			std::string* str = (std::string*)wParam;
			*str = getWindowName();

			return (*str).size();
		}
		break;
		case WM__SETTEXT:
		{
			char* str = (char*)lParam;
			setWindowName(str);
			return WM__OKAY;
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
		case WM__GETSIZE:
		{
			DWORD* dwSize = (DWORD*)lParam;
			*dwSize |= (getWidth() & 0xffff);
			*dwSize |= ((getHeight() & 0xffff) << 16);

			return WM__OKAY;
		}
		break;
		case WM__SETSIZE:
		{
			DWORD dwPos = (DWORD)lParam;
			onResize((dwPos & 0xffff), ((dwPos >> 16) & 0xffff));
		}
		break;
	}

	return WContainer::OnSendMessage(msg, wParam, lParam);
}
#endif