#ifdef USE_YAGUI
#include "Engine/UI/WTextField.h"
#include "Engine/UI/WWidgetManager.h"
#include <algorithm>

using namespace std;

#define TB_LEFT_GUTTER				5
#define TB_TOP_GUTTER				3
#define TB_RIGHT_GUTTER				10
#define	 TB_TOP_GUTTER				2

unsigned int WTextField::LINE_HEIGHT;

WTextField::WTextField()
: m_clipData("")
{

}

H_WND WTextField::Create(	const char* lpClassName, 
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
	WTextField* pWTextField = new WTextField();
	pWTextField->setText(lpWindowName);

	((WComponent*)pWTextField)->Create(	lpClassName, 
										"WTextField", 
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

	return pWTextField;
}

void WTextField::onCreateEx(LPVOID lpVoid) 
{
	LINE_HEIGHT = WWidgetManager::CHARACTER_HEIGHT + (TB_TOP_GUTTER << 1);

	mState = NORMAL;
	m_CaretPosX = m_CaretPosY = 0;
	m_mainX = 0;
	m_mainY = 0;

	m_bIsSelecting = false;
	setBorderVisibility(true);

	m_TextBoxWidget = WWidgetManager::getWidget("TextBox");
	SEL_COLUMN_NO = COLUMN_NO = 0;
	mState = NORMAL;

	m_minX = getLeft() + TB_LEFT_GUTTER;
	m_minY = getTop() + TB_TOP_GUTTER;
	m_maxX = getRight() - TB_RIGHT_GUTTER;
	m_maxY = getBottom();

	updateMains();
}

const char* WTextField::getText() 
{
	return m_Lines[0].c_str();
}

int WTextField::getLength() 
{
	return strlen(m_Lines[0].c_str());
}

void WTextField::setText(const char* str) 
{	
	int startPos = 0, endPos = 0;
	std::string strBuff = str;
	
	endPos = strBuff.find_first_of("\r\n", startPos);
	if(endPos < 0)
		endPos = strBuff.size();
	
	if(m_Lines.size() > 0) 
	{
		std::vector<std::string>::iterator itr = m_Lines.begin();
		m_Lines.erase(itr + 0);
	}

	m_Lines.push_back(strBuff.substr(startPos, endPos-startPos));
}

void WTextField::drawStringFont(int x, int y, int anchor) 
{
	int X = 0, Y = 0;
	
	int xX = x;
	int yY = y - m_mainY;

	std::string str = m_Lines[0].c_str();

	int i = 0;
	while(true) 
	{
		bool bDrawSelect = false;
		if(yY >= m_maxY)
			break;
		
		int CHAR_WIDTH = 0;
		char c = ' ';

		if(m_Lines[0].size() > 0) 
		{
			c = m_Lines[0].at(i);
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

					if(m_bIsSelecting)
						bDrawSelect = isUnderSelection(i);
				}
				else
				if(c == ' ' || c == '\t') 
				{
					if(m_bIsSelecting)
						bDrawSelect = isUnderSelection(i);
				}

				if(bDrawSelect) 
				{
					Rect rect(xX, m_minY, CHAR_WIDTH, WWidgetManager::CHARACTER_HEIGHT - TB_TOP_GUTTER);
					WWidgetManager::getInstance()->fillRect(0.5f, 0.5f, 0.5f, 0.5f, &rect);
				}
			}

			xX += CHAR_WIDTH;
		}

		i++;

		if(i >= str.length()) 
		{
			break;
		}
	}
}

bool WTextField::isUnderSelection(int column) 
{
	bool bRet = false;

	if(COLUMN_NO > SEL_COLUMN_NO) 
	{
		if(column >= SEL_COLUMN_NO && column < COLUMN_NO)
			bRet = true;
	}
	if(COLUMN_NO < SEL_COLUMN_NO) 
	{
		if(column >= COLUMN_NO && column < SEL_COLUMN_NO)
			bRet = true;
	}

	return bRet;
}

void WTextField::getCaretPos(int x, int y) 
{
	int lineNo = 0;//abs(y)/LINE_HEIGHT;

	std::string str = m_Lines[lineNo].c_str();

	int xX = 0;
	int yY = lineNo*LINE_HEIGHT;

	int i = 0;
	int caretPos = 0;

	if(str.length() == 0) 
	{
		m_CaretPosX = xX;
		m_CaretPosY = yY;
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
			//if(y >= yY && y < yY+LINE_HEIGHT) 
			{
				m_CaretPosX = xX;
				m_CaretPosY = yY;
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
				//if(y >= yY && y < yY+LINE_HEIGHT) 
				{
					m_CaretPosX = xX;
					m_CaretPosY = yY;
					COLUMN_NO = i;

					return;
				}
			}
			
			break;
		}
	}

	return;
}

void WTextField::onUpdate(float deltaTimeMs) 
{
	m_minX = getLeft() + TB_LEFT_GUTTER;
	m_minY = getTop() + TB_TOP_GUTTER;
	m_maxX = getRight() - TB_RIGHT_GUTTER;
	m_maxY = getBottom() - TB_TOP_GUTTER;
}

void WTextField::onRender() 
{
	//setClip(getLeft(), getTop(), getWidth(), getHeight());

	WWidgetManager* renderer =  WWidgetManager::getInstance();
	RectF thisWndRect(getLeft(), getTop(), getWidth(), LINE_HEIGHT);

	if(getBorderVisibility()) 
	{
		CHILD* child = m_TextBoxWidget->getChild("TextArea");
		renderer->renderChild(m_TextBoxWidget, child, &thisWndRect);
	}
	/////////////////////////////
	if(mState == READONLY) 
	{
		Rect rect(getLeft(), getTop(), getWidth(), getHeight());
		WWidgetManager::getInstance()->fillRect(0.5f, 0.5f, 0.5f, 0.5f, &rect);
	}
	/////////////////////////////

	/////////////////////////////
	//resetClip();

	setClip(m_minX, m_minY, m_maxX - m_minX, LINE_HEIGHT);
	drawStringFont(m_minX + m_mainX, m_minY + m_mainY, 0);
	resetClip();
	/////////////////////////////

	setClip(getLeft(), getTop(), getWidth(), getHeight());
	if(hasKeyFocus()) 
	{
		WWidgetManager::drawVerticalLine(	m_minX + m_mainX + m_CaretPosX, 
											m_minY,
											m_minX + m_mainX + m_CaretPosX, 
											m_minY + WWidgetManager::CHARACTER_HEIGHT - TB_TOP_GUTTER);
	}	
	drawSelection();
	resetClip();
}

void WTextField::setClip(int x, int y , int width, int height) 
{
	WWidgetManager::GetClipBounds(&m_reclaimRect);

	RectF clipRect(x, y, width, height);
	RectF::Intersect(clipRect, m_reclaimRect, clipRect);

	WWidgetManager::setClip(clipRect.X, clipRect.Y, clipRect.Width, clipRect.Height);
}

void WTextField::resetClip() 
{
	WWidgetManager::setClip(m_reclaimRect.X, m_reclaimRect.Y, m_reclaimRect.Width, m_reclaimRect.Height);
}

void WTextField::drawSelection() 
{
	if(m_bIsSelecting) 
	{
	}
	else 
	{
		SEL_COLUMN_NO = COLUMN_NO;
	}
}

DWORD WTextField::getSelection() 
{
	DWORD dwRange = 0;
	if(m_bIsSelecting) 
	{
		if(SEL_COLUMN_NO > COLUMN_NO ) 
		{
			dwRange |= (COLUMN_NO & 0xffff);
			dwRange |= ((SEL_COLUMN_NO & 0xffff) << 16);
		}
		else
		if(COLUMN_NO > SEL_COLUMN_NO) 
		{
			dwRange |= (SEL_COLUMN_NO & 0xffff);
			dwRange |= ((COLUMN_NO & 0xffff) << 16);
		}
	}

	return dwRange;
}

void WTextField::setSelection(DWORD dwRange) 
{
	short leftSel = (dwRange & 0xffff);
	short rightSel = ((dwRange >> 16) & 0xffff);

	if(leftSel < 0 || leftSel > getLength() || rightSel < 0 || rightSel > getLength()) 
	{
		m_bIsSelecting = false;
		return;
	}
	
	m_bIsSelecting = true;
	COLUMN_NO = leftSel;
	SEL_COLUMN_NO = rightSel;

	setCaretDrawPosition();
	updateMains();
}

void WTextField::onMouseDownEx(int x, int y, int iButton) 
{
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

void WTextField::onMouseUpEx(int x, int y, int iButton) 
{
	WWidgetManager::setCursor(IDC__IBEAM);
}

void WTextField::onMouseEnterEx(int mCode, int x, int y, int prevX, int prevY) 
{
	WWidgetManager::setCursor(IDC__IBEAM);
}

void WTextField::onMouseHoverEx(int mCode, int x, int y, int prevX, int prevY) 
{
	WWidgetManager::setCursor(IDC__IBEAM);
}

void WTextField::onMouseMoveEx(int mCode, int x, int y, int prevX, int prevY) 
{
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
		if(m_mainX > 0)
			m_mainX = 0;
		
		m_mainY += (y - prevY);
		if(m_mainY > 0)
			m_mainY = 0;
	}
}

void WTextField::onMouseLeaveEx(int mCode, int x, int y, int prevX, int prevY) 
{
	WWidgetManager::setCursor(IDC__ARROW);
}

void WTextField::onMouseWheelEx(WPARAM wParam, LPARAM lParam) 
{
	
}

bool WTextField::isReadOnlyChar(char ch) 
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

static bool isModifierPressed(int32_t iModifierSet, int32_t iModifier)
{
	return ((iModifierSet & iModifier) > 0);
}

void WTextField::onKeyBDownEx(unsigned int iKey, unsigned short ch) 
{
	if(mState == READONLY && !isReadOnlyChar(ch))
		return;

	WWidgetManager* pWidgetManager = WWidgetManager::getInstance();
	bool CONTROL_ON = pWidgetManager->isModifierKeyDown(GLFW_MOD_CONTROL);
	bool SHIFT_ON = pWidgetManager->isModifierKeyDown(GLFW_MOD_SHIFT);

	std::string leftHalfSubstr("");
	std::string rightHalfSubstr("");

	if (CONTROL_ON)
	{
		if(iKey == 'X' || iKey == 'C')
		{
			//if(OpenClipboard(GetForegroundWindow())) 
			{
				bool isCut = (iKey == 'X');

				std::string leftStrUnderSelection = "";
				std::string rightStrUnderSelection = "";
				std::string strUnderSelection = "";
				{
					std::string middleString = "";

					if(COLUMN_NO > SEL_COLUMN_NO) 
					{
						leftHalfSubstr = m_Lines[0].substr(0, SEL_COLUMN_NO);
						rightHalfSubstr = m_Lines[0].substr(COLUMN_NO);
						middleString = m_Lines[0].substr(SEL_COLUMN_NO, (COLUMN_NO-SEL_COLUMN_NO));
							
						if(isCut)
							COLUMN_NO = SEL_COLUMN_NO;
					}
					else 
					{
						leftHalfSubstr = m_Lines[0].substr(0, COLUMN_NO);
						rightHalfSubstr = m_Lines[0].substr(SEL_COLUMN_NO);
						middleString = m_Lines[0].substr(COLUMN_NO, (SEL_COLUMN_NO-COLUMN_NO));
					}
						
					if(isCut) 
					{
						m_Lines[0] = "";
						m_Lines[0] += leftHalfSubstr;
						m_Lines[0] += rightHalfSubstr;
						m_bIsSelecting = false;
					}
						
					WWidgetManager::m_clipboardTextData = middleString.c_str();
				}
					
				setCaretDrawPosition();
				updateMains();

				return;
			}
		}
		else
		if (iKey == 'V')
		{
			//if (OpenClipboard(GetForegroundWindow()))
			{
				if (m_bIsSelecting)
					deleteSelectedText();

				pasteText();

				setCaretDrawPosition();
				updateMains();

				return;
			}
		}
		else
		if (iKey == 'A')
		{
			selectText();
			return;
		}
	}

	if (iKey == GLFW_KEY_LEFT)
	{
		bool bWasSelecting = m_bIsSelecting;
		m_bIsSelecting = SHIFT_ON;
			
		std::string str = m_Lines[0];
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
			COLUMN_NO = 0;
			
		/* Set Caret Position & Update Viewport*/
		setCaretDrawPosition();
		updateMains();

		return;
	}

	if (iKey == GLFW_KEY_RIGHT)
	{	
		int prevColumnNo = COLUMN_NO;
		bool bWasSelecting = m_bIsSelecting;
		m_bIsSelecting = SHIFT_ON;
			
		std::string str = m_Lines[0];
		/* Move Caret */
		if (CONTROL_ON)
		{
			if(COLUMN_NO < str.size()-1)
				COLUMN_NO = WWidgetManager::getNextWhitespacePos(str.c_str(), COLUMN_NO, false) + 1;
			else
				COLUMN_NO = str.size()+1;
		}
		else 
		{
			COLUMN_NO++;
		}
			
		if(COLUMN_NO > str.size())
			COLUMN_NO = prevColumnNo;
			
		/* Set Caret Position & Update Viewport*/
		setCaretDrawPosition();
		updateMains();

		return;
	}

	if (iKey == GLFW_KEY_HOME)
	{
		m_bIsSelecting = SHIFT_ON;

		COLUMN_NO = 0;

		setCaretDrawPosition();
		updateMains();

		return;
	}

	if (iKey == GLFW_KEY_END)
	{	
		bool bCtrl = CONTROL_ON;
		m_bIsSelecting = SHIFT_ON;
			
		std::string str = m_Lines[0];
		COLUMN_NO = str.size();

		if(bCtrl) 
		{
			std::string str = m_Lines[0];
			COLUMN_NO = str.size();
		}

		setCaretDrawPosition();
		updateMains();

		return;
	}

	if (iKey == GLFW_KEY_BACKSPACE)
	{
		bool bWasSelecting = m_bIsSelecting;
		if(m_bIsSelecting)
			deleteSelectedText();
		else 
		{
			if(COLUMN_NO > 0) 
			{
				leftHalfSubstr = m_Lines[0].substr(0, COLUMN_NO-1);
				rightHalfSubstr = m_Lines[0].substr(COLUMN_NO);

				if(rightHalfSubstr.size() > 0) 
				{
					m_Lines[0] = "";
					m_Lines[0] += leftHalfSubstr;
					m_Lines[0] += rightHalfSubstr;
				}
				else 
				{
					m_Lines[0].assign(leftHalfSubstr);
				}

				COLUMN_NO--;
			}
		}
			
		setCaretDrawPosition();
		updateMains();

		return;
	}

	if (iKey == GLFW_KEY_DELETE)
	{
		bool bWasSelecting = m_bIsSelecting;
		if(m_bIsSelecting)
			deleteSelectedText();
		else 
		{
			if(COLUMN_NO < m_Lines[0].size()) 
			{
				leftHalfSubstr = m_Lines[0].substr(0, COLUMN_NO);
				rightHalfSubstr = m_Lines[0].substr(COLUMN_NO+1);

				if(rightHalfSubstr.size() > 0) 
				{
					m_Lines[0] = "";
					m_Lines[0] += leftHalfSubstr;
					m_Lines[0] += rightHalfSubstr;
				}
				else
					m_Lines[0].assign(leftHalfSubstr);
			}
		}
			
		setCaretDrawPosition();
		updateMains();

		return;
	}

	if(ch >= ' ' && ch <= '~' )
	{
		bool bWasSelecting = m_bIsSelecting;
		if(m_bIsSelecting) 
		{
			if(COLUMN_NO > SEL_COLUMN_NO) 
			{
				leftHalfSubstr = m_Lines[0].substr(0, SEL_COLUMN_NO);
				rightHalfSubstr = m_Lines[0].substr(COLUMN_NO);

				m_Lines[0] = "";
				m_Lines[0] += leftHalfSubstr;
				m_Lines[0] += rightHalfSubstr;
				COLUMN_NO = SEL_COLUMN_NO;
			}
			else 
			{
				leftHalfSubstr = m_Lines[0].substr(0, COLUMN_NO);
				rightHalfSubstr = m_Lines[0].substr(SEL_COLUMN_NO);

				m_Lines[0] = "";
				m_Lines[0] += leftHalfSubstr;
				m_Lines[0] += rightHalfSubstr;
			}
				
			m_bIsSelecting = false;
		}
		else 
		{
			leftHalfSubstr = m_Lines[0].substr(0, COLUMN_NO);
			rightHalfSubstr = m_Lines[0].substr(COLUMN_NO);
		}

		if(rightHalfSubstr.size() > 0) 
		{
			m_Lines[0] = "";
			m_Lines[0] += leftHalfSubstr;
			m_Lines[0] += ch;
			m_Lines[0] += rightHalfSubstr;
		}	
		else 
		if(leftHalfSubstr.size() > 0) 
		{
			m_Lines[0] = "";
			m_Lines[0] += leftHalfSubstr;
			m_Lines[0] += ch;
		}
		else 
		{
			m_Lines[0] = "";
			m_Lines[0] += ch;
		}
			
		COLUMN_NO++;
		
		setCaretDrawPosition();
		updateMains();
	}
	else
	if(ch == 13) 
	{
	}

	if(!m_bIsSelecting) 
	{
		SEL_COLUMN_NO = COLUMN_NO;
	}

	if(m_pParent)
		m_pParent->onMessage((H_WND)this, KEY_DOWN, getComponentID(), (iKey << 32) | ch);
}

void WTextField::updateMains() 
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
		m_mainX = -m_CaretPosX;
	}
	if(m_mainX > 0)
		m_mainX = 0;

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

	if(m_mainY > 0)
		m_mainY = 0;
	
	//////////////////////////////////////////////////////////////
}

void WTextField::setCaretDrawPosition() 
{
	int xX = 0;
	int yY = 0;
	std::string str = m_Lines[0];

	for(int i = 0; i < COLUMN_NO; i++) 
	{
		char c = str[i];
		int CHAR_WIDTH = WWidgetManager::getCharWidth(c);

		xX += CHAR_WIDTH;
	}

	m_CaretPosX = xX;
	m_CaretPosY = yY;
}

void WTextField::onKeyBUpEx(unsigned int iVirtualKeycode, unsigned short ch) 
{

}

void WTextField::onMessage(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
}

void WTextField::setReadOnly(bool bRd) 
{
	mState = bRd?READONLY:NORMAL;
}

bool WTextField::getReadOnly() 
{
	return (mState == READONLY);
}

void WTextField::selectText() 
{
	m_bIsSelecting = true;
	SEL_COLUMN_NO = 0;
	COLUMN_NO = m_Lines[0].size();
}

void WTextField::pasteText() 
{
	const char* pIsClipboardString = WWidgetManager::m_clipboardTextData.c_str();

	std::string leftHalfSubstr = m_Lines[0].substr(0, COLUMN_NO);
	std::string rightHalfSubstr = m_Lines[0].substr(COLUMN_NO);

	// Left Text
	m_Lines[0] = "";
	m_Lines[0] += leftHalfSubstr;

	// Center Text
	{
		m_Lines[0] += pIsClipboardString;
		COLUMN_NO += strlen(pIsClipboardString);
	}

	// Right Text
	if (rightHalfSubstr.size() > 0)
	{
		m_Lines[0] += rightHalfSubstr;
	}
}

void WTextField::deleteSelectedText() 
{
	std::string leftHalfSubstr("");
	std::string rightHalfSubstr("");

	if(COLUMN_NO > SEL_COLUMN_NO) 
	{
		leftHalfSubstr = m_Lines[0].substr(0, SEL_COLUMN_NO);
		rightHalfSubstr = m_Lines[0].substr(COLUMN_NO);
		COLUMN_NO = SEL_COLUMN_NO;
	}
	else 
	{
		leftHalfSubstr = m_Lines[0].substr(0, COLUMN_NO);
		rightHalfSubstr = m_Lines[0].substr(SEL_COLUMN_NO);
	}

	m_bIsSelecting = false;
	m_Lines[0] = "";
	m_Lines[0] += leftHalfSubstr;
	m_Lines[0] += rightHalfSubstr;
}

LRESULT WTextField::OnSendMessage(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch(msg) 
	{
		case WM__GETTEXTLENGTH:
		{
			return strlen(m_Lines[0].c_str());
		}
		break;
		case WM__GETTEXT:
		{
			std::string* str = (std::string*)wParam;
			*str = m_Lines[0].c_str();

			return m_Lines[0].size();
		}
		break;
		case WM__SETTEXT:
		{
			char* str = (char*)lParam;
			if(str == NULL)
				return -1;

			m_Lines[0] = str;

			return WM__OKAY;
		}
		break;
	}

	return -1;
}

WTextField::~WTextField() 
{
}
#endif