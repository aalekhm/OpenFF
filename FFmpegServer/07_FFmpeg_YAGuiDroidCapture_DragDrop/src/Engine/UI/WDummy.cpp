#ifdef USE_YAGUI
#include "Engine/UI/WDummy.h"
#include "Engine/UI/WComponentFactory.h"
#include "Engine/UI/WWidgetManager.h"

#define FR_RIGHT_GUTTER 3
#define FR_BOTTOM_GUTTER 3
#define FR_RESIZE_GAP 5

WDummy::WDummy() 
: m_bResizable(true)

, m_sbVertical(NULL)
, m_sbHorizontal(NULL)

, m_ButtonWResizeLeft(NULL)
, m_ButtonWResizeRight(NULL)

, m_iResizingX(0)
{
}

WDummy::~WDummy() 
{

}

H_WND WDummy::Create(		const char* lpClassName, 
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
	WDummy* pWDummy = new WDummy();

	((WContainer*)pWDummy)->Create(	lpClassName, 
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

	return pWDummy;
}

void WDummy::onCreateEx(LPVOID lpVoid) 
{
	H_WND hWnd = NULL;
	RectF wndRect(m_iLeft, m_iTop, getWidth(), getHeight()), destRect;

	m_bHasScrollBars = (bool)lpVoid;
	m_DummyWidget = WWidgetManager::getWidget("DummyWindow");
	///////////////////////////////////////////////////
	
	if (m_sbVertical == nullptr)
	{
		m_sbVertical = (WScrollbar*)WWidgetManager::addComponentFromWidget(	"WScrollbar",
																			"DummyWindow", 
																			"VScroll", 
																			wndRect, 
																			this, 
																			HMENU(ID_VERTICAL_SCROLLBAR),
																			(LPVOID)1,
																			[=](CHILD* pChildWidget, RectF destRect, RectF& childRect)
																			{
																				childRect.X = destRect.X - m_iLeft;
																				childRect.Y = destRect.Y - m_iTop;
																				childRect.Width = destRect.Width;
																				childRect.Height = destRect.Height;
																			});
	}
	destRect = WWidgetManager::getDestinationRectFromWidgetForChild("DummyWindow", "VScroll", wndRect);
	{
		m_sbVertical->hasBG(true);
		m_sbVertical->setPostRender(true);
		m_sbVertical->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
		m_sbVertical->setSize(destRect.Width, destRect.Height);

		m_iMaxVScrollbarHeight = destRect.Height;
		m_minY = destRect.Y;
		m_maxY = destRect.Y + destRect.Height;
	}
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	if (m_sbHorizontal == nullptr)
	{
		m_sbHorizontal = (WScrollbar*)WWidgetManager::addComponentFromWidget(	"WScrollbar",
																				"DummyWindow", 
																				"HScroll", 
																				wndRect, 
																				this, 
																				HMENU(ID_HORIZONTAL_SCROLLBAR),
																				(LPVOID)0,
																				[=](CHILD* pChildWidget, RectF destRect, RectF& childRect)
																				{
																					childRect.X = destRect.X - m_iLeft;
																					childRect.Y = destRect.Y - m_iTop;
																					childRect.Width = destRect.Width;
																					childRect.Height = destRect.Height;
																				});
	}
	destRect = WWidgetManager::getDestinationRectFromWidgetForChild("DummyWindow", "HScroll", wndRect);
	{
		m_sbHorizontal->hasBG(true);
		m_sbHorizontal->setPostRender(true);
		m_sbHorizontal->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
		m_sbHorizontal->setSize(destRect.Width, destRect.Height);

		m_iMaxHScrollbarWidth = destRect.Width;
		m_minX = destRect.X;
		m_maxX = destRect.X + destRect.Width;
	}
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	if (m_ButtonWResizeLeft == nullptr)
	{
		m_ButtonWResizeLeft = (WButton*)WWidgetManager::addComponentFromWidget(		"WButton",
																					"DummyWindow", 
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
	destRect = WWidgetManager::getDestinationRectFromWidgetForChild("DummyWindow", "WindowResizeLeft", wndRect);
	{
		m_ButtonWResizeLeft->setPostRender(true);
		m_ButtonWResizeLeft->setMovable(false);
		m_ButtonWResizeLeft->setAsIntegral(true);
		m_ButtonWResizeLeft->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
		m_ButtonWResizeLeft->setSize(destRect.Width, destRect.Height);
	}
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	if (m_ButtonWResizeRight == nullptr)
	{
		m_ButtonWResizeRight = (WButton*)WWidgetManager::addComponentFromWidget("WButton",
																				"DummyWindow", 
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
	destRect = WWidgetManager::getDestinationRectFromWidgetForChild("DummyWindow", "WindowResizeRight", wndRect);
	{
		m_ButtonWResizeRight->setPostRender(true);
		m_ButtonWResizeRight->setMovable(false);
		m_ButtonWResizeRight->setAsIntegral(true);
		m_ButtonWResizeRight->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
		m_ButtonWResizeRight->setSize(destRect.Width, destRect.Height);
	}
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	updateClientRect("DummyWindow", wndRect);
	///////////////////////////////////////////
}

void WDummy::onUpdate(float deltaTimeMs) 
{
	m_iMaxWidthPixels = m_iMaxHeightPixels = 0;
	if(m_pChildren.size() > 0) 
	{
		for(int i = 0; i < m_pChildren.size(); i++) 
		{
			WComponent* comp = (WComponent*)m_pChildren[i];
			if(comp->isComponentAChild() && !comp->isIntegral()) 
			{
				int iRight = comp->getOffsetX() + comp->getWidth();
				if(iRight > m_iMaxWidthPixels)
					m_iMaxWidthPixels = iRight;

				int iBottom = comp->getOffsetY() + comp->getHeight();
				if(iBottom > m_iMaxHeightPixels)
					m_iMaxHeightPixels = iBottom;
			}
		}

		setVScrollbarLength();
		setHScrollbarLength();
	}

	bool bVertSBVisibility = (m_iMaxHeightPixels > m_ClientRect.Height);
	m_sbVertical->setVisible(bVertSBVisibility);
	if(!bVertSBVisibility) 
	{
		m_ClientRect.Width = m_iClientRectW + m_sbVertical->getWidth() - FR_RIGHT_GUTTER;
		m_sbHorizontal->setWidth(m_iMaxHScrollbarWidth + m_sbVertical->getWidth());
	}
	else 
	{
		m_ClientRect.Width = m_iClientRectW - FR_RIGHT_GUTTER;
		m_sbHorizontal->setWidth(m_iMaxHScrollbarWidth);
	}

	bool bHoriSBVisibility = (m_iMaxWidthPixels > m_ClientRect.Width);
	m_sbHorizontal->setVisible(bHoriSBVisibility);
	if(!bHoriSBVisibility) 
	{
		m_ClientRect.Height = m_iClientRectH + m_sbHorizontal->getHeight() - FR_BOTTOM_GUTTER;
		m_sbVertical->setHeight(m_iMaxVScrollbarHeight + m_sbHorizontal->getHeight());
	}
	else 
	{
		m_ClientRect.Height = m_iClientRectH - FR_BOTTOM_GUTTER;
		m_sbVertical->setHeight(m_iMaxVScrollbarHeight);
	}
}

void WDummy::setVScrollbarLength() 
{
	float _part = m_ClientRect.Height;
	float _total = m_iMaxHeightPixels;

	if(_total > 0) 
	{
		float _percentage = (_part / _total) * 100;

		if(_percentage <= 100) 
		{
			m_sbVertical->setLength(_percentage);
		}
	}
}

void WDummy::setHScrollbarLength() 
{
	float _part = m_ClientRect.Width;
	float _total = m_iMaxWidthPixels;
	float _percentage = (_part / _total) * 100;

	if(_percentage <= 100)
		m_sbHorizontal->setLength(_percentage);
}

void WDummy::updateVBarPosition() 
{
	float _part = abs(m_iMainY);
	float _total = m_iMaxHeightPixels;
	float _percentage = (_part / _total) * 100;

	m_sbVertical->setCursorPositionInPercent(_percentage);
}

void WDummy::updateHBarPosition() 
{
	float _part = abs(m_iMainX);
	float _total = m_iMaxWidthPixels;
	float _percentage = (_part / _total) * 100;

	m_sbHorizontal->setCursorPositionInPercent(_percentage);
}

void WDummy::onRender() 
{
	if(getBorderVisibility()) 
	{
		WWidgetManager* renderer =  WWidgetManager::getInstance();

		RectF thisWndRect(getLeft(), getTop(), getWidth(), getHeight());
			
		CHILD* cWnd = 0;
		cWnd = m_DummyWidget->getChild("TextArea");

		renderer->renderChild(m_DummyWidget, cWnd, &thisWndRect);
		//renderer->renderClientArea(m_DummyWidget, 0, &thisWndRect);
	}
}

void WDummy::resizeWidth(int iDiffWidth) 
{
	if(m_iResizingX == 1) 
	{
		if(iDiffWidth < 0 && getWidth() < m_DummyWidget->widgetSize.width)
			return;

		setWidth(getWidth() + iDiffWidth);
	}
	else 
	{
		if(iDiffWidth > 0 && getWidth() < m_DummyWidget->widgetSize.width)
			return;

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

	onResize();
}

void WDummy::resizeHeight(int iDiffHeight) 
{
	if(iDiffHeight < 0 && getHeight() < m_DummyWidget->widgetSize.height)
		return;

	DWORD dSize = (((getHeight() + iDiffHeight) & 0xFFFF) << 16) | ((getWidth()) & 0xFFFF);
	SendMessageQ(this, WM__SETSIZE, NULL, dSize);
}

void WDummy::onMouseDownEx(int x, int y, int iButton) 
{

}

void WDummy::onMouseMoveEx(int mCode, int x, int y, int prevx, int prevy) 
{
	// If click was on the title bar, drag window
	// If resizing, resize window

	// Drag window around:
	int diffX = (x - prevx);
	int diffY = (y - prevy);
	bool LEFT_MOUSE_DOWN = WWidgetManager::getInstance()->isMousePressed(GLFW_MOUSE_BUTTON_LEFT);

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

void WDummy::onMouseUpEx(int x, int y, int iButton) 
{
}

void WDummy::onMouseWheelEx(WPARAM wParam, LPARAM lParam) 
{

}

bool WDummy::isPtInside(int x, int y) 
{
	// x,y in local coordinates!

	// Check title bar and related gap!
	//TODO

	if (x < m_iLeft || x > m_iRight)
		return false;
	if (y < m_iTop || y > m_iBottom)
		return false;

	return true;
}

void WDummy::onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
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
				case ID_VERTICAL_SCROLLBAR:
					switch(lParam) 
					{
						case BTN_SCROLLBAR_UP:
						{
							m_iMainY += 25;
							if(m_iMainY > 0)
								m_iMainY = 0;

							updateVBarPosition();
						}
						break;
						case BTN_SCROLLBAR_DOWN:
						{
							m_iMainY -= 25;
							if(abs(m_iMainY) > (m_iMaxHeightPixels - m_ClientRect.Height))
								m_iMainY = -(m_iMaxHeightPixels - m_ClientRect.Height);

							updateVBarPosition();
						}
						break;
					}
					break;
				case ID_HORIZONTAL_SCROLLBAR:
					switch(lParam) 
					{
						case BTN_SCROLLBAR_LEFT:
						{
							m_iMainX += 25;
							if(m_iMainX > 0)
								m_iMainX = 0;

							updateHBarPosition();
						}
						break;
						case BTN_SCROLLBAR_RIGHT:
						{
							m_iMainX -= 25;
							if(abs(m_iMainX) > (m_iMaxWidthPixels - m_ClientRect.Width))
								m_iMainX = -(m_iMaxWidthPixels - m_ClientRect.Width);

							updateHBarPosition();
						}
						break;
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
				case ID_VERTICAL_SCROLLBAR:
				break;
				case ID_HORIZONTAL_SCROLLBAR:
				break;
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
				case ID_VERTICAL_SCROLLBAR:
				break;
				case ID_HORIZONTAL_SCROLLBAR:
				break;
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
				case ID_VERTICAL_SCROLLBAR:
				break;
				case ID_HORIZONTAL_SCROLLBAR:
				break;
				case ID_RESIZE_LEFT:
					WWidgetManager::getInstance()->setCursor(IDC__SIZENESW);
				break;
				case ID_RESIZE_RIGHT:
					WWidgetManager::getInstance()->setCursor(IDC__SIZENWSE);
				break;
			}
		}
		break;
		case SCROLLER_POS_ON_DRAG:
		{
			int cursorPosInPercentage = (int)lParam;
			switch(wParam) 
			{
				case ID_VERTICAL_SCROLLBAR:
				{
					float scrollMaterialHeight = (m_iMaxHeightPixels - m_ClientRect.Height);
					int mainYValue = (cursorPosInPercentage*scrollMaterialHeight)/100;
					m_iMainY = -mainYValue;
				}
				break;
				case ID_HORIZONTAL_SCROLLBAR:
				{
					float scrollMaterialWidth = (m_iMaxWidthPixels - m_ClientRect.Width);
					int mainXValue = (cursorPosInPercentage*scrollMaterialWidth)/100;
					m_iMainX = -mainXValue;
				}
				break;
			}
		}
		break;
	}
}

void WDummy::onResize() 
{
	onCreateEx((LPVOID)m_bHasScrollBars);
}

LRESULT WDummy::OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam) 
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
			DWORD dwSize = (DWORD)lParam;
			{
				int32_t iWidth = (dwSize & 0xffff);
				int32_t iHeight = ((dwSize >> 16) & 0xffff);

				setSize(iWidth, iHeight);
				onResize();
			}
		}
		break;
		case WM__RESIZE:
		{
			updateComponentPosition();
			onCreateEx(0);
		}
		break;
	}

	return WContainer::OnSendMessage(msg, wParam, lParam);
}
#endif