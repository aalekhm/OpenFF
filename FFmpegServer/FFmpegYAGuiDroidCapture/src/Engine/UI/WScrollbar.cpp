#ifdef USE_YAGUI
#include "Engine/UI/WScrollbar.h"
#include "Engine/UI/WComponentFactory.h"
#include "Engine/UI/WWidgetManager.h"

WScrollbar::WScrollbar()
{
	
}

H_WND WScrollbar::Create(	const char* lpClassName, 
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
	WScrollbar* pWScrollbar = new WScrollbar();
	((WContainer*)pWScrollbar)->Create(	lpClassName, 
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

	return pWScrollbar;
}

void WScrollbar::onCreateEx(LPVOID lpVoid) 
{
	H_WND hWnd = NULL;

	m_Align = (SB_ALIGN)((int)lpVoid);
	m_State = NORMAL;

	m_bDrawBG = false;
	////////////////////////////////////////////
	std::string leftOrUpScrollButtonName;
	std::string rightOrDownScrollButtonName;
	std::string scrollTrackName;

	bool bIsHorizontal = false;
	if(m_Align == HORIZONTAL) 
	{
		bIsHorizontal = true;

		m_ScrollbarWidget = WWidgetManager::getWidget("HScroll");
		leftOrUpScrollButtonName = "ButtonLeft";
		rightOrDownScrollButtonName = "ButtonRight";
		scrollTrackName = "HScrollTrack";
	}
	else
	if(m_Align == VERTICAL) 
	{
		m_ScrollbarWidget = WWidgetManager::getWidget("VScroll");
		leftOrUpScrollButtonName = "ButtonUp";
		rightOrDownScrollButtonName = "ButtonDown";
		scrollTrackName = "VScrollTrack";
	}
	////////////////////////////////////////////

	////////////////////////////////////////////
	RectF destRect;
	RectF wndRect;
	RectF idealRect;

	CHILD* leftScrollChild = m_ScrollbarWidget->getChild(leftOrUpScrollButtonName.c_str());
	wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();
	idealRect.X = leftScrollChild->posOffsets.x;
	idealRect.Y = leftScrollChild->posOffsets.y;
	idealRect.Width = leftScrollChild->posOffsets.w; 
	idealRect.Height = leftScrollChild->posOffsets.h;
	WWidgetManager::getDestinationRect(	destRect,
										m_ScrollbarWidget->widgetSize.width,
										m_ScrollbarWidget->widgetSize.height,
										&wndRect,
										&idealRect,
										leftScrollChild->align.eHAlign,
										leftScrollChild->align.eVAlign
									);
	hWnd = 
	CreateComponent(	"WButton", 
						"", 
						0, 
						destRect.X - m_iLeft,
						destRect.Y - m_iTop, 
						destRect.Width, 
						destRect.Height,
						this, 
						HMENU((m_Align == HORIZONTAL)?BTN_SCROLLBAR_LEFT:BTN_SCROLLBAR_UP), 
						(LPVOID)leftOrUpScrollButtonName.c_str());
	m_ButtonLeft = (WButton*)hWnd;
	////////////////////////////////////////////

	////////////////////////////////////////////
	CHILD* rightScrollChild = m_ScrollbarWidget->getChild(rightOrDownScrollButtonName.c_str());
	wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();
	idealRect.X = rightScrollChild->posOffsets.x;
	idealRect.Y = rightScrollChild->posOffsets.y;
	idealRect.Width = rightScrollChild->posOffsets.w; 
	idealRect.Height = rightScrollChild->posOffsets.h;
	WWidgetManager::getDestinationRect(	destRect,
										m_ScrollbarWidget->widgetSize.width,
										m_ScrollbarWidget->widgetSize.height,
										&wndRect,
										&idealRect,
										rightScrollChild->align.eHAlign,
										rightScrollChild->align.eVAlign
									);
	hWnd = 
	CreateComponent(	"WButton", 
						"", 
						0, 
						destRect.X - m_iLeft,
						destRect.Y - m_iTop, 
						destRect.Width, 
						destRect.Height,
						this, 
						HMENU((m_Align == HORIZONTAL)?BTN_SCROLLBAR_RIGHT:BTN_SCROLLBAR_DOWN), 
						(LPVOID)rightOrDownScrollButtonName.c_str());
	m_ButtonRight = (WButton*)hWnd;
	////////////////////////////////////////////

	////////////////////////////////////////////
	CHILD* scrollTrackChild = m_ScrollbarWidget->getChild(scrollTrackName.c_str());
	m_sliderLen = (bIsHorizontal)?scrollTrackChild->posOffsets.w:scrollTrackChild->posOffsets.h;
	wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();
	idealRect.X = scrollTrackChild->posOffsets.x;
	idealRect.Y = scrollTrackChild->posOffsets.y;
	idealRect.Width = (bIsHorizontal)?m_sliderLen:scrollTrackChild->posOffsets.w; 
	idealRect.Height = (bIsHorizontal)?scrollTrackChild->posOffsets.h:m_sliderLen;
	WWidgetManager::getDestinationRect(	destRect,
										m_ScrollbarWidget->widgetSize.width,
										m_ScrollbarWidget->widgetSize.height,
										&wndRect,
										&idealRect,
										scrollTrackChild->align.eHAlign,
										scrollTrackChild->align.eVAlign
									);
	m_minSliderPos = (bIsHorizontal)?destRect.X - m_iLeft:destRect.Y - m_iTop;
	m_curSliderPos = m_minSliderPos;
	m_maxSliderPos = (bIsHorizontal)?(getWidth() - scrollTrackChild->posOffsets.x - m_sliderLen):(getWidth() - scrollTrackChild->posOffsets.y - m_sliderLen);

	hWnd = 
	CreateComponent(	"WButton", 
						"", 
						0, 
						destRect.X - m_iLeft,
						destRect.Y - m_iTop, 
						destRect.Width, 
						destRect.Height,
						this, 
						HMENU(BTN_SCROLLBAR_SCROLL), 
						scrollTrackChild->sName);
	m_ButtonScrollTrack = (WButton*)hWnd;
	m_ButtonScrollTrack->setMovable(true);
	////////////////////////////////////////////

	setMovable(false);
	setAsIntegral(true);
}

void WScrollbar::onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch(msg) 
	{
		case MOUSE_DOWN:
		{
			int buttonID = wParam;
			switch(wParam) 
			{
				case BTN_SCROLLBAR_LEFT:
				case BTN_SCROLLBAR_UP:
					if(m_pParent)
						m_pParent->onMessage((H_WND)this, MOUSE_DOWN, getComponentID(), (m_Align == HORIZONTAL)?BTN_SCROLLBAR_LEFT:BTN_SCROLLBAR_UP);
				break;
				case BTN_SCROLLBAR_RIGHT:
				case BTN_SCROLLBAR_DOWN:
					if(m_pParent)
						m_pParent->onMessage((H_WND)this, MOUSE_DOWN, getComponentID(), (m_Align == HORIZONTAL)?BTN_SCROLLBAR_RIGHT:BTN_SCROLLBAR_DOWN);
				break;
				case BTN_SCROLLBAR_SCROLL:
				break;
			}

		}
		break;
		case MOUSE_UP:
		{
			int buttonID = wParam;
			switch(wParam) 
			{
				case BTN_SCROLLBAR_LEFT:
				case BTN_SCROLLBAR_UP:
					if(m_pParent)
						m_pParent->onMessage((H_WND)this, MOUSE_UP, getComponentID(), (m_Align == HORIZONTAL)?BTN_SCROLLBAR_LEFT:BTN_SCROLLBAR_UP);
				break;
				case BTN_SCROLLBAR_RIGHT:
				case BTN_SCROLLBAR_DOWN:
					if(m_pParent)
						m_pParent->onMessage((H_WND)this, MOUSE_UP, getComponentID(), (m_Align == HORIZONTAL)?BTN_SCROLLBAR_RIGHT:BTN_SCROLLBAR_DOWN);
				break;
				case BTN_SCROLLBAR_SCROLL:					
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
				case BTN_SCROLLBAR_SCROLL:
				{
					bool LEFT_MOUSE_DOWN = WWidgetManager::getInstance()->isMousePressed(GLFW_MOUSE_BUTTON_LEFT);
					if(LEFT_MOUSE_DOWN)
					{
						if(m_Align == HORIZONTAL)
							moveCursorBy(dwDiffX);
						else
							moveCursorBy(dwDiffY);

						if(m_pParent) 
						{
							float _0Per = m_minSliderPos;
							float _100Per = m_maxSliderPos;// + m_sliderLen;
							float _total = _100Per - _0Per;

							float actualVal = m_curSliderPos-m_minSliderPos;
							float percentage = (actualVal/_total)*100;

							m_pParent->onMessage((H_WND)this, SCROLLER_POS_ON_DRAG, getComponentID(), percentage);
						}
					}
				}
				break;
			}
		}
		break;
		case WIDTH_CHANGED:
		{
			int iLastWidth = (int)wParam;
			int iNewWidth = (int)lParam;
			if(m_Align == HORIZONTAL) 
			{
				RectF destRect;
				RectF wndRect;
				RectF idealRect;

				CHILD* rightScrollChild = m_ScrollbarWidget->getChild("ButtonRight");
				wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();
				idealRect.X = rightScrollChild->posOffsets.x;
				idealRect.Y = rightScrollChild->posOffsets.y;
				idealRect.Width = rightScrollChild->posOffsets.w; 
				idealRect.Height = rightScrollChild->posOffsets.h;
				WWidgetManager::getDestinationRect(	destRect,
													m_ScrollbarWidget->widgetSize.width,
													m_ScrollbarWidget->widgetSize.height,
													&wndRect,
													&idealRect,
													rightScrollChild->align.eHAlign,
													rightScrollChild->align.eVAlign
												);
				m_ButtonRight->setPositionX(destRect.X - m_iLeft);
			}
		}
		break;
		case HEIGHT_CHANGED:
		{
			int iLastHeight = (int)wParam;
			int iNewHeight = (int)lParam;
			if(m_Align == VERTICAL) 
			{
				RectF destRect;
				RectF wndRect;
				RectF idealRect;

				CHILD* bottomScrollChild = m_ScrollbarWidget->getChild("ButtonDown");
				wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();
				idealRect.X = bottomScrollChild->posOffsets.x;
				idealRect.Y = bottomScrollChild->posOffsets.y;
				idealRect.Width = bottomScrollChild->posOffsets.w; 
				idealRect.Height = bottomScrollChild->posOffsets.h;
				WWidgetManager::getDestinationRect(	destRect,
													m_ScrollbarWidget->widgetSize.width,
													m_ScrollbarWidget->widgetSize.height,
													&wndRect,
													&idealRect,
													bottomScrollChild->align.eHAlign,
													bottomScrollChild->align.eVAlign
												);
				m_ButtonRight->setPositionY(destRect.Y - m_iTop);
			}
		}
		break;
	}
}

void WScrollbar::frameUpdate(float deltaTimeMs) 
{
	WContainer::frameUpdate(deltaTimeMs);
}

void WScrollbar::onUpdate(float deltaTimeMs) 
{

}

void WScrollbar::setDisable(bool bDisable) 
{
	SendMessageQ(m_ButtonLeft,			bDisable ? BM__DISABLE : BM__ENABLE, NULL, NULL);
	SendMessageQ(m_ButtonRight,			bDisable ? BM__DISABLE : BM__ENABLE, NULL, NULL);
	SendMessageQ(m_ButtonScrollTrack,	bDisable ? BM__DISABLE : BM__ENABLE, NULL, NULL);
}

void WScrollbar::setReadOnly(bool bDisable) 
{
	if(bDisable) 
	{
		m_ButtonLeft->deactivate();
		m_ButtonRight->deactivate();
		m_ButtonScrollTrack->deactivate();
	}
	else 
	{
		m_ButtonLeft->activate();
		m_ButtonRight->activate();
		m_ButtonScrollTrack->activate();
	}
}

void WScrollbar::frameRender() 
{
	WContainer::frameRender();
}

void WScrollbar::onRender() 
{
	WWidgetManager::getInstance()->setColor(m_RenderColor.r, m_RenderColor.g, m_RenderColor.b, m_RenderColor.a);
	{
		WWidgetManager* renderer = WWidgetManager::getInstance();
		RectF thisWndRect(getLeft(), getTop(), getWidth(), getHeight());

		BASESKIN* leftBG = m_ScrollbarWidget->getSkin("HSB_LEFT_BG");
		if (m_bDrawBG) renderer->renderSkin(m_ScrollbarWidget, leftBG, &thisWndRect);

		BASESKIN* centerBG = m_ScrollbarWidget->getSkin("HSB_CENTER_BG");
		if (m_bDrawBG) renderer->renderSkin(m_ScrollbarWidget, centerBG, &thisWndRect);

		BASESKIN* rightBG = m_ScrollbarWidget->getSkin("HSB_RIGHT_BG");
		if (m_bDrawBG) renderer->renderSkin(m_ScrollbarWidget, rightBG, &thisWndRect);

		if (m_Align == HORIZONTAL)
		{
			CHILD* child = m_ScrollbarWidget->getChild("HScrollLeftPart");
			renderer->renderChild(m_ScrollbarWidget, child, &thisWndRect);

			child = m_ScrollbarWidget->getChild("HScrollRightPart");
			renderer->renderChild(m_ScrollbarWidget, child, &thisWndRect);
		}
		else
		{
			CHILD* child = m_ScrollbarWidget->getChild("VScrollTopPart");
			renderer->renderChild(m_ScrollbarWidget, child, &thisWndRect);

			child = m_ScrollbarWidget->getChild("VScrollBottomPart");
			renderer->renderChild(m_ScrollbarWidget, child, &thisWndRect);
		}
	}
	WWidgetManager::getInstance()->resetColor();

	//BaseWindow::drawString(child->sName, getLeft() + 25, getTop() + 2, getWidth(), 16, FontStyle::FontStyleBold);
}

void WScrollbar::onMouseDownEx(int x, int y, int iButton) 
{
	if(m_Align == HORIZONTAL) 
	{
		if(x > m_curSliderPos)
			moveCursorBy(5);
		else
			moveCursorBy(-5);
	}
	else 
	{
		if(y > m_curSliderPos)
			moveCursorBy(5);
		else
			moveCursorBy(-5);
	}
}

void WScrollbar::onMouseUpEx(int x, int y, int iButton) 
{

}

void WScrollbar::onMouseEnterEx(int mCode, int x, int y, int prevX, int prevY) 
{

}

void WScrollbar::onMouseHoverEx(int mCode, int x, int y, int prevX, int prevY) 
{
	WWidgetManager::setCursor(IDC__ARROW);
}

void WScrollbar::onMouseLeaveEx(int mCode, int x, int y, int prevX, int prevY) 
{

}

void WScrollbar::onMouseMoveEx(int mCode, int x, int y, int prevX, int prevY) 
{

}

void WScrollbar::onMouseWheelEx(WPARAM wParam, LPARAM lParam) 
{

}

void WScrollbar::onKeyBDownEx(unsigned int iVirtualKeycode, unsigned short ch) 
{

}

void WScrollbar::onKeyBUpEx(unsigned int iVirtualKeycode, unsigned short ch) 
{

}

void WScrollbar::setCursorPositionInPercent(float pixelsPercentage) 
{
	float _100Per = m_maxSliderPos - m_minSliderPos + m_sliderLen;
	float _value = pixelsPercentage * _100Per / 100.0f;

	int prevSliderPos = m_curSliderPos;
	m_curSliderPos = m_minSliderPos + _value;

	if(m_curSliderPos < m_minSliderPos)
		m_curSliderPos = m_minSliderPos;
	else
	if(m_curSliderPos > m_maxSliderPos)
		m_curSliderPos = m_maxSliderPos;

	int diff = (m_curSliderPos - prevSliderPos);
	if(m_Align == HORIZONTAL)
		m_ButtonScrollTrack->setPositionX(prevSliderPos + diff);
	else
		m_ButtonScrollTrack->setPositionY(prevSliderPos + diff);	
}

void WScrollbar::moveCursorBy(int pixels) 
{
	int prevSliderPos = m_curSliderPos;
	m_curSliderPos += pixels;

	if(m_curSliderPos < m_minSliderPos)
		m_curSliderPos = m_minSliderPos;
	else
	if(m_curSliderPos > m_maxSliderPos)
		m_curSliderPos = m_maxSliderPos;
	
	int diff = (m_curSliderPos - prevSliderPos);
	if(m_Align == HORIZONTAL)
		m_ButtonScrollTrack->setPositionX(prevSliderPos + diff);
	else
		m_ButtonScrollTrack->setPositionY(prevSliderPos + diff);
}

void WScrollbar::setLength(float pixelsPercentage) 
{
	int buttonH = 0;
	CHILD* scrollTrackChild = 0;
	float actualHeight = 0;

	if(m_Align == HORIZONTAL) 
	{
		scrollTrackChild = m_ScrollbarWidget->getChild("HScrollTrack");
		buttonH = scrollTrackChild->posOffsets.x;

		actualHeight = (getRight()-getLeft())-(2*buttonH);
	}
	else 
	{
		scrollTrackChild = m_ScrollbarWidget->getChild("VScrollTrack");
		buttonH = scrollTrackChild->posOffsets.y;

		actualHeight = (getBottom()-getTop())-(2*buttonH);
	}
	
	int pixels = (pixelsPercentage*actualHeight)/100;
	if(pixels < 10)
		pixels = 10;

	m_sliderLen = pixels;

	if(m_Align == HORIZONTAL) 
	{
		m_ButtonScrollTrack->setWidth(m_sliderLen);
		m_maxSliderPos = getWidth() - scrollTrackChild->posOffsets.x - m_sliderLen;
	}
	else 
	{
		m_ButtonScrollTrack->setHeight(pixels);
		m_maxSliderPos = getHeight() - scrollTrackChild->posOffsets.y - m_sliderLen;
	}
}

WScrollbar::~WScrollbar() 
{
}
#endif