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
	H_WND hWnd = NULL;

	RectF vDestRect;
	RectF hDestRect;

	RectF wndRect;
	RectF idealRect;

	m_bHasScrollBars = (bool)lpVoid;
	m_DummyWidget = WWidgetManager::getWidget("CanvasWindow");

	///////////////////////////////////////////////////
	m_minX = hDestRect.X;
	m_maxX = hDestRect.X + hDestRect.Width;
	m_minY = vDestRect.Y;
	m_maxY = vDestRect.Y + vDestRect.Height;
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	CHILD* btnChild = m_DummyWidget->getChild("WindowResizeLeft");
	wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();
	idealRect.Set(btnChild->posOffsets.x, btnChild->posOffsets.y, btnChild->posOffsets.w, btnChild->posOffsets.h);
	WWidgetManager::getDestinationRect(	hDestRect,
										m_DummyWidget->widgetSize.width,
										m_DummyWidget->widgetSize.height,
										&wndRect,
										&idealRect,
										btnChild->align.eHAlign,
										btnChild->align.eVAlign
									);
	if(m_ButtonWResizeLeft == NULL) 
	{
		hWnd = 
		CreateComponent(	"WButton", 
							"", 
							0, 
							hDestRect.X - m_iLeft,
							hDestRect.Y - m_iTop, 
							hDestRect.Width, 
							hDestRect.Height,
							this, 
							HMENU(ID_RESIZE_LEFT), 
							LPVOID("WindowResizeLeft"));
		m_ButtonWResizeLeft = (WButton*)hWnd;
		m_ButtonWResizeLeft->setPostRender(true);
		m_ButtonWResizeLeft->setMovable(false);
		m_ButtonWResizeLeft->setAsIntegral(true);
	}
	else 
	{
		m_ButtonWResizeLeft->setPosition(hDestRect.X - m_iLeft, hDestRect.Y - m_iTop);
		m_ButtonWResizeLeft->setSize(hDestRect.Width, hDestRect.Height);
	}
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	btnChild = m_DummyWidget->getChild("WindowResizeRight");
	wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();
	idealRect.Set(btnChild->posOffsets.x, btnChild->posOffsets.y, btnChild->posOffsets.w, btnChild->posOffsets.h);
	WWidgetManager::getDestinationRect(	hDestRect,
											m_DummyWidget->widgetSize.width,
											m_DummyWidget->widgetSize.height,
											&wndRect,
											&idealRect,
											btnChild->align.eHAlign,
											btnChild->align.eVAlign
											);
	if(m_ButtonWResizeRight == NULL) 
	{
		hWnd = 
		CreateComponent(	"WButton", 
									"", 
									0, 
									hDestRect.X - m_iLeft,
									hDestRect.Y - m_iTop, 
									hDestRect.Width, 
									hDestRect.Height,
									this, 
									HMENU(ID_RESIZE_RIGHT), 
									LPVOID("WindowResizeRight"));
		m_ButtonWResizeRight = (WButton*)hWnd;
		m_ButtonWResizeRight->setPostRender(true);
		m_ButtonWResizeRight->setMovable(false);
		m_ButtonWResizeRight->setAsIntegral(true);

		CHILD* cWnd = m_DummyWidget->getChild("CanvasBorder");
		addBaseSkinChild(cWnd);
	}
	else 
	{
		m_ButtonWResizeRight->setPosition(hDestRect.X - m_iLeft, hDestRect.Y - m_iTop);
		m_ButtonWResizeRight->setSize(hDestRect.Width, hDestRect.Height);
	}
	///////////////////////////////////////////////////

	bool bHasClientArea = (m_DummyWidget->clientAreas.size() > 0);
	if(bHasClientArea) 
	{
		wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();

		m_ClientRect = wndRect;
		m_iClientRectW = m_ClientRect.Width;
		m_iClientRectH = m_ClientRect.Height;
	}
	///////////////////////////////////////////
}

void WCanvas::onUpdate(float deltaTimeMs) 
{

}

void WCanvas::onRender() 
{
	if(getBorderVisibility()) 
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
	setSize(width, height);
	onCreateEx((LPVOID)m_bHasScrollBars);
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

			return WM__OKAY;
		}
		break;
	}
}
#endif