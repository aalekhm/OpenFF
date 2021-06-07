#ifdef USE_YAGUI
#include "Engine/UI/WConsoleLog.h"
#include "Engine/UI/WComponentFactory.h"
#include "Engine/UI/WWidgetManager.h"
#include <algorithm>

using namespace std;

#define TB_LEFT_GUTTER			5
#define TB_TOP_GUTTER			5
#define TB_RIGHT_GUTTER			10

unsigned int WConsoleLog::LINE_HEIGHT;

WConsoleLog::WConsoleLog() 
: m_iMaxWidthPixels(0)
, m_iMaxHeightPixels(0)
{
	setIsContainer(true);
	m_iMainX = m_iMainY = 0;

	LINE_HEIGHT = WWidgetManager::CHARACTER_HEIGHT;
}

struct maxLineLength 
{
	bool operator() ( const std::string& str1, const std::string& str2 )
	{
		int s1 = str1.size();
		int s2 = str2.size();
		//printf("%d 0r %d\n", s1, s2);

		return (s1 < s2);
	}
};

H_WND WConsoleLog::Create(		const char* lpClassName, 
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
	WConsoleLog* pWConsoleLog = new WConsoleLog();
	pWConsoleLog->setText(lpWindowName);

	((WContainer*)pWConsoleLog)->Create(	lpClassName, 
											"WConsoleLog", 
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
	
	return pWConsoleLog;
}

void WConsoleLog::onCreateEx(LPVOID lpVoid) 
{
	H_WND hWnd = NULL;
	mState = NORMAL;
	m_CaretPosX = m_CaretPosY = 0;
	m_mainX = 0;
	m_mainY = 0;

	showLineNumbers(false);

	mState = NORMAL;

	m_IsVScrolling = m_IsHScrolling = false;
	SEL_CURSOR_LINE_NO = CURSOR_LINE_NO = 0;
	SEL_COLUMN_NO = COLUMN_NO = 0;
	m_bIsSelecting = false;

	dbStr = new char[255];
	memset(dbStr, 0, 255);

	///////////////////////////////////////////////////

	m_TextBoxWidget = WWidgetManager::getWidget("TextBox");

	CHILD* verticalSBChild = m_TextBoxWidget->getChild("VScroll");
	RectF destRect;
	RectF wndRect(m_iLeft, m_iTop, getWidth(), getHeight());
	RectF idealRect;

	idealRect.X = verticalSBChild->posOffsets.x;
	idealRect.Y = verticalSBChild->posOffsets.y;
	idealRect.Width = verticalSBChild->posOffsets.w; 
	idealRect.Height = verticalSBChild->posOffsets.h;
	WWidgetManager::getDestinationRect(	destRect,
										m_TextBoxWidget->widgetSize.width,
										m_TextBoxWidget->widgetSize.height,
										&wndRect,
										&idealRect,
										verticalSBChild->align.eHAlign,
										verticalSBChild->align.eVAlign
									);
	hWnd = 
	CreateComponent(	"WScrollbar", 
						"", 
						0, 
						destRect.X - m_iLeft, 
						destRect.Y - m_iTop,
						destRect.Width, 
						destRect.Height,
						this, 
						HMENU(ID_VERTICAL_SCROLLBAR), 
						(LPVOID)1);
	m_sbVertical = (WScrollbar*)hWnd;
	m_sbVertical->hasBG(true);
	m_sbVertical->setPostRender(true);
	m_iMaxVScrollbarHeight = destRect.Height;

	///////////////////////////////////////////////////

	CHILD* horizontalSBChild = m_TextBoxWidget->getChild("HScroll");
	idealRect.X = horizontalSBChild->posOffsets.x;
	idealRect.Y = horizontalSBChild->posOffsets.y;
	idealRect.Width = horizontalSBChild->posOffsets.w; 
	idealRect.Height = horizontalSBChild->posOffsets.h;
	WWidgetManager::getDestinationRect(	destRect,
										m_TextBoxWidget->widgetSize.width,
										m_TextBoxWidget->widgetSize.height,
										&wndRect,
										&idealRect,
										horizontalSBChild->align.eHAlign,
										horizontalSBChild->align.eVAlign
									);
	hWnd = 
	CreateComponent(	"WScrollbar", 
						"", 
						0, 
						destRect.X - m_iLeft, 
						destRect.Y - m_iTop,
						destRect.Width, 
						destRect.Height,
						this, 
						HMENU(ID_HORIZONTAL_SCROLLBAR), 
						(LPVOID)0);
	m_sbHorizontal = (WScrollbar*)hWnd;
	m_sbHorizontal->hasBG(true);
	m_sbHorizontal->setPostRender(true);
	m_iMaxHScrollbarWidth = destRect.Width;

	bool bHasClientArea = (m_TextBoxWidget->clientAreas.size() > 0);
	if(bHasClientArea) 
	{
		CLIENT_AREA* clientArea = m_TextBoxWidget->getClientAreaAt(0);
		RectF destClientRect;
		idealRect.X = clientArea->posOffsets.x;
		idealRect.Y = clientArea->posOffsets.y;
		idealRect.Width = clientArea->posOffsets.w; 
		idealRect.Height = clientArea->posOffsets.h;
		WWidgetManager::getDestinationRect(	destClientRect,
											m_TextBoxWidget->widgetSize.width,
											m_TextBoxWidget->widgetSize.height,
											&wndRect,
											&idealRect,
											clientArea->align.eHAlign,
											clientArea->align.eVAlign
										);
		m_ClientRect.X = destClientRect.X - getLeft();
		m_ClientRect.Y = destClientRect.Y - getTop();
		m_ClientRect.Width = destClientRect.Width;
		m_ClientRect.Height = destClientRect.Height;

		m_iClientRectW = m_ClientRect.Width;
		m_iClientRectH = m_ClientRect.Height;
	}

	showLineNumbers(true);

	///////////////////////////////////////////////////
	updateScrollBarVisibility();
	updateMains();
	///////////////////////////////////////////////////
}

void WConsoleLog::calculateMaxLineWidth() 
{
	std::vector<std::string>::iterator pos = std::max_element(m_Lines.begin(), m_Lines.end(), maxLineLength());
	const char* str = m_Lines[pos - m_Lines.begin()].c_str();
	m_iMaxWidthPixels = TB_LEFT_GUTTER + (WWidgetManager::getStringWidthTillPos(str, strlen(str)) + ((HAS_LINE_NO)?TB_LINE_NO_SPACE:0) ) + TB_RIGHT_GUTTER;
	m_iMaxHeightPixels = (m_Lines.size() + 1)*LINE_HEIGHT;
}

void WConsoleLog::setText(const char* str) 
{
	if(str != NULL && strlen(str) > 0) 
	{
		m_Lines.clear();
		appendText(str);
	}
}

void WConsoleLog::appendText(const char* str) 
{
	int startPos = 0, endPos = 0;
	std::string strBuff = str;
	
	while(true) 
	{
		endPos = strBuff.find_first_of("\r\n", startPos);
		if (endPos < 0)
		{
			break;
		}

		m_Lines.push_back(strBuff.substr(startPos, endPos-startPos));

		if (strBuff[endPos] == '\r')
		{
			endPos++;
		}

		startPos = endPos+1;
	}
	
	if (startPos < strBuff.size())
	{
		m_Lines.push_back(strBuff.substr(startPos, strBuff.size() - startPos));
	}

	strBuff.~basic_string();
}

void WConsoleLog::setVScrollbarLength() 
{
	float _part = m_ClientRect.Height;
	float _total = m_iMaxHeightPixels;
	float _percentage = (_part / _total) * 100;

	if (_percentage <= 100)
	{
		m_sbVertical->setLength(_percentage);
	}
}

void WConsoleLog::setHScrollbarLength() 
{
	float _part = m_ClientRect.Width;
	float _total = m_iMaxWidthPixels;
	float _percentage = (_part / _total) * 100;

	if (_percentage <= 100)
	{
		m_sbHorizontal->setLength(_percentage);
	}
}

void WConsoleLog::drawStringFont(int x, int y, int anchor) 
{
	Rect rect;

	int lineNo = abs(m_mainY)/LINE_HEIGHT;
	if (lineNo > m_Lines.size() - 1)
	{
		return;
	}

	int X = 0, Y = 0;
	
	int xX = x;
	int yY = y - m_mainY;

	int i = 0;
	while(true) 
	{
		bool bDrawSelect = false;
		if (yY >= m_maxY)
		{
			break;
		}
		
		int CHAR_WIDTH = 0;
		char c = ' ';

		bool bSkipRestLine = false;

		if(m_Lines[lineNo].size() > 0) 
		{
			c = m_Lines[lineNo].at(i);
			CHAR_WIDTH = WWidgetManager::getCharWidth(c);
			
			int iRightTop = (xX + CHAR_WIDTH);
			int iLeftTop = xX;
			if(	(iLeftTop < m_maxX && iLeftTop >= m_minX) || ( iRightTop < m_maxX && iRightTop >= m_minX )) 
			{
				if(c > ' ') 
				{
					X = WWidgetManager::m_GlyphArray[c-32].uvCoords[0];
					Y = WWidgetManager::m_GlyphArray[c-32].uvCoords[1];

					WWidgetManager::setColor(0, 0, 0, 1.0f);
					WWidgetManager::drawFont(xX, yY, CHAR_WIDTH, WWidgetManager::CHARACTER_HEIGHT, X, Y);
					WWidgetManager::resetColor();

					if (m_bIsSelecting)
					{
						bDrawSelect = isUnderSelection(lineNo, i);
					}
				}
				else
				if(c == ' ' || c == '\t') 
				{
					if (m_bIsSelecting)
					{
						bDrawSelect = isUnderSelection(lineNo, i);
					}
				}

				if(bDrawSelect) 
				{
					Rect rect(xX, yY, CHAR_WIDTH, LINE_HEIGHT);
					WWidgetManager::getInstance()->fillRect(0.5f, 0.5f, 0.5f, 0.5f, &rect);
				}
			}

			if(iLeftTop >= m_maxX || iRightTop >= m_maxX) 
			{
				bSkipRestLine = true;
			}

			xX += CHAR_WIDTH;
		}

		i++;

		if(i >= m_Lines[lineNo].size() || bSkipRestLine) 
		{
			bSkipRestLine = false;
			if(lineNo == CURSOR_LINE_NO) //Current line Selection.
			{
				rect.X = m_minX;
				rect.Y = yY;
				rect.Width = (m_maxX - m_minX) + CHAR_WIDTH;
				rect.Height = LINE_HEIGHT;
				
				WWidgetManager::getInstance()->fillRect(0.0f, 0.0f, 1.0f, 0.13f, &rect);
			}

			lineNo++;
			if (lineNo >= m_Lines.size())
			{
				break;
			}

			i = 0;
			yY += LINE_HEIGHT;
			xX = x;
		}
	}
}

char WConsoleLog::peek(int iLineNo, int iCurrPos, int iLineLength, int count) 
{
	if (iCurrPos + count >= iLineLength)
	{
		return -1;
	}
	else
	{
		return m_Lines[iLineNo].at(iCurrPos + count);
	}
}

char WConsoleLog::consume(int iLineNo, int* iCurrPos, int iLineLength, int iHowMany) 
{
	char ret;
	*iCurrPos += iHowMany;	

	ret = m_Lines[iLineNo].at(*iCurrPos);

	return ret;
}

bool WConsoleLog::isUnderSelection(int lineNo, int column) 
{
	bool bRet = false;

	if(lineNo == CURSOR_LINE_NO && lineNo == SEL_CURSOR_LINE_NO) 
	{
		if(COLUMN_NO > SEL_COLUMN_NO) 
		{
			if (column >= SEL_COLUMN_NO && column < COLUMN_NO)
			{
				bRet = true;
			}
		}
		if(COLUMN_NO < SEL_COLUMN_NO) 
		{
			if (column >= COLUMN_NO && column < SEL_COLUMN_NO)
			{
				bRet = true;
			}
		}
	}
	else
	if(CURSOR_LINE_NO > SEL_CURSOR_LINE_NO) 
	{
		if(	lineNo == SEL_CURSOR_LINE_NO && column >= SEL_COLUMN_NO
			||
			lineNo == CURSOR_LINE_NO && column < COLUMN_NO
			||
			lineNo > SEL_CURSOR_LINE_NO && lineNo < CURSOR_LINE_NO
		) {
			bRet = true;
		}
	}
	else
	if(CURSOR_LINE_NO < SEL_CURSOR_LINE_NO) 
	{
		if(	lineNo == CURSOR_LINE_NO && column >= COLUMN_NO
			||
			lineNo == SEL_CURSOR_LINE_NO && column < SEL_COLUMN_NO
			||
			lineNo > CURSOR_LINE_NO && lineNo < SEL_CURSOR_LINE_NO
		) {
			bRet = true;
		}
	}

	return bRet;
}

void WConsoleLog::getCaretPos(int x, int y) 
{
	int lineNo = abs(y)/LINE_HEIGHT;
	if (lineNo >= m_Lines.size() || (CURSOR_LINE_NO == 0 && y < 0))
	{
		return;
	}

	std::string str = m_Lines[lineNo].c_str();

	int xX = 0;
	int yY = lineNo*LINE_HEIGHT;

	CURSOR_LINE_NO = lineNo;
	int i = 0;
	int caretPos = 0;

	if(str.length() == 0) 
	{
		m_CaretPosX = xX;
		m_CaretPosY = yY;
		CURSOR_LINE_NO = lineNo;
		COLUMN_NO = i;

		return;
	}

	int CHAR_WIDTH = 0;
	while(true) 
	{	
		char c = str[i];
		CHAR_WIDTH = WWidgetManager::getCharWidth(c);

		if(x >= xX && x <= xX + CHAR_WIDTH) 
		{
			if(y >= yY && y < yY + LINE_HEIGHT) 
			{
				m_CaretPosX = xX;
				m_CaretPosY = yY;
				CURSOR_LINE_NO = lineNo;
				COLUMN_NO = i;

				return;
			}
		}

		xX += CHAR_WIDTH;
		i++;

		if(	i >= str.length()) 
		{
			if(x >= xX) 
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

void WConsoleLog::onUpdate(float deltaTimeMs) 
{
	setVScrollbarLength();
	setHScrollbarLength();

	updateScrollBarVisibility();

	if(!m_IsVScrolling)	updateVBarPosition();
	if(!m_IsHScrolling)	updateHBarPosition();
	
	/////////////// RE-ADJUST MAINS
	if(m_mainY > 0)	m_mainY = 0;
	if (abs(m_mainY) > (m_Lines.size() * LINE_HEIGHT - LINES_PER_PAGE * LINE_HEIGHT))
	{
		m_mainY = -1 * (m_Lines.size() * LINE_HEIGHT - LINES_PER_PAGE * LINE_HEIGHT);
	}
	//if(m_iMaxHeightPixels> m_ClientRect.Height) {
	//	if(abs(m_mainY) > (m_iMaxHeightPixels - m_ClientRect.Height))
	//		m_mainY = -(m_iMaxHeightPixels - m_ClientRect.Height);
	//}

	if(m_mainX > 0)	m_mainX = 0;
	if(m_iMaxWidthPixels > m_ClientRect.Width) 
	{
		if (abs(m_mainX) > (m_iMaxWidthPixels - m_ClientRect.Width))
		{
			m_mainX = -(m_iMaxWidthPixels - m_ClientRect.Width);
		}
	}
	///////////////////////////////

	PREV_CURSOR_LINE_NO = CURSOR_LINE_NO;
}

void WConsoleLog::updateScrollBarVisibility() 
{
	m_minX = getLeft() + ((HAS_LINE_NO)?TB_LINE_NO_SPACE:0) + TB_LEFT_GUTTER;
	m_minY = getTop() + TB_TOP_GUTTER;

	///*
	///////// VERTICAL
	bool bVertSBVisibility = (m_iMaxHeightPixels > m_ClientRect.Height);
	m_sbVertical->setVisible(bVertSBVisibility);
	if(!bVertSBVisibility) 
	{
		//m_ClientRect.Width = m_iClientRectW + m_sbVertical->getWidth();
		m_sbHorizontal->setWidth(m_iMaxHScrollbarWidth + m_sbVertical->getWidth());
	}
	else 
	{
		//m_ClientRect.Width = m_iClientRectW;
		m_sbHorizontal->setWidth(m_iMaxHScrollbarWidth);
	}
	if(!bVertSBVisibility)	m_maxX = getRight() - TB_RIGHT_GUTTER;
	else					m_maxX = getRight() - m_sbVertical->getWidth() - TB_RIGHT_GUTTER;
	///////////////////////////////////////////////////

	///////// HORIZONTAL
	bool bHoriSBVisibility = (m_iMaxWidthPixels > m_ClientRect.Width);
	m_sbHorizontal->setVisible(bHoriSBVisibility);
	if(!bHoriSBVisibility) 
	{
		//m_ClientRect.Height = m_iClientRectH + m_sbHorizontal->getHeight();
		m_sbVertical->setHeight(m_iMaxVScrollbarHeight + m_sbHorizontal->getHeight());
	}
	else 
	{
		//m_ClientRect.Height = m_iClientRectH;
		m_sbVertical->setHeight(m_iMaxVScrollbarHeight);
	}
	if(!bHoriSBVisibility)	m_maxY = getBottom() - TB_LEFT_GUTTER;
	else					m_maxY = getBottom() - m_sbHorizontal->getHeight() - TB_LEFT_GUTTER;
	LINES_PER_PAGE = (m_maxY-m_minY)/LINE_HEIGHT;
	///////////////////////////////////////////////////
	//*/
}

void WConsoleLog::updateVBarPosition() 
{
	float _part = abs(m_mainY);
	float _total = m_iMaxHeightPixels;
	float _percentage = (_part / _total) * 100;

	m_sbVertical->setCursorPositionInPercent(_percentage);
}

void WConsoleLog::updateHBarPosition() 
{
	float _part = abs(m_mainX);
	float _total = m_iMaxWidthPixels;
	float _percentage = (_part / _total) * 100;

	m_sbHorizontal->setCursorPositionInPercent(_percentage);
}

void WConsoleLog::onRender() 
{
	WWidgetManager* renderer =  WWidgetManager::getInstance();
	Rect thisWndRect(getLeft(), getTop(), getWidth(), getHeight());

	renderer->fillRect(1.0f, 1.0f, 1.0f, 1.0f, &thisWndRect);
	renderer->drawRect(0.0f, 0.0f, 0.0f, 1.0f, &thisWndRect);

	////////////////////////////////////////////////////////
	setClip(getLeft(), getTop(), getWidth(), getHeight());
	{	
			CHILD* child = m_TextBoxWidget->getChild("TextArea");
			//renderer->renderChild(m_TextBoxWidget, child, &thisWndRect);

			Rect clientRect(getLeft() + m_ClientRect.X, getTop() + m_ClientRect.Y, m_ClientRect.Width, m_ClientRect.Height);
			renderer->fillRect(1.0, 1.0f, 0.0f, 1.0f, &clientRect);

			if(mState == READONLY) 
			{
				Rect readOnlyRect(thisWndRect.X, thisWndRect.Y, thisWndRect.Width, thisWndRect.Height);
				renderer->fillRect(0.5f, 0.5f, 0.5f, 1.0f, &readOnlyRect);
			}
	}
	resetClip();
	////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////
	setClip(m_minX, m_minY, m_maxX - m_minX, m_maxY - m_minY);
	{	
		drawStringFont(m_minX + m_mainX, m_minY + m_mainY, 0);
	}
	resetClip();
	////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////
	setClip(getLeft(), getTop(), getWidth(), getHeight());
	{
		drawLineNumbers();

		if(hasKeyFocus()) 
		{
			WWidgetManager::drawVerticalLine(	m_minX + m_mainX + m_CaretPosX, 
												m_minY + m_mainY + m_CaretPosY,
												m_minX + m_mainX + m_CaretPosX, 
												m_minY + m_mainY + m_CaretPosY + LINE_HEIGHT);
		}
		
		setSelection();
	}
	resetClip();
	////////////////////////////////////////////////////////
}

void WConsoleLog::drawLineNumbers() 
{
	if(HAS_LINE_NO) 
	{
		Rect rect(getLeft(), getTop(), TB_LINE_NO_SPACE, getHeight());
		WWidgetManager::getInstance()->fillRect(0.5f, 0.5f, 0.5f, 0.5f, &rect);

		int startLineNo = abs(m_mainY)/LINE_HEIGHT;
		int endLineNo = (m_maxY-m_minY)/LINE_HEIGHT;
		for(int i = 0; i <= endLineNo; i++) 
		{
			if ((startLineNo + i) > m_Lines.size() - 1)
			{
				break;
			}

			memset(dbStr, 0, 255);
			sprintf(dbStr, "%d", (startLineNo+i));
			WWidgetManager::setColor(0, 0, 0, 1.0f);
			WWidgetManager::drawStringFont(dbStr, getLeft() + TB_LINE_NO_SPACE - TB_LEFT_GUTTER, m_minY+(i*LINE_HEIGHT), 2);
			WWidgetManager::resetColor();
		}
	}
}

void WConsoleLog::setClip(int x, int y , int width, int height) 
{
	WWidgetManager::GetClipBounds(&m_reclaimRect);

	RectF clipRect(x, y, width, height);
	RectF::Intersect(clipRect, m_reclaimRect, clipRect);

	WWidgetManager::setClip(clipRect.X, clipRect.Y, clipRect.Width, clipRect.Height);
}

void WConsoleLog::resetClip() 
{
	WWidgetManager::setClip(m_reclaimRect.X, m_reclaimRect.Y, m_reclaimRect.Width, m_reclaimRect.Height);
}

void WConsoleLog::setSelection() 
{	
	if(m_bIsSelecting) 
	{
	}
	else 
	{
		SEL_CURSOR_LINE_NO = CURSOR_LINE_NO;
		SEL_COLUMN_NO = COLUMN_NO;
	}
}

void WConsoleLog::onMouseDownEx(int x, int y, int iButton) 
{
	m_IsVScrolling = m_IsHScrolling = false;

	if(	x < m_minX || x > m_maxX
		||
		y < m_minY || y > m_maxY
	) {
		return;
	}

	WWidgetManager::setCursor(IDC__IBEAM);
	if(m_bIsSelecting && !GetAsyncKeyState(VK_SHIFT)) 
	{
		m_bIsSelecting = false;
	}	

	getCaretPos(x-m_minX-m_mainX, y-m_minY-m_mainY);
	setCaretDrawPosition();
}

void WConsoleLog::onMouseUpEx(int x, int y, int iButton) 
{
	if(x < m_minX || x > m_maxX
		||
		y < m_minY || y > m_maxY
	) {
		return;
	}

	WWidgetManager::setCursor(IDC__IBEAM);
}

void WConsoleLog::onMouseEnterEx(int mCode, int x, int y, int prevX, int prevY) 
{

}

void WConsoleLog::onMouseHoverEx(int mCode, int x, int y, int prevX, int prevY) 
{
	if(x < m_minX || x > m_maxX
		||
		y < m_minY || y > m_maxY
	) {

	}
	else
		WWidgetManager::setCursor(IDC__IBEAM);
}

void WConsoleLog::onMouseLeaveEx(int mCode, int x, int y, int prevX, int prevY) 
{
	WWidgetManager::setCursor(IDC__ARROW);
}

void WConsoleLog::onMouseMoveEx(int mCode, int x, int y, int prevX, int prevY) 
{	
	if (m_IsVScrolling || m_IsHScrolling)
	{
		return;
	}

	bool bHasHScrollBar = (m_iMaxWidthPixels > m_ClientRect.Width);
	if(	(	x < m_minX || x > m_maxX
			||
			y < m_minY || y > m_maxY
		)
		&&
		bHasHScrollBar
	) {
			return;
	}

	WWidgetManager::setCursor(IDC__IBEAM);
	bool LEFT_MOUSE_DOWN = WWidgetManager::getInstance()->isMousePressed(GLFW_MOUSE_BUTTON_LEFT);
	bool RIGHT_MOUSE_DOWN = WWidgetManager::getInstance()->isMousePressed(GLFW_MOUSE_BUTTON_RIGHT);

	if(LEFT_MOUSE_DOWN)
	{
		m_bIsSelecting = true;

		if(m_bIsSelecting) 
		{
			getCaretPos(x-m_minX-m_mainX, y-m_minY-m_mainY);

			setCaretDrawPosition();
			updateMains();
		}
	}
	else
	if(RIGHT_MOUSE_DOWN)
	{
		m_mainX += (x - prevX);
		if (m_mainX > 0)
		{
			m_mainX = 0;
		}
		
		m_mainY += (y - prevY);
		if (m_mainY > 0)
		{
			m_mainY = 0;
		}
	}
}

void WConsoleLog::onMouseWheelEx(WPARAM wParam, LPARAM lParam) 
{
	int fwKeys = GET_KEYSTATE_WPARAM(wParam);
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

	if(zDelta < 0) 
	{
		m_mainY -= LINE_HEIGHT;
		if (abs(m_mainY) > (m_Lines.size() * LINE_HEIGHT - LINES_PER_PAGE * LINE_HEIGHT))
		{
			m_mainY = -1 * (m_Lines.size() * LINE_HEIGHT - LINES_PER_PAGE * LINE_HEIGHT);
		}
	}
	else 
	{
		m_mainY += LINE_HEIGHT;
		if (m_mainY > 0)
		{
			m_mainY = 0;
		}
	}
}

bool WConsoleLog::isReadOnlyChar(char ch) 
{
	bool bRet = false;
	if(	(GetAsyncKeyState(VK_CONTROL) && (ch == 'C' || ch == 'c'))
		||
		ch == VK_PRIOR
		||
		ch == VK_NEXT
		||
		ch == VK_LEFT
		||
		ch == VK_RIGHT
		||
		ch == VK_UP
		||
		ch == VK_DOWN
		||
		ch == VK_HOME
		||
		ch == VK_END
	)
		bRet = true;

	return bRet;
}

void WConsoleLog::onKeyBDownEx(unsigned int iVirtualKeycode, unsigned short ch) 
{
	WWidgetManager* pWidgetManager = WWidgetManager::getInstance();
	bool CONTROL_ON = pWidgetManager->isModifierKeyDown(GLFW_MOD_CONTROL);
	bool SHIFT_ON = pWidgetManager->isModifierKeyDown(GLFW_MOD_SHIFT);

	if (mState == READONLY && !isReadOnlyChar(iVirtualKeycode))
	{
		return;
	}
	else
	if(CONTROL_ON && SHIFT_ON && iVirtualKeycode == GLFW_KEY_LEFT)
	{
		showLineNumbers(!HAS_LINE_NO);
	}

	m_IsVScrolling = m_IsHScrolling = false;

	std::string leftHalfSubstr;
	std::string rightHalfSubstr;

	if (CONTROL_ON)
	{
		if (iVirtualKeycode == 'X' || iVirtualKeycode == 'C')
		{
			if(GetAsyncKeyState(VK_CONTROL)) 
			{
				if(OpenClipboard(GetForegroundWindow())) 
				{
					bool isCut = (iVirtualKeycode == 'X');

					std::string leftStrUnderSelection;
					std::string rightStrUnderSelection;
					std::string strUnderSelection;

					if(CURSOR_LINE_NO == SEL_CURSOR_LINE_NO) 
					{
						std::string middleString;

						if(COLUMN_NO > SEL_COLUMN_NO) 
						{
							leftHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(0, SEL_COLUMN_NO).c_str();
							rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();
							middleString = m_Lines[CURSOR_LINE_NO].substr(SEL_COLUMN_NO, (COLUMN_NO-SEL_COLUMN_NO)).c_str();
							
							if(isCut) COLUMN_NO = SEL_COLUMN_NO;
						}
						else 
						{
							leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
							rightHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(SEL_COLUMN_NO).c_str();
							middleString = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO, (SEL_COLUMN_NO-COLUMN_NO)).c_str();
						}
						
						if(isCut) 
						{
							m_Lines[CURSOR_LINE_NO] = "";
							m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
							m_Lines[CURSOR_LINE_NO] += rightHalfSubstr.c_str();
							m_bIsSelecting = false;
						}
						
						WWidgetManager::m_clipboardTextData = middleString.c_str();
					}
					else
					if(CURSOR_LINE_NO > SEL_CURSOR_LINE_NO) 
					{
						leftHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(0, SEL_COLUMN_NO).c_str();
						rightStrUnderSelection = m_Lines[SEL_CURSOR_LINE_NO].substr(SEL_COLUMN_NO).c_str();

						rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();
						leftStrUnderSelection = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
							
						strUnderSelection += rightStrUnderSelection.c_str();
						strUnderSelection += "\r\n";
						for(int ii = 0; ii < (CURSOR_LINE_NO - SEL_CURSOR_LINE_NO); ii++) 
						{
							if(ii == (CURSOR_LINE_NO-SEL_CURSOR_LINE_NO)-1)
								continue;
							strUnderSelection += m_Lines[SEL_CURSOR_LINE_NO + 1 + ii].c_str();
							strUnderSelection += "\r\n";
						}
						strUnderSelection += leftStrUnderSelection.c_str();

						WWidgetManager::m_clipboardTextData = strUnderSelection.c_str();

						if(isCut) 
						{
							for(int ii = 0; ii < (CURSOR_LINE_NO-SEL_CURSOR_LINE_NO); ii++) 
							{
								m_Lines[SEL_CURSOR_LINE_NO + 1].~basic_string();
								m_Lines.erase(m_Lines.begin() + SEL_CURSOR_LINE_NO + 1);
							}

							m_Lines[SEL_CURSOR_LINE_NO] = "";
							m_Lines[SEL_CURSOR_LINE_NO] += leftHalfSubstr.c_str();
							m_Lines[SEL_CURSOR_LINE_NO] += rightHalfSubstr.c_str();

							m_bIsSelecting = false;
							CURSOR_LINE_NO = SEL_CURSOR_LINE_NO;
							COLUMN_NO = SEL_COLUMN_NO;
						}
					}
					else 
					if(CURSOR_LINE_NO < SEL_CURSOR_LINE_NO) 
					{
						leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
						rightStrUnderSelection = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();

						rightHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(SEL_COLUMN_NO).c_str();
						leftStrUnderSelection = m_Lines[SEL_CURSOR_LINE_NO].substr(0, SEL_COLUMN_NO).c_str();
						
						strUnderSelection += rightStrUnderSelection.c_str();
						strUnderSelection += "\r\n";
						for(int ii = 1; ii < (SEL_CURSOR_LINE_NO-CURSOR_LINE_NO); ii++) 
						{
							if(ii == (CURSOR_LINE_NO-SEL_CURSOR_LINE_NO)-1)
								continue;
							strUnderSelection += m_Lines[CURSOR_LINE_NO + ii].c_str();
							strUnderSelection += "\r\n";
						}
						strUnderSelection += leftStrUnderSelection.c_str();

						WWidgetManager::m_clipboardTextData = strUnderSelection.c_str();
						
						if(isCut) 
						{
							for(int ii = 0; ii < (SEL_CURSOR_LINE_NO-CURSOR_LINE_NO); ii++) 
							{
								m_Lines[CURSOR_LINE_NO + 1].~basic_string();
								m_Lines.erase(m_Lines.begin() + CURSOR_LINE_NO + 1);
							}

							m_Lines[CURSOR_LINE_NO] = "";
							m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
							m_Lines[CURSOR_LINE_NO] += rightHalfSubstr.c_str();

							m_bIsSelecting = false;
						}
					}

					setCaretDrawPosition();
					updateMains();
				}
			}

			return;
		}
		if (iVirtualKeycode == 'V')
		{
			//if(OpenClipboard(GetForegroundWindow())) 
			{
				if(m_bIsSelecting)
					deleteSelectedText();

				pasteText();
					
				setCaretDrawPosition();
				updateMains();

				return;
			}
		}
	}

	if (iVirtualKeycode == GLFW_KEY_PAGE_UP)
	{
		CURSOR_LINE_NO -= LINES_PER_PAGE;
		if(CURSOR_LINE_NO < 0)
			CURSOR_LINE_NO = 0;

		std::string str = m_Lines[CURSOR_LINE_NO].c_str();
		if(COLUMN_NO > str.length())
			COLUMN_NO = str.length();

		setCaretDrawPosition();
		updateMains();

		return;
	}

	if (iVirtualKeycode == GLFW_KEY_PAGE_DOWN)
	{
		CURSOR_LINE_NO += LINES_PER_PAGE;
		if(CURSOR_LINE_NO >= m_Lines.size())
			CURSOR_LINE_NO = m_Lines.size()-1;

		std::string str = m_Lines[CURSOR_LINE_NO].c_str();
		if(COLUMN_NO > str.length())
			COLUMN_NO = str.length();

		setCaretDrawPosition();
		updateMains();
		return;
	}

	if (iVirtualKeycode == GLFW_KEY_UP)
	{
		scrollVertically(-1);
		return;
	}

	if (iVirtualKeycode == GLFW_KEY_DOWN)
	{
		scrollVertically(1);
		return;
	}

	if (iVirtualKeycode == GLFW_KEY_LEFT)
	{
		bool bWasSelecting = m_bIsSelecting;
		m_bIsSelecting = GetAsyncKeyState(VK_SHIFT);
			
		std::string str = m_Lines[CURSOR_LINE_NO].c_str();
		/* Move Caret */
		if(CONTROL_ON)
		{
			if(COLUMN_NO > 0)
				COLUMN_NO = WWidgetManager::getNextWhitespacePos(str.c_str(), COLUMN_NO-1, true);
			else
				COLUMN_NO = -1;
		}
		else 
		{
			COLUMN_NO--;
		}
			
		if(COLUMN_NO < 0) 
		{
			CURSOR_LINE_NO--;
			bool bIsFirstLine = (CURSOR_LINE_NO < 0);
			if(bIsFirstLine) 
			{
				CURSOR_LINE_NO = 0;
				COLUMN_NO = 0;
			}
			else 
			{
				COLUMN_NO = m_Lines[CURSOR_LINE_NO].size();
			}
		}
			
		/* Set Caret Position & Update Viewport*/
		setCaretDrawPosition();
		updateMains();

		return;
	}

	if (iVirtualKeycode == GLFW_KEY_RIGHT)
	{
		int prevColumnNo = COLUMN_NO;
		bool bWasSelecting = m_bIsSelecting;
		m_bIsSelecting = GetAsyncKeyState(VK_SHIFT);
			
		std::string str = m_Lines[CURSOR_LINE_NO].c_str();
		/* Move Caret */
		if(CONTROL_ON) 
		{
			if(COLUMN_NO < str.length()-1)
				COLUMN_NO = WWidgetManager::getNextWhitespacePos(str.c_str(), COLUMN_NO, false) + 1;
			else
				COLUMN_NO = str.length()+1;
		}
		else 
		{
			COLUMN_NO++;
		}
			
		if(COLUMN_NO > str.length()) 
		{
			COLUMN_NO = 0;
			CURSOR_LINE_NO++;
			if(CURSOR_LINE_NO >= m_Lines.size()) 
			{
				CURSOR_LINE_NO = m_Lines.size()-1;
				COLUMN_NO = prevColumnNo;
			}
		}
			
		/* Set Caret Position & Update Viewport*/
		setCaretDrawPosition();
		updateMains();

		return;
	}

	if (iVirtualKeycode == GLFW_KEY_HOME)
	{
		m_bIsSelecting = GetAsyncKeyState(VK_SHIFT);

		COLUMN_NO = 0;

		if(GetAsyncKeyState(VK_CONTROL))
			CURSOR_LINE_NO = 0;

		setCaretDrawPosition();
		updateMains();

		return;
	}

	if (iVirtualKeycode == GLFW_KEY_END)
	{	
		bool bCtrl = CONTROL_ON;
		m_bIsSelecting = SHIFT_ON;
			
		std::string str = m_Lines[CURSOR_LINE_NO].c_str();
		COLUMN_NO = str.length();

		if(bCtrl) 
		{
			CURSOR_LINE_NO = m_Lines.size()-1;
			std::string str = m_Lines[CURSOR_LINE_NO].c_str();
			COLUMN_NO = str.length();
		}

		setCaretDrawPosition();
		updateMains();

		return;
	}

	if (iVirtualKeycode == GLFW_KEY_BACKSPACE)
	{
		bool bWasSelecting = m_bIsSelecting;
		if(m_bIsSelecting) 
		{
			deleteSelectedText();
		}
		else 
		{
			if(COLUMN_NO > 0) 
			{
				leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO-1).c_str();
				rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();

				if(rightHalfSubstr.length() > 0) 
				{
					m_Lines[CURSOR_LINE_NO] = "";
					m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
					m_Lines[CURSOR_LINE_NO] += rightHalfSubstr.c_str();
				}
				else 
				{
					m_Lines[CURSOR_LINE_NO].assign(leftHalfSubstr.c_str());
				}

				COLUMN_NO--;
			}
			else 
			{
				if(CURSOR_LINE_NO > 0) 
				{
					rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();

					m_Lines[CURSOR_LINE_NO].~basic_string();
					m_Lines.erase(m_Lines.begin() + CURSOR_LINE_NO);

					CURSOR_LINE_NO--;
					COLUMN_NO = m_Lines[CURSOR_LINE_NO].size();
					m_Lines[CURSOR_LINE_NO].append(rightHalfSubstr.c_str());
				}
			}
		}
			
		setCaretDrawPosition();
		updateMains();
			
		return;
	}

	if (iVirtualKeycode == GLFW_KEY_DELETE)
	{
		bool bWasSelecting = m_bIsSelecting;
		if(m_bIsSelecting) 
		{
			deleteSelectedText();
		}
		else 
		{
			if(COLUMN_NO < m_Lines[CURSOR_LINE_NO].size()) 
			{
				leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
				rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO+1).c_str();

				if(rightHalfSubstr.length() > 0) 
				{
					m_Lines[CURSOR_LINE_NO] = "";
					m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
					m_Lines[CURSOR_LINE_NO] += rightHalfSubstr.c_str();
				}
				else
					m_Lines[CURSOR_LINE_NO].assign(leftHalfSubstr.c_str());
			}
			else 
			{
				if(CURSOR_LINE_NO < m_Lines.size()-1) 
				{
					leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
					rightHalfSubstr = m_Lines[CURSOR_LINE_NO+1].c_str();
					leftHalfSubstr += rightHalfSubstr.c_str();
					m_Lines[CURSOR_LINE_NO].append(leftHalfSubstr.c_str());
						
					m_Lines[CURSOR_LINE_NO + 1].~basic_string();
					m_Lines.erase(m_Lines.begin() + CURSOR_LINE_NO + 1);
				}
			}
		}
			
		setCaretDrawPosition();
		updateMains();
			
		return;
	}

	if (ch >= ' ' && ch <= '~')
	{
		bool bWasSelecting = m_bIsSelecting;
		if(m_bIsSelecting) 
		{
			if(CURSOR_LINE_NO == SEL_CURSOR_LINE_NO) 
			{
				if(COLUMN_NO > SEL_COLUMN_NO) 
				{
					leftHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(0, SEL_COLUMN_NO).c_str();
					rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();

					m_Lines[SEL_CURSOR_LINE_NO] = "";
					m_Lines[SEL_CURSOR_LINE_NO] += leftHalfSubstr.c_str();
					m_Lines[SEL_CURSOR_LINE_NO] += rightHalfSubstr.c_str();
					COLUMN_NO = SEL_COLUMN_NO;
				}
				else 
				{
					leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
					rightHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(SEL_COLUMN_NO).c_str();

					m_Lines[SEL_CURSOR_LINE_NO] = "";
					m_Lines[SEL_CURSOR_LINE_NO] += leftHalfSubstr.c_str();
					m_Lines[SEL_CURSOR_LINE_NO] += rightHalfSubstr.c_str();
				}
				
				m_bIsSelecting = false;
			}
			else
			if(CURSOR_LINE_NO > SEL_CURSOR_LINE_NO) 
			{
				leftHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(0, SEL_COLUMN_NO).c_str();
				rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();
				
				m_Lines[SEL_CURSOR_LINE_NO] = "";
				m_Lines[SEL_CURSOR_LINE_NO] += leftHalfSubstr.c_str();
				m_Lines[SEL_CURSOR_LINE_NO] += rightHalfSubstr.c_str();

				for(int ii = 0; ii < (CURSOR_LINE_NO-SEL_CURSOR_LINE_NO); ii++) 
				{
					m_Lines[SEL_CURSOR_LINE_NO + 1].~basic_string();
					m_Lines.erase(m_Lines.begin() + SEL_CURSOR_LINE_NO + 1);
				}

				COLUMN_NO = SEL_COLUMN_NO;
				CURSOR_LINE_NO = SEL_CURSOR_LINE_NO;
				m_bIsSelecting = false;
			}
			else
			if(SEL_CURSOR_LINE_NO > CURSOR_LINE_NO) 
			{
				leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
				rightHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(SEL_COLUMN_NO).c_str();
				
				m_Lines[CURSOR_LINE_NO] = "";
				m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
				m_Lines[CURSOR_LINE_NO] += rightHalfSubstr.c_str();
				
				for(int ii = 0; ii < (SEL_CURSOR_LINE_NO-CURSOR_LINE_NO); ii++) 
				{
					m_Lines[CURSOR_LINE_NO + 1].~basic_string();
					m_Lines.erase(m_Lines.begin() + CURSOR_LINE_NO + 1);
				}

				m_bIsSelecting = false;
			}
		}
		else 
		{
			leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
			rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();
		}

		if(rightHalfSubstr.length() > 0) 
		{
			m_Lines[CURSOR_LINE_NO] = "";
			m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
			m_Lines[CURSOR_LINE_NO] += ch;
			m_Lines[CURSOR_LINE_NO] += rightHalfSubstr.c_str();
		}	
		else 
		if(leftHalfSubstr.length() > 0) 
		{
			m_Lines[CURSOR_LINE_NO] = "";
			m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
			m_Lines[CURSOR_LINE_NO] += ch;
		}
		else 
		{
			m_Lines[CURSOR_LINE_NO] = "";
			m_Lines[CURSOR_LINE_NO] += ch;
		}
			
		COLUMN_NO++;
		
		setCaretDrawPosition();
		updateMains();
	}
	else
	if(ch == 13) 
	{
		if(m_bIsSelecting) 
		{
			if(CURSOR_LINE_NO == SEL_CURSOR_LINE_NO) 
			{
				if(COLUMN_NO > SEL_COLUMN_NO) 
				{
					leftHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(0, SEL_COLUMN_NO).c_str();
					rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();
				}
				else 
				{
					leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
					rightHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(SEL_COLUMN_NO).c_str();
				}
				
				m_Lines[CURSOR_LINE_NO] = "";
				m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
				
				m_Lines.insert(m_Lines.begin() + CURSOR_LINE_NO+1, rightHalfSubstr.c_str());
				COLUMN_NO = 0;
				CURSOR_LINE_NO++;
				m_bIsSelecting = false;
			}
			else
			if(CURSOR_LINE_NO > SEL_CURSOR_LINE_NO) 
			{
				leftHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(0, SEL_COLUMN_NO).c_str();
				rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();
				
				m_Lines[SEL_CURSOR_LINE_NO] = "";
				m_Lines[SEL_CURSOR_LINE_NO] += leftHalfSubstr.c_str();

				for(int ii = 0; ii < (CURSOR_LINE_NO-SEL_CURSOR_LINE_NO); ii++) 
				{
					m_Lines[SEL_CURSOR_LINE_NO + 1].~basic_string();
					m_Lines.erase(m_Lines.begin() + SEL_CURSOR_LINE_NO + 1);
				}

				m_Lines.insert(m_Lines.begin() + SEL_CURSOR_LINE_NO+1, rightHalfSubstr.c_str());

				COLUMN_NO = 0;
				CURSOR_LINE_NO = SEL_CURSOR_LINE_NO+1;
				m_bIsSelecting = false;
			}
			else
			if(SEL_CURSOR_LINE_NO > CURSOR_LINE_NO) 
			{
				leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
				rightHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(SEL_COLUMN_NO).c_str();
				
				m_Lines[CURSOR_LINE_NO] = "";
				m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
				
				for(int ii = 0; ii < (SEL_CURSOR_LINE_NO-CURSOR_LINE_NO); ii++) 
				{
					m_Lines[CURSOR_LINE_NO + 1].~basic_string();
					m_Lines.erase(m_Lines.begin() + CURSOR_LINE_NO + 1);
				}

				m_Lines.insert(m_Lines.begin() + CURSOR_LINE_NO+1, rightHalfSubstr.c_str());
				
				COLUMN_NO = 0;
				CURSOR_LINE_NO++;
				m_bIsSelecting = false;
			}
		}
		else 
		{
			leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
			rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();
			
			if(rightHalfSubstr.length() > 0) 
			{
				COLUMN_NO = 0;
				CURSOR_LINE_NO++;
				
				m_Lines[CURSOR_LINE_NO-1] = leftHalfSubstr.c_str();
				m_Lines.insert(m_Lines.begin() + CURSOR_LINE_NO, rightHalfSubstr.c_str());
			}
			else 
			{	
				COLUMN_NO = 0;
				CURSOR_LINE_NO++;
				m_Lines.insert(m_Lines.begin() + CURSOR_LINE_NO, "");
			}
		}
		
		setCaretDrawPosition();
		updateMains();
	}

	if(!m_bIsSelecting) 
	{
		SEL_CURSOR_LINE_NO = CURSOR_LINE_NO;
		SEL_COLUMN_NO = COLUMN_NO;
	}
}

void WConsoleLog::updateMains() 
{
	//////////////////////////////////////////////////////////////

	int xVal = m_minX+m_CaretPosX+m_mainX;
	if(m_CaretPosX == 0) 
	{
		m_mainX = 0;
	}
	else
	if(xVal > m_maxX) 
	{
		m_mainX = -(m_CaretPosX-(m_maxX-m_minX) + 1);
	}

	if(xVal < m_minX) 
	{
		m_mainX += m_ClientRect.Width;
	}
	if (m_mainX > 0)
	{
		m_mainX = 0;
	}

	//////////////////////////////////////////////////////////////

	int yPosTop = m_minY+(m_CaretPosY)+m_mainY;
	if(yPosTop < m_minY) 
	{
		//m_mainY = -(m_CaretPosY/LINE_HEIGHT)*LINE_HEIGHT;
		//Or
		m_mainY = -m_CaretPosY;
	}

	int yPosBottom = m_minY + (m_CaretPosY + LINE_HEIGHT) + m_mainY;
	if(yPosBottom >= m_maxY) 
	{
		m_mainY -= ((yPosBottom - m_maxY) + LINE_HEIGHT);
		m_mainY -= (m_mainY % LINE_HEIGHT);
	}

	if (m_mainY > 0)
	{
		m_mainY = 0;
	}
	
	//////////////////////////////////////////////////////////////

	calculateMaxLineWidth();
	setTBLineNoSpace();
}

void WConsoleLog::setCaretDrawPosition() 
{
	int xX = 0;
	int yY = CURSOR_LINE_NO*LINE_HEIGHT;

	if(CURSOR_LINE_NO < m_Lines.size()) 
	{
		yY = CURSOR_LINE_NO*LINE_HEIGHT;

		std::string str = m_Lines[CURSOR_LINE_NO].c_str();
		for(int i = 0; i < COLUMN_NO; i++) 
		{
			char c = str[i];
			int CHAR_WIDTH = WWidgetManager::getCharWidth(c);

			xX += CHAR_WIDTH;
		}
	}

	m_CaretPosX = xX;
	m_CaretPosY = yY;
}

void WConsoleLog::onKeyBUpEx(unsigned int iVirtualKeycode, unsigned short ch) 
{
}

void WConsoleLog::onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
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
							if(m_mainY > 0)
								m_mainY = 0;
						}
						break;
						case BTN_SCROLLBAR_DOWN:
						{
							m_mainY -= LINE_HEIGHT;
							if(abs(m_mainY) > (m_iMaxHeightPixels- m_ClientRect.Height))
								m_mainY = -(m_iMaxHeightPixels - m_ClientRect.Height);
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
					m_IsVScrolling = false;
				break;
				case ID_HORIZONTAL_SCROLLBAR:
					m_IsHScrolling = false;
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
					float scrollMaterialHeight = m_iMaxHeightPixels - m_ClientRect.Height;
					int mainYValue = (cursorPosInPercentage*scrollMaterialHeight)/100;

					m_mainY = -mainYValue;

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

void WConsoleLog::showLineNumbers(bool bShow) 
{
	HAS_LINE_NO = bShow;
	setTBLineNoSpace();
}

void WConsoleLog::setReadOnly(bool bRd) 
{
	mState = bRd?READONLY:NORMAL;

	m_sbVertical->setReadOnly(bRd);
	m_sbHorizontal->setReadOnly(bRd);
}

bool WConsoleLog::getReadOnly() 
{
	return (mState == READONLY);
}

void WConsoleLog::setTBLineNoSpace() 
{
	char* sLen = new char[16];
	memset(sLen, 0, 16);
	sprintf(sLen, "%d", m_Lines.size());
	TB_LINE_NO_SPACE = WWidgetManager::getStringWidthTillPos(sLen, strlen(sLen)) + (TB_LEFT_GUTTER << 1);

	m_minX = getLeft() + ((HAS_LINE_NO)?TB_LINE_NO_SPACE:0) + TB_LEFT_GUTTER;

	delete[] sLen;
}

void WConsoleLog::scrollVertically(int dir) 
{
	if(dir < 0) 
	{
		CURSOR_LINE_NO--;
		if(CURSOR_LINE_NO < 0)
			CURSOR_LINE_NO = 0;
	}
	else
	if(dir > 0) 
	{
		CURSOR_LINE_NO++;
		if(CURSOR_LINE_NO >= m_Lines.size())
			CURSOR_LINE_NO = m_Lines.size()-1;
	}
	
	std::string str = m_Lines[CURSOR_LINE_NO].c_str();
	if(COLUMN_NO > str.length())
		COLUMN_NO = str.length();

	setCaretDrawPosition();
	updateMains();
}

void WConsoleLog::deleteSelectedText() 
{
	std::string leftHalfSubstr;
	std::string rightHalfSubstr;

	if(CURSOR_LINE_NO == SEL_CURSOR_LINE_NO) 
	{
		if(COLUMN_NO > SEL_COLUMN_NO) 
		{
			leftHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(0, SEL_COLUMN_NO).c_str();
			rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();
			COLUMN_NO = SEL_COLUMN_NO;
		}
		else 
		{
			leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
			rightHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(SEL_COLUMN_NO).c_str();
		}

		m_bIsSelecting = false;
		m_Lines[CURSOR_LINE_NO] = "";
		m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
		m_Lines[CURSOR_LINE_NO] += rightHalfSubstr.c_str();
	}
	else
	if(CURSOR_LINE_NO > SEL_CURSOR_LINE_NO) 
	{
		leftHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(0, SEL_COLUMN_NO).c_str();
		rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();

		m_bIsSelecting = false;
		COLUMN_NO = SEL_COLUMN_NO;
		m_Lines[SEL_CURSOR_LINE_NO] = "";
		m_Lines[SEL_CURSOR_LINE_NO] += leftHalfSubstr.c_str();
		m_Lines[SEL_CURSOR_LINE_NO] += rightHalfSubstr.c_str();

		for(int ii = 0; ii < (CURSOR_LINE_NO-SEL_CURSOR_LINE_NO); ii++) 
		{
			m_Lines[SEL_CURSOR_LINE_NO + 1].~basic_string();
			m_Lines.erase(m_Lines.begin() + SEL_CURSOR_LINE_NO + 1);
		}

		CURSOR_LINE_NO = SEL_CURSOR_LINE_NO;
	}
	else 
	if(CURSOR_LINE_NO < SEL_CURSOR_LINE_NO) 
	{
		leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
		rightHalfSubstr = m_Lines[SEL_CURSOR_LINE_NO].substr(SEL_COLUMN_NO).c_str();

		m_bIsSelecting = false;
		m_Lines[CURSOR_LINE_NO] = "";
		m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();
		m_Lines[CURSOR_LINE_NO] += rightHalfSubstr.c_str();

		for(int ii = 0; ii < (SEL_CURSOR_LINE_NO-CURSOR_LINE_NO); ii++) 
		{
			m_Lines[CURSOR_LINE_NO + 1].~basic_string();
			m_Lines.erase(m_Lines.begin() + CURSOR_LINE_NO + 1);
		}
	}
}

void WConsoleLog::pasteText() 
{
	const char* pIsClipboardString = WWidgetManager::m_clipboardTextData.c_str();

	std::string leftHalfSubstr;
	std::string rightHalfSubstr;

	leftHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(0, COLUMN_NO).c_str();
	rightHalfSubstr = m_Lines[CURSOR_LINE_NO].substr(COLUMN_NO).c_str();

	m_Lines[CURSOR_LINE_NO] = "";
	m_Lines[CURSOR_LINE_NO] += leftHalfSubstr.c_str();

	//////////////////////////////////////////////////
	std::string sLine;
	int startPos = 0, endPos = 0;
	bool bFirstLine = true;
	std::string strBuff = pIsClipboardString;
	while(true) 
	{
		endPos = strBuff.find_first_of("\r\n", startPos);
		if(endPos < 0)
			break;

		sLine = strBuff.substr(startPos, endPos-startPos).c_str();

		if(bFirstLine) 
		{
			m_Lines[CURSOR_LINE_NO] += sLine.c_str();
			bFirstLine = false;
		}
		else 
		{
			m_Lines.insert(m_Lines.begin() + CURSOR_LINE_NO, sLine.c_str());
		}

		if(strBuff[endPos] == '\r')
			endPos++;

		CURSOR_LINE_NO++;
		startPos = endPos+1;
	}
	//////////////////////////////////////////////////

	if(bFirstLine) //Single-Line selection
	{
		m_Lines[CURSOR_LINE_NO] += pIsClipboardString;
		COLUMN_NO += strlen(pIsClipboardString);
	}
	else //Multi-Line selection
	{
		if(startPos < strBuff.size())
			m_Lines.insert(m_Lines.begin() + (CURSOR_LINE_NO), strBuff.substr(startPos, strBuff.size() - startPos));

		COLUMN_NO = strBuff.size() - startPos;
	}

	if(rightHalfSubstr.length() > 0)
		m_Lines[CURSOR_LINE_NO] += rightHalfSubstr.c_str();
}

int WConsoleLog::getTextLength() 
{
	m_sText = "";
	for(size_t i = 0; i < m_Lines.size(); i++) 
	{
		m_sText += m_Lines[i].c_str();
		if(i < m_Lines.size() - 1)
			m_sText += "\n";
	}

	return strlen(m_sText.c_str());
}

const char* WConsoleLog::getText() 
{
	m_sText = "";
	for(size_t i = 0; i < m_Lines.size(); i++) 
	{
		m_sText += m_Lines[i].c_str();
		if(i < m_Lines.size() - 1)
			m_sText += "\n";
	}

	return m_sText.c_str();
}

LRESULT WConsoleLog::OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch(msg) 
	{
		case WM__GETTEXTLENGTH:
		{
			return getTextLength();
		}
		break;
		case WM__GETTEXT:
		{
			return sprintf_s((char*)lParam, (size_t)wParam, "%s", getText());
		}
		break;
		case WM__SETTEXT:
		{
			setText((char*)lParam);
			return 1;
		}
		break;
		case WM__APPENDTEXT:
		{
			appendText((char*)lParam);
			return 1;
		}
		break;
	}

	return -1;
}

WConsoleLog::~WConsoleLog() 
{
	int i = 0;
	while(m_Lines.size() > 0) 
	{
		m_Lines[i].~basic_string();
		i++;
	}

	m_Lines.~vector();

	delete[] dbStr;
}
#endif