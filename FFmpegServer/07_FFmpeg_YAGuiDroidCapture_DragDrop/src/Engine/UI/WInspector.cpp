#ifdef USE_YAGUI
#include "Engine/UI/WInspector.h"
#include "Engine/UI/WWidgetManager.h"
#include "Engine/UI/WComponentFactory.h"
#include "Engine/UI/WTextField.h"
#include "Engine/UI/WComboBox.h"

WInspector::WInspector() 
: m_bResizing(false)
, m_bMoving(false)
, m_bResizable(true)
, m_bLButtonDown(false)
, m_sbVertical(nullptr)
, m_sbHorizontal(nullptr)
, m_ButtonWResizeLeft(nullptr)
, m_ButtonWResizeRight(nullptr)
{
}

WInspector::~WInspector() 
{

}

H_WND WInspector::Create(	const char* lpClassName, 
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
	WInspector* pWInspector = new WInspector();

	((WContainer*)pWInspector)->Create(	lpClassName, 
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

	return pWInspector;
}

void WInspector::onCreateEx(LPVOID lpVoid) 
{
	H_WND hWnd = NULL;

	RectF destRect;
	RectF wndRect(m_iLeft, m_iTop, getWidth(), getHeight());

	m_bHasScrollBars = (bool)lpVoid;
	m_FrameWidget = WWidgetManager::getWidget("InspectorC_Scroll");

	///////////////////////////////////////////////////
	if(m_bHasScrollBars) 
	{
		if (m_sbVertical == nullptr)
		{
			m_sbVertical = (WScrollbar*)WWidgetManager::addComponentFromWidget(	"WScrollbar",
																				"InspectorC_Scroll", 
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
		destRect = WWidgetManager::getDestinationRectFromWidgetForChild("InspectorC_Scroll", "VScroll", wndRect);
		{
			m_sbVertical->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
			m_sbVertical->setSize(destRect.Width, destRect.Height);
			m_sbVertical->hasBG(true);
			m_sbVertical->setPostRender(true);
		}
		///////////////////////////////////////////////////
		///////////////////////////////////////////////////
		if (m_sbHorizontal == nullptr)
		{
			m_sbHorizontal = (WScrollbar*)WWidgetManager::addComponentFromWidget(	"WScrollbar",
																					"InspectorC_Scroll", 
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
		destRect = WWidgetManager::getDestinationRectFromWidgetForChild("InspectorC_Scroll", "HScroll", wndRect);
		{
			m_sbHorizontal->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
			m_sbHorizontal->setSize(destRect.Width, destRect.Height);
			m_sbHorizontal->hasBG(true);
			m_sbHorizontal->setPostRender(true);
		}
		///////////////////////////////////////////////////
		///////////////////////////////////////////////////
		if (m_ButtonWResizeLeft == nullptr)
		{
			m_ButtonWResizeLeft = (WButton*)WWidgetManager::addComponentFromWidget(		"WButton",
																						"InspectorC_Scroll", 
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
		destRect = WWidgetManager::getDestinationRectFromWidgetForChild("InspectorC_Scroll", "WindowResizeLeft", wndRect);
		{
			m_ButtonWResizeLeft->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
			m_ButtonWResizeLeft->setSize(destRect.Width, destRect.Height);
			m_ButtonWResizeLeft->setPostRender(true);
			m_ButtonWResizeLeft->setMovable(false);
		}
		///////////////////////////////////////////////////
		///////////////////////////////////////////////////
		if (m_ButtonWResizeRight == nullptr)
		{
			m_ButtonWResizeRight = (WButton*)WWidgetManager::addComponentFromWidget("WButton",
																					"InspectorC_Scroll", 
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
		destRect = WWidgetManager::getDestinationRectFromWidgetForChild("InspectorC_Scroll", "WindowResizeRight", wndRect);
		{
			m_ButtonWResizeRight->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
			m_ButtonWResizeRight->setSize(destRect.Width, destRect.Height);
			m_ButtonWResizeRight->setPostRender(true);
			m_ButtonWResizeRight->setMovable(false);
		}
		///////////////////////////////////////////////////
	}

	updateClientRect("InspectorC_Scroll", wndRect);
	///////////////////////////////////////////
}

void WInspector::onUpdate(float deltaTimeMs) 
{
	if(!m_bHasScrollBars)
		return;

	//////////////////////////////////
	WInspectorTab* inspChild = 0;
	int yy = TOP_MARGIN_WIDTH;
	int prevInspectorTabHeight = 0;
	for(int i = 0; i < m_vInspectorTabs.size(); i++) 
	{
		inspChild = (WInspectorTab*)m_vInspectorTabs[i];

		inspChild->setPositionY(yy);

		prevInspectorTabHeight = inspChild->getHeight();
		yy += prevInspectorTabHeight;
	}
	//////////////////////////////////

	m_iMaxWidthPixels = m_iMaxHeightPixels = 0;
	if(m_pChildren.size() > 0) 
	{
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
	}

	setVScrollbarLength();
	setHScrollbarLength();

	bool bVertSBVisibility = (m_iMaxHeightPixels > m_ClientRect.Height);
	m_sbVertical->setDisable(NOT bVertSBVisibility);
	if(!bVertSBVisibility) 
	{
		m_ClientRect.Width = m_iClientRectW + m_sbVertical->getWidth();
		
		WInspectorTab* inspChild = 0;
		for(int i = 0; i < m_vInspectorTabs.size(); i++) 
		{
			inspChild = (WInspectorTab*)m_vInspectorTabs[i];
			inspChild->setWidth(m_iMaxInspectorTabWidth + m_sbVertical->getWidth());
		}
	}
	else 
	{
		m_ClientRect.Width = m_iClientRectW;

		WInspectorTab* inspChild = 0;
		for(int i = 0; i < m_vInspectorTabs.size(); i++) 
		{
			inspChild = (WInspectorTab*)m_vInspectorTabs[i];
			inspChild->setWidth(m_iMaxInspectorTabWidth);
		}
	}

	bool bHoriSBVisibility = (m_iMaxWidthPixels > m_ClientRect.Width);
	m_sbHorizontal->setDisable(NOT bHoriSBVisibility);
}

void WInspector::setVScrollbarLength() 
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

void WInspector::setHScrollbarLength() 
{
	float _part = m_ClientRect.Width;
	float _total = m_iMaxWidthPixels;
	float _percentage = (_part / _total) * 100;

	if(_percentage <= 100)
		m_sbHorizontal->setLength(_percentage);
}

void WInspector::updateVBarCursorPosition() 
{
	float _part = abs(m_iMainY);
	float _total = m_iMaxHeightPixels;
	float _percentage = (_part / _total) * 100;

	m_sbVertical->setCursorPositionInPercent(_percentage);
}

void WInspector::updateHBarCursorPosition() 
{
	float _part = abs(m_iMainX);
	float _total = m_iMaxWidthPixels;
	float _percentage = (_part / _total) * 100;

	m_sbHorizontal->setCursorPositionInPercent(_percentage);
}

void WInspector::onRender() 
{
	if(!m_bHasScrollBars)
		return;

	WWidgetManager* renderer =  WWidgetManager::getInstance();

	RectF thisWndRect(getLeft(), getTop(), getWidth(), getHeight());
		
	CHILD* cWnd = 0;
	cWnd = m_FrameWidget->getChild("FrameWindow");

	renderer->renderChild(m_FrameWidget, cWnd, &thisWndRect);
	//renderer->renderClientArea(m_FrameWidget, 0, &thisWndRect);
}

void WInspector::onMouseDownEx(int x, int y, int iButton) 
{
	// Check if click was on the title bar
	// Check if click was on the border and the window is resizable
	//TODO

	// Click was on the empty space, and window is movable.
	// Start moving.
	m_bLButtonDown = true;
}

void WInspector::onMouseMoveEx(int mCode, int x, int y, int prevx, int prevy) 
{
	// If click was on the title bar, drag window
	// If resizing, resize window
	
	if (isMovable() && m_bLButtonDown) 
	{
		if (x-prevx!=0 || y-prevy!=0)
			m_bMoving = true;

		// Drag window around:
		int diffX = (x - prevx);
		int diffY = (y - prevy);

		setPosition(getOffsetX() + diffX, getOffsetY() + diffY);
	}
}

void WInspector::onMouseUpEx(int x, int y, int iButton) 
{
	// stop dragging or
	// ..stop resizing
	m_bResizing = false;
	m_bMoving = false;
	m_bLButtonDown = false;
}

void WInspector::onMouseWheelEx(WPARAM wParam, LPARAM lParam)
{

}

bool WInspector::isPtInside(int x, int y) 
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

void WInspector::onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch(msg) 
	{
		case MOUSE_DOWN:
		{
			int buttonID = wParam;
			switch(wParam) 
			{
				case ID_VERTICAL_SCROLLBAR:
					switch(lParam) 
					{
						case BTN_SCROLLBAR_UP:
						{
							m_iMainY += 25;
							if(m_iMainY > 0)
								m_iMainY = 0;

							updateVBarCursorPosition();
						}
						break;
						case BTN_SCROLLBAR_DOWN:
						{
							m_iMainY -= 25;
							if(abs(m_iMainY) > (m_iMaxHeightPixels - m_ClientRect.Height))
								m_iMainY = -(m_iMaxHeightPixels - m_ClientRect.Height);

							updateVBarCursorPosition();
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

							updateHBarCursorPosition();
						}
						break;
						case BTN_SCROLLBAR_RIGHT:
						{
							m_iMainX -= 25;
							if(abs(m_iMainX) > (m_iMaxWidthPixels - m_ClientRect.Width))
								m_iMainX = -(m_iMaxWidthPixels - m_ClientRect.Width);

							updateHBarCursorPosition();
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
			}
		}
		break;
		case MOUSE_MOVE:
		{
			int buttonID = (wParam & 0xffff);
			int code = (wParam >> 16)& 0xffff;
			int diffX = (lParam >> 16)& 0xffff;
			int diffY = (lParam & 0xffff);
			switch(buttonID) 
			{
				case ID_VERTICAL_SCROLLBAR:
				break;
				case ID_HORIZONTAL_SCROLLBAR:
				break;
				case ID_RESIZE_LEFT:
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

void WInspector::addTab() 
{
	H_WND hWnd = NULL;

	hWnd = 
	CreateComponent(	"WInspectorTab", 
						"Title1", 
						0, 
						LEFT_MARGIN_WIDTH,
						TOP_MARGIN_WIDTH, 
						getWidth() - (RIGHT_MARGIN_WIDTH + INSPECTOR_SCROLL_WIDTH), 
						250,
						this, 
						HMENU(1112), 
						(LPVOID)NULL);
	WInspectorTab* wInspTab = (WInspectorTab*)hWnd;
	m_vInspectorTabs.push_back(wInspTab);
	m_iMaxInspectorTabWidth = wInspTab->getWidth();
	{
		/////////////////////////////////////////
		hWnd = 
		CreateComponent(	"WTextField", 
							"I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
							0, 
							20,
							40, 
							150, 
							23,
							wInspTab, 
							HMENU(99), 
							NULL);
		((WTextField*)hWnd)->setComponentAsChild(true);

		hWnd = 
		CreateComponent(	"WComboBox", 
							"", 
							0, 
							20,
							70, 
							250, 
							100,
							wInspTab, 
							HMENU(100), 
							NULL);

		hWnd = 
		CreateComponent(	"WButton", 
							"Simple Button", 
							0, 
							20,
							100, 
							125, 
							25,
							wInspTab, 
							HMENU(111), 
							LPVOID("Button"));
		((WButton*)hWnd)->setComponentAsChild(true);

		hWnd = 
		CreateComponent(	"WTextField", 
							"I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
							0, 
							20,
							130, 
							150, 
							23,
							wInspTab, 
							HMENU(99), 
							NULL);
		((WTextField*)hWnd)->setComponentAsChild(true);

		hWnd = 
		CreateComponent(	"WComboBox", 
							"", 
							0, 
							20,
							160, 
							250, 
							100,
							wInspTab, 
							HMENU(100), 
							NULL);

		hWnd = 
		CreateComponent(	"WButton", 
							"Simple Button", 
							0, 
							20,
							190, 
							125, 
							25,
							wInspTab, 
							HMENU(111), 
							LPVOID("Button"));
		((WButton*)hWnd)->setComponentAsChild(true);
		/////////////////////////////////////////
	}

	m_iInspTabCount = m_vInspectorTabs.size();
}
#endif