#ifdef USE_YAGUI
#include "Engine/UI/WListBox.h"
#include "Engine/UI/WComponentFactory.h"
#include "Engine/UI/WWidgetManager.h"
#include <algorithm>

using namespace std;

#define TB_LEFT_GUTTER			5
#define TB_TOP_GUTTER			5
#define TB_RIGHT_GUTTER			10
#define TB_CURSOR_TOP			2
#define TB_CURSOR_BOTTOM		12

WListBox::WListBox() 
: m_IsVScrolling(false)
, m_IsHScrolling(false)
, m_bStretchProportionalToParent(false)
, m_CaretPosX(0)
, m_CaretPosY(0)
, m_mainX(0)
, m_minX(0)
, m_maxX(0)
, m_mainY(0)
, m_minY(0)
, m_maxY(0)
, m_iMaxWidthPixels(0)
, m_iMaxHScrollbarWidth(0)
, m_iMaxVScrollbarHeight(0)
, m_sbVertical(nullptr)
, m_sbHorizontal(nullptr)
{

}

struct maxLineLength 
{
	bool operator() ( LISTBOX_ITEM* item1, LISTBOX_ITEM* item2 )
	{
		int s1 = item1->itemLabel.size();
		int s2 = item2->itemLabel.size();
		//printf("%d 0r %d\n", s1, s2);

		return (s1 < s2);
	}
};

H_WND WListBox::Create(	const char* lpClassName, 
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
	WListBox* pWListBox = new WListBox();

	((WContainer*)pWListBox)->Create(	lpClassName, 
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

	return pWListBox;
}

void WListBox::onCreateEx(LPVOID lpVoid) 
{
	H_WND hWnd = NULL;

	m_ListBoxWidget = WWidgetManager::getWidget("ListBox");
	m_minX = getLeft() + TB_LEFT_GUTTER;
	m_minY = getTop() + TB_TOP_GUTTER;
	LINE_HEIGHT = WWidgetManager::CHARACTER_HEIGHT;

	///////////////////////////////////////////////////
	RectF destRect, idealRect;
	RectF wndRect(m_iLeft, m_iTop, getWidth(), getHeight());

	if (m_sbVertical == nullptr)
	{
		m_sbVertical = (WScrollbar*)WWidgetManager::addComponentFromWidget(	"WScrollbar",
																			"ListBox", 
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
	destRect = WWidgetManager::getDestinationRectFromWidgetForChild("ListBox", "VScroll", wndRect);
	{
		m_sbVertical->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
		m_sbVertical->setSize(destRect.Width, destRect.Height);
		m_sbVertical->hasBG(true);
		m_sbVertical->setPostRender(true);
		m_iMaxVScrollbarHeight = destRect.Height;
		m_maxX = getRight() - m_sbVertical->getWidth() - TB_RIGHT_GUTTER;
	}
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	if (m_sbHorizontal == nullptr)
	{
		m_sbHorizontal = (WScrollbar*)WWidgetManager::addComponentFromWidget(	"WScrollbar",
																				"ListBox", 
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
	destRect = WWidgetManager::getDestinationRectFromWidgetForChild("ListBox", "HScroll", wndRect);
	{
		m_sbHorizontal->setPosition(destRect.X - m_iLeft, destRect.Y - m_iTop);
		m_sbHorizontal->setSize(destRect.Width, destRect.Height);
		m_sbHorizontal->hasBG(true);
		m_sbHorizontal->setVisible(false);
		m_sbHorizontal->setPostRender(true);
		m_iMaxHScrollbarWidth = destRect.Width;
	}
	///////////////////////////////////////////////////
	updateClientRect("ListBox", wndRect);
	///////////////////////////////////////////////////

	setProportionalToParent(true);//STRETCH_TO_PARENTS_WIDTH);

	updateVBarPosition();

	updateScrollBarVisibility();
	updateMains();
}

void WListBox::calculateMaxLineWidth() 
{
	if(m_Data.size() > 0) 
	{
		std::vector<LISTBOX_ITEM*>::iterator pos = std::max_element(m_Data.begin(), m_Data.end(), maxLineLength());
		LISTBOX_ITEM* maxItem = m_Data[pos - m_Data.begin()];

		const char* str = maxItem->itemLabel.c_str();
		m_iMaxWidthPixels = TB_LEFT_GUTTER + WWidgetManager::getStringWidthTillPos(str, strlen(str)) + TB_RIGHT_GUTTER;
	}
}

void WListBox::addItem(LISTBOX_ITEM* item) 
{
	m_Data.push_back(item);
	
	updateStretch();
	updateMains();
}

void WListBox::insertItemAt(int pos, LISTBOX_ITEM* item) 
{
	std::vector<LISTBOX_ITEM*>::iterator itr = m_Data.begin();
	m_Data.insert(itr + pos, item);
}

void WListBox::removeItemAt(int pos) 
{
	if(pos > m_Data.size() || pos < 0)
		return;

	LISTBOX_ITEM* item = m_Data[pos];
	delete &item->itemLabel;
	/*if(item->itemData) {
		delete item->itemData;
		item->itemData = NULL;
	}*/
	
	std::vector<LISTBOX_ITEM*>::iterator itr = m_Data.begin();
	m_Data.erase(itr + pos);

	updateStretch();
	updateMains();
}

int WListBox::getSelectedIndex() 
{
	return CURSOR_LINE_NO;
}

void WListBox::setSelectedIndex(int pos) 
{
	if(pos > m_Data.size()-1 || pos < 0)
		return;

	CURSOR_LINE_NO = pos;

	setCaretDrawPosition();
	updateMains();
}

const char* WListBox::getSelectedString() 
{
	LISTBOX_ITEM* item = m_Data[CURSOR_LINE_NO];
	return (const char*)item->itemLabel.c_str();
}

const char* WListBox::getStringAt(int pos) 
{
	if(pos > m_Data.size() || pos < 0)
		return NULL;
	
	LISTBOX_ITEM* item = m_Data[pos];
	return (const char*)item->itemLabel.c_str();
}

int WListBox::setStringAt(int iIndex, const char* newString) 
{
	if(iIndex < 0 || iIndex > getSize() - 1 || newString == NULL)
		return -1;

	LISTBOX_ITEM* item = m_Data[iIndex];
	item->itemLabel = newString;

	return 1;
}

int WListBox::getSize() 
{
	if(m_Data.size() > 0)
		return m_Data.size();

	return -1;
}

void WListBox::updateStretch() 
{
	calculateMaxLineWidth();

	if(!m_bStretchProportionalToParent) 
	{
		int newWidth = TB_LEFT_GUTTER + m_iMaxWidthPixels + TB_RIGHT_GUTTER + m_sbVertical->getWidth();
		setWidth(newWidth);
		
		//////////////////////
		CHILD* verticalSBChild = m_ListBoxWidget->getChild("VScroll");
		RectF vDestRect;
		RectF wndRect;
		RectF idealRect;
		wndRect.X = m_iLeft; wndRect.Y = m_iTop; wndRect.Width = getWidth(); wndRect.Height = getHeight();
		idealRect.X = verticalSBChild->posOffsets.x;
		idealRect.Y = verticalSBChild->posOffsets.y;
		idealRect.Width = verticalSBChild->posOffsets.w; 
		idealRect.Height = verticalSBChild->posOffsets.h;
		WWidgetManager::getDestinationRect(	vDestRect,
											m_ListBoxWidget->widgetSize.width,
											m_ListBoxWidget->widgetSize.height,
											&wndRect,
											&idealRect,
											verticalSBChild->align.eHAlign,
											verticalSBChild->align.eVAlign
											);
		//////////////////////
	}
}

void WListBox::setProportionalToParent(bool bValue) 
{
	m_bStretchProportionalToParent = bValue;

	if(m_bStretchProportionalToParent) 
	{
		m_maxY = getBottom() - m_sbHorizontal->getHeight();
		m_sbHorizontal->setVisible(true);
	}
	else 
	{
		m_maxY = getBottom() - TB_TOP_GUTTER;
		m_sbHorizontal->setVisible(false);
	}
}

void WListBox::setVScrollbarLength() 
{
	float _part = LINES_PER_PAGE*LINE_HEIGHT;
	float _total = m_Data.size()*LINE_HEIGHT;

	if(_total > 0) 
	{
		float _percentage = (_part / _total) * 100;

		if(_percentage <= 100) 
		{
			m_sbVertical->setLength(_percentage);
		}
	}
}

void WListBox::setHScrollbarLength() 
{
	float _part = m_ClientRect.Width;
	float _total = m_iMaxWidthPixels;
	float _percentage = (_part / _total) * 100;

	if(_percentage <= 100)
		m_sbHorizontal->setLength(_percentage);
}

void WListBox::drawStringFont(int x, int y, int anchor) 
{
	Rect destRect;

	int lineNo = abs(m_mainY)/LINE_HEIGHT;
	if(lineNo > m_Data.size()-1 || m_Data.size() <= 0)
		return;

	int X = 0, Y = 0;
	
	int xX = x;
	int yY = y - m_mainY;

	std::string str = getStringAt(lineNo);

	int i = 0;
	while(true) 
	{
		if(yY >= m_maxY)
			break;
		
		int CHAR_WIDTH = 0;
		char c = str[i];
		CHAR_WIDTH = WWidgetManager::getCharWidth(c);

		if(c > ' ') 
		{
			if(xX < m_maxX && xX >= m_minX) 
			{
				X = WWidgetManager::m_GlyphArray[c-32].uvCoords[0];
				Y = WWidgetManager::m_GlyphArray[c-32].uvCoords[1];
				
				WWidgetManager::setColor(0, 0, 0, 1.0f);
				WWidgetManager::drawFont(xX, yY, CHAR_WIDTH, WWidgetManager::CHARACTER_HEIGHT, X, Y);
				WWidgetManager::resetColor();
			}
		}
		else
		if(c == ' ' || c == '\t') 
		{
		}

		xX += CHAR_WIDTH;
		i++;

		if(i >= str.size()) 
		{
			if(lineNo == CURSOR_LINE_NO) //Current line Selection.
			{
				destRect.X = getLeft();
				destRect.Y = yY - TB_CURSOR_TOP;
				destRect.Width = (m_maxX - getLeft()) + WWidgetManager::CHARACTER_WIDTH;
				destRect.Height = LINE_HEIGHT + 2;

				WWidgetManager::getInstance()->fillRect(0.0f, 0.0f, 1.0f, 0.13f, &destRect);
			}

			lineNo++;
			if(lineNo >= m_Data.size())
				break;
			i = 0;
			yY += LINE_HEIGHT;
			xX = x;

			str = getStringAt(lineNo);
		}
	}
}

void WListBox::getCaretPos(int x, int y) 
{
	int lineNo = abs(y)/LINE_HEIGHT;
	if(lineNo >= m_Data.size() || (CURSOR_LINE_NO == 0 && y < 0))
		return;

	std::string str = getStringAt(lineNo);;

	int xX = 0;
	int yY = lineNo*LINE_HEIGHT;

	CURSOR_LINE_NO = lineNo;
	int i = 0;
	int caretPos = 0;

	if(str.size() == 0) 
	{
		m_CaretPosX = xX;
		m_CaretPosY = yY;
		CURSOR_LINE_NO = lineNo;
		COLUMN_NO = i;

		return;
	}

	while(true) 
	{	
		char c = str[i];
		{
			if(y >= yY && y < yY+LINE_HEIGHT) 
			{
				m_CaretPosX = xX;
				m_CaretPosY = yY;
				CURSOR_LINE_NO = lineNo;
				COLUMN_NO = i;

				return;
			}
		}

		xX += (WWidgetManager::CHARACTER_WIDTH + 0);
		i++;

		if(	i >= str.size()) 
		{
			//if(x >= xX) 
			{
				if(y >= yY && y < yY+LINE_HEIGHT) 
				{
					m_CaretPosX = xX;
					m_CaretPosY = yY;
					CURSOR_LINE_NO = lineNo;
					COLUMN_NO = i;

					return;
				}
			}
			
			break;
		}
	}

	return;
}

void WListBox::onUpdate(float deltaTimeMs) 
{
	updateScrollBarVisibility();

	if(!m_IsVScrolling) 
	{
		updateVBarPosition();
		setVScrollbarLength();
	}
	
	if(!m_IsHScrolling) 
	{
		updateHBarPosition();
		setHScrollbarLength();
	}
	
	/////////////// RE-ADJUST MAINS
	if(m_mainY > 0)
		m_mainY = 0;
	if(abs(m_mainY) > (m_Data.size()*LINE_HEIGHT - LINES_PER_PAGE*LINE_HEIGHT))
		m_mainY = -1 * (m_Data.size()*LINE_HEIGHT - LINES_PER_PAGE*LINE_HEIGHT);
	///////////////////////////////
}

void WListBox::updateScrollBarVisibility() 
{
	m_minX = getLeft() + TB_LEFT_GUTTER;
	m_minY = getTop() + TB_TOP_GUTTER;
	m_maxY = getBottom() - m_sbHorizontal->getHeight();

	LINES_PER_PAGE = (m_maxY-m_minY)/LINE_HEIGHT;

	///////// VERTICAL
	bool bVertSBVisibility = (m_Data.size() > LINES_PER_PAGE);
	m_sbVertical->setVisible(bVertSBVisibility);
	if(!bVertSBVisibility) 
	{
		m_ClientRect.Width = m_iClientRectW + m_sbVertical->getWidth();
		m_sbHorizontal->setWidth(m_iMaxHScrollbarWidth + m_sbVertical->getWidth());
	}
	else 
	{
		m_ClientRect.Width = m_iClientRectW;
		m_sbHorizontal->setWidth(m_iMaxHScrollbarWidth);
	}
	if(!bVertSBVisibility)	m_maxX = getRight() - TB_RIGHT_GUTTER;
	else					m_maxX = getRight() - m_sbVertical->getWidth() - TB_RIGHT_GUTTER;
	///////////////////////////////////////////////////

	///////// HORIZONTAL
	bool bHoriSBVisibility = (m_iMaxWidthPixels > m_ClientRect.Width);
	m_sbHorizontal->setVisible(bHoriSBVisibility);
	///////////////////////////////////////////////////
}

void WListBox::updateVBarPosition() 
{
	float _part = abs(m_mainY);
	float _total = m_Data.size()*LINE_HEIGHT;
	float _percentage = (_part / _total) * 100;

	m_sbVertical->setCursorPositionInPercent(_percentage);
}

void WListBox::updateHBarPosition() 
{
	float _part = abs(m_mainX);
	float _total = m_iMaxWidthPixels;
	float _percentage = (_part / _total) * 100;

	m_sbHorizontal->setCursorPositionInPercent(_percentage);
}

void WListBox::onRender() 
{
	WWidgetManager* renderer =  WWidgetManager::getInstance();

	RectF thisWndRect(getLeft(), getTop(), getWidth(), getHeight());
	CHILD* child = m_ListBoxWidget->getChild("TextArea");
	renderer->renderChild(m_ListBoxWidget, child, &thisWndRect);
	//renderer->renderClientArea(m_ListBoxWidget, 0, &m_ClientRect);

	thisWndRect.X = m_minX; thisWndRect.Y = m_minY; thisWndRect.Width = m_maxX - m_minX; thisWndRect.Height = m_maxY - m_minY;
	drawStringFont(m_minX + m_mainX, m_minY + m_mainY, 0);
}

void WListBox::onMouseEnterEx(int mCode, int x, int y, int prevX, int prevY) 
{
	
}

void WListBox::onMouseHoverEx(int mCode, int x, int y, int prevX, int prevY) 
{
	if(	x < m_minX || x > m_maxX
		||
		y < m_minY || y > m_maxY
		||	
		(getSize() <= 0)
	) {
			
	}
	else 
	{
		getCaretPos(x-m_minX-m_mainX, y-m_minY-m_mainY);
		setCaretDrawPosition();	
	}
}

void WListBox::onMouseLeaveEx(int mCode, int x, int y, int prevX, int prevY) 
{

}

void WListBox::onMouseDownEx(int x, int y, int iButton) 
{
	m_IsVScrolling = m_IsHScrolling = false;

	if(	x < m_minX || x > m_maxX
		||
		y < m_minY || y > m_maxY
		||	
		(getSize() <= 0)
	) {
		return;
	}

	getCaretPos(x-m_minX-m_mainX, y-m_minY-m_mainY);
	setCaretDrawPosition();
	
	if(m_pParent) 
	{
		m_pParent->onMessage((H_WND)this, MOUSE_DOWN, getComponentID(), 0);
	}
}

void WListBox::onMouseUpEx(int x, int y, int iButton) 
{
	if(	x < m_minX || x > m_maxX
		||
		y < m_minY || y > m_maxY
		||
		(getSize() <= 0)
	) {
			return;
	}

	if(m_pParent)
		m_pParent->onMessage((H_WND)this, MOUSE_UP, getComponentID(), 0);
}

void WListBox::onMouseMoveEx(int mCode, int x, int y, int prevX, int prevY) 
{
	
}

void WListBox::onMouseWheelEx(WPARAM wParam, LPARAM lParam) 
{
	if((getSize() <= 0))
		return;

	int fwKeys = GET_KEYSTATE_WPARAM(wParam);
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

	if(zDelta < 0) 
	{
		m_mainY -= LINE_HEIGHT;
	}
	else 
	{
		m_mainY += LINE_HEIGHT;
	}
}

void WListBox::onKeyBDownEx(unsigned int iVirtualKeycode, unsigned short ch) 
{
	if(getSize() <= 0)
		return;

	m_IsVScrolling = m_IsHScrolling = false;

	switch(iVirtualKeycode) 
	{
		case VK_PRIOR:
		{
			CURSOR_LINE_NO -= LINES_PER_PAGE;
			if(CURSOR_LINE_NO < 0)
				CURSOR_LINE_NO = 0;

			setCaretDrawPosition();
			updateMains();

			return;
		}
		break;
		case VK_NEXT:
		{
			CURSOR_LINE_NO += LINES_PER_PAGE;
			if(CURSOR_LINE_NO >= m_Data.size())
				CURSOR_LINE_NO = m_Data.size()-1;

			setCaretDrawPosition();
			updateMains();
			return;
		}
		break;
		case VK_UP:
		{
			CURSOR_LINE_NO--;
			if(CURSOR_LINE_NO < 0)
				CURSOR_LINE_NO = 0;
			
			setCaretDrawPosition();
			updateMains();

			return;
		}
		break;
		case VK_DOWN:
		{
			CURSOR_LINE_NO++;
			if(CURSOR_LINE_NO >= m_Data.size())
				CURSOR_LINE_NO = m_Data.size()-1;
			
			setCaretDrawPosition();
			updateMains();
			return;
		}
		break;
		case VK_HOME:
		{
			CURSOR_LINE_NO = 0;

			setCaretDrawPosition();
			updateMains();

			return;
		}
		break;
		case VK_END:
		{	
			CURSOR_LINE_NO = m_Data.size()-1;

			setCaretDrawPosition();
			updateMains();

			return;
		}
		break;
	}
}

void WListBox::updateMains() 
{
	//////////////////////////////////////////////////////////////
	int yPosTop = m_minY+(m_CaretPosY - TB_CURSOR_TOP)+m_mainY;
	if(yPosTop < m_minY) 
	{
		//m_mainY = -(m_CaretPosY/LINE_HEIGHT)*LINE_HEIGHT;
		//Or
		m_mainY = -m_CaretPosY;
	}

	int yPosBottom = m_minY + (m_CaretPosY + TB_CURSOR_BOTTOM) + m_mainY;
	if(yPosBottom >= m_maxY) 
	{
		m_mainY -= ((yPosBottom - m_maxY) + LINE_HEIGHT);
		m_mainY -= (m_mainY % LINE_HEIGHT);
	}
	//////////////////////////////////////////////////////////////
}

void WListBox::setCaretDrawPosition() 
{
	int xX = 0;
	int yY = CURSOR_LINE_NO*LINE_HEIGHT;

	m_CaretPosX = xX;
	m_CaretPosY = yY;
}

void WListBox::onKeyBUpEx(unsigned int iVirtualKeycode, unsigned short ch) 
{
	if(getSize() <= 0)
		return;
}

void WListBox::onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
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
							m_mainY += LINE_HEIGHT;
						}
						break;
						case BTN_SCROLLBAR_DOWN:
						{
							m_mainY -= LINE_HEIGHT;
						}
						break;
					}
				break;
				case ID_HORIZONTAL_SCROLLBAR:
					switch(lParam) 
					{
						case BTN_SCROLLBAR_LEFT:
						{
							m_mainX += WWidgetManager::CHARACTER_WIDTH;
							if(m_mainX > 0)
								m_mainX = 0;
						}
						break;
						case BTN_SCROLLBAR_RIGHT:
						{
							m_mainX -= WWidgetManager::CHARACTER_WIDTH;
							if(abs(m_mainX) > (m_iMaxWidthPixels - m_ClientRect.Width))
								m_mainX = -(m_iMaxWidthPixels - m_ClientRect.Width);
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
		case SCROLLER_POS_ON_DRAG:
		{
			int cursorPosInPercentage = (int)lParam;

			switch(wParam) 
			{
				case ID_VERTICAL_SCROLLBAR:
				{
					float scrollMaterialHeight = m_Data.size()*LINE_HEIGHT - LINES_PER_PAGE*LINE_HEIGHT;
					int mainYValue = (cursorPosInPercentage*scrollMaterialHeight)/100;

					m_mainY = -1*(mainYValue/LINE_HEIGHT)*LINE_HEIGHT;

					m_IsVScrolling = true;
				}
				break;
				case ID_HORIZONTAL_SCROLLBAR:
				{
					float scrollMaterialWidth = m_iMaxWidthPixels - m_ClientRect.Width;
					int mainXValue = (cursorPosInPercentage*scrollMaterialWidth)/100;

					m_mainX = -mainXValue;

					m_IsHScrolling = true;
				}
				break;
			}
		}
		break;
	}
}

void WListBox::setReadOnly(bool bRd) 
{
	mState = bRd?READONLY:NORMAL;
}

bool WListBox::getReadOnly() 
{
	return (mState == READONLY);
}

void WListBox::clearList() 
{
	m_Data.clear();
}

LRESULT WListBox::OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM__SETSIZE:
		{
			DWORD dwSize = (DWORD)lParam;
			{
				int32_t iWidth = (dwSize & 0xffff);
				int32_t iHeight = ((dwSize >> 16) & 0xffff);

				setSize(iWidth, iHeight);
				onCreateEx((LPVOID)0);
			}
		}
		break;
		case WM__RESIZE:
		{
			updateComponentPosition();
			onCreateEx((LPVOID)0);
			return CB_OKAY;
		}
		break;
	}

	return WContainer::OnSendMessage(msg, wParam, lParam);
}

WListBox::~WListBox() 
{
	int size = m_Data.size();
	while(size > 0) {
		removeItemAt(size-1);
		size = m_Data.size();
	}
}
#endif