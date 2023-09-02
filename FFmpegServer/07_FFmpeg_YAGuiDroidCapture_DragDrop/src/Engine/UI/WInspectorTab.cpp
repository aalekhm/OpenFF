#ifdef USE_YAGUI
#include "Engine/UI/WInspectorTab.h"
#include "Engine/UI/WComponentFactory.h"
#include "Engine/UI/WWidgetManager.h"

WInspectorTab::WInspectorTab()
: m_bLButtonDown(false)
, m_ButtonExpand(nullptr)
, m_ButtonCollapse(nullptr)
{
	setIsContainer(true);
	m_iMainX = m_iMainY = 0;

	m_iState = STATE_INSPECTOR_OPEN;
}

WInspectorTab::~WInspectorTab() 
{

}

H_WND WInspectorTab::Create(	const char* lpClassName, 
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
	WInspectorTab* pWInspectorTab = new WInspectorTab();
	((WContainer*)pWInspectorTab)->Create(	lpClassName, 
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

	return pWInspectorTab;
}

void WInspectorTab::onCreateEx(LPVOID lpVoid) 
{
	H_WND hWnd = NULL;

	//RectF vDestRect;
	RectF destRect;
	RectF wndRect(m_iLeft, m_iTop, getWidth(), getHeight());
	RectF idealRect;

	m_InspectorTabCWidget = WWidgetManager::getWidget("InspectorTabC_Scroll");

	///////////////////////////////////////////////////
	if (m_ButtonExpand == nullptr)
	{
		m_ButtonExpand = (WButton*)WWidgetManager::addComponentFromWidget(	"WButton",
																			"InspectorTabC_Scroll", 
																			"ButtonPlus", 
																			wndRect, 
																			this, 
																			HMENU(BTN_PLUS),
																			LPVOID("ButtonPlus"),
																			[=](CHILD* pChildWidget, RectF destRect, RectF& childRect)
																			{
																				childRect.X = destRect.X - m_iLeft;
																				childRect.Y = destRect.Y - m_iTop;
																				childRect.Width = destRect.Width;
																				childRect.Height = destRect.Height;
																			});
	}
	destRect = WWidgetManager::getDestinationRectFromWidgetForChild("InspectorTabC_Scroll", "ButtonPlus", wndRect);
	{
		m_ButtonExpand->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
		m_ButtonExpand->setSize(destRect.Width, destRect.Height);
		m_ButtonExpand->setPostRender(true);
		m_ButtonExpand->setMovable(true);
	}
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	if (m_ButtonCollapse == nullptr)
	{
		m_ButtonCollapse = (WButton*)WWidgetManager::addComponentFromWidget("WButton",
																			"InspectorTabC_Scroll", 
																			"ButtonMinus", 
																			wndRect, 
																			this, 
																			HMENU(BTN_MINUS),
																			LPVOID("ButtonMinus"),
																			[=](CHILD* pChildWidget, RectF destRect, RectF& childRect)
																			{
																				childRect.X = destRect.X - m_iLeft;
																				childRect.Y = destRect.Y - m_iTop;
																				childRect.Width = destRect.Width;
																				childRect.Height = destRect.Height;
																			});
	}
	destRect = WWidgetManager::getDestinationRectFromWidgetForChild("InspectorTabC_Scroll", "ButtonMinus", wndRect);
	{
		m_ButtonCollapse->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
		m_ButtonCollapse->setSize(destRect.Width, destRect.Height);
		m_ButtonCollapse->setPostRender(true);
		m_ButtonCollapse->setMovable(true);
		m_ButtonCollapse->setVisible(false);
	}
	///////////////////////////////////////////////////
	
	///////////////////////////////////////////////////
	updateClientRect("InspectorTabC_Scroll", wndRect);
	///////////////////////////////////////////
}

void WInspectorTab::onUpdate(float deltaTimeMs) 
{
	m_iMaxWidthPixels = m_iMaxHeightPixels = 0;
	for(int i = 0; i < m_pChildren.size(); i++) 
	{
		WComponent* comp = (WComponent*)m_pChildren[i];
		if(comp->isComponentAChild()) 
		{
			int iRight = comp->getOffsetX() + comp->getWidth();
			if(iRight > m_iMaxWidthPixels)
				m_iMaxWidthPixels = iRight;

			int iBottom = comp->getOffsetY() + comp->getHeight();
			if(iBottom > m_iMaxHeightPixels)
				m_iMaxHeightPixels = iBottom;
		}
	}

	//ANIMATE//////////////////////////
	switch(m_iState) 
	{
		case STATE_INSPECTOR_OPEN:
		{
			m_ButtonExpand->setVisible(false);
			m_ButtonCollapse->setVisible(true);
		}
		break;
		case STATE_INSPECTOR_CLOSING:
		{
			m_iRate = deltaTimeMs;

			setHeight(getHeight() - m_iRate);
			m_ClientRect.Height = getHeight() - INSPECTOR_TITLEBAR_HEIGHT - INSPECTOR_BOTTOM_DECORATION;

			if(getHeight() <= INSPECTOR_TITLEBAR_HEIGHT) 
			{
				setHeight(INSPECTOR_TITLEBAR_HEIGHT);

				//Set Client Height to '0' so that childrens are not drawn in WContainer::frameRender().
				m_ClientRect.Height = 0;

				setState(STATE_INSPECTOR_CLOSED);
			}
		}
		break;
		case STATE_INSPECTOR_CLOSED:
		{
			m_ButtonExpand->setVisible(true);
			m_ButtonCollapse->setVisible(false);
		}
		break;
		case STATE_INSPECTOR_OPENING:
		{
			m_iRate = deltaTimeMs;

			setHeight(getHeight() + m_iRate);
			m_ClientRect.Height = getHeight() - INSPECTOR_TITLEBAR_HEIGHT - INSPECTOR_BOTTOM_DECORATION;

			if(getHeight() >= m_iMaxHeightPixels + INSPECTOR_TITLEBAR_HEIGHT) 
			{
				setHeight(m_iMaxHeightPixels + INSPECTOR_TITLEBAR_HEIGHT);
				m_ClientRect.Height = getHeight() - INSPECTOR_TITLEBAR_HEIGHT - INSPECTOR_BOTTOM_DECORATION;

				setState(STATE_INSPECTOR_OPEN);
			}
		}
		break;
	}
	///////////////////////////////////
}

void WInspectorTab::onRender() 
{
	WWidgetManager* renderer =  WWidgetManager::getInstance();

	RectF thisWndRect(getLeft(), getTop(), getWidth(), getHeight());
		
	CHILD* cWnd = m_InspectorTabCWidget->getChild("InspectorTabC");
	renderer->renderChild(m_InspectorTabCWidget, cWnd, &thisWndRect);
	//renderer->renderClientArea(m_InspectorTabCWidget, 0, &thisWndRect);
}

void WInspectorTab::onMouseDownEx(int x, int y, int iButton) 
{
	// Check if click was on the title bar
	// Check if click was on the border and the window is resizable
	//TODO

	// Click was on the empty space, and window is movable.
	// Start moving.
	m_bLButtonDown = true;
}

void WInspectorTab::onMouseMoveEx(int mCode, int x, int y, int prevx, int prevy) 
{

}

void WInspectorTab::onMouseUpEx(int x, int y, int iButton) 
{
	// stop dragging or
	// ..stop resizing
	m_bLButtonDown = false;
}

void WInspectorTab::onMouseWheelEx(WPARAM wParam, LPARAM lParam)
{

}

bool WInspectorTab::isPtInside(int x, int y) 
{
	// x,y in local coordinates!

	// Check title bar and related gap!
	//TODO

	if (x < m_iLeft+12 || x > m_iRight-14)
		return false;
	if (y < m_iTop+12 || y > m_iBottom-14)
		return false;

	return true;
}

void WInspectorTab::onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{

	switch(msg) 
	{
		case MOUSE_UP:
		{
			int buttonID = wParam;
			switch(wParam) 
			{
				case BTN_PLUS:
					setState(STATE_INSPECTOR_OPENING);
				break;
				case BTN_MINUS:
					setState(STATE_INSPECTOR_CLOSING);
				break;
			}
		}
		break;
		case WIDTH_CHANGED:
		{
			int iLastWidth = (int)wParam;
			int iNewWidth = (int)lParam;

			RectF destRect;
			RectF wndRect;
			RectF idealRect;

			CHILD* btnChild = m_InspectorTabCWidget->getChild("ButtonPlus");
			wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();
			idealRect.X = btnChild->posOffsets.x;
			idealRect.Y = btnChild->posOffsets.y;
			idealRect.Width = btnChild->posOffsets.w; 
			idealRect.Height = btnChild->posOffsets.h;
			WWidgetManager::getDestinationRect(	destRect,
												m_InspectorTabCWidget->widgetSize.width,
												m_InspectorTabCWidget->widgetSize.height,
												&wndRect,
												&idealRect,
												btnChild->align.eHAlign,
												btnChild->align.eVAlign
											);
			//m_ButtonExpand->setPositionX(destRect.X - m_iLeft);

			btnChild = m_InspectorTabCWidget->getChild("ButtonMinus");
			wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();
			idealRect.X = btnChild->posOffsets.x;
			idealRect.Y = btnChild->posOffsets.y;
			idealRect.Width = btnChild->posOffsets.w; 
			idealRect.Height = btnChild->posOffsets.h;
			WWidgetManager::getDestinationRect(	destRect,
												m_InspectorTabCWidget->widgetSize.width,
												m_InspectorTabCWidget->widgetSize.height,
												&wndRect,
												&idealRect,
												btnChild->align.eHAlign,
												btnChild->align.eVAlign
											);
			//m_ButtonCollapse->setPositionX(destRect.X - m_iLeft);
		}
		break;
	}
}

void WInspectorTab::setState(int iState) 
{
	m_iState = iState;
}
#endif