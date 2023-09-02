
#ifdef USE_YAGUI

#include "Engine/UI/WWidgetManager.h"
#include "Engine/UI/WComponentFactory.h"
#include "Common/TGA.h"
#include <glm/gtc/type_ptr.hpp>
#include "Engine/InputManager.h"

#define CORE_TEXTURE_UI					"../res/UI/core.tga"
#define CORE_FONT_UI_TTF_PATH			"../res/UI/%s.ttf"
#define CORE_FONT_UI_TTF_EXPORT_NAME	"../res/UI/%s_%d_%dDPI"
#define CORE_FONT_UI_TTF				"Rosemary_DroidSans"
#define CORE_FONT_UI_TTF_INFO			"../res/UI/coreInfo.txt"

#define CORE_SPRITE_VERT_SHADER			"../res/UI/sprite.vert"
#define CORE_SPRITE_FRAG_SHADER			"../res/UI/sprite.frag"

//////////////////////////////////////////////////////////////////////////////////////////////
WWidgetManager*				WWidgetManager::m_pInstance = NULL;
UI_FUNC_CALLBACK*			WWidgetManager::m_lpfnWWndProc = nullptr;
std::vector<WIDGET*>		WWidgetManager::m_pWidgets;
RectF						WWidgetManager::m_ClipRect = RectF(0, 0, 0, 0);
RectF						WWidgetManager::m_reclaimRect = RectF(0, 0, 0, 0);
int							WWidgetManager::m_SpriteCount;
int							WWidgetManager::m_FrameCount;
Color						WWidgetManager::m_RenderColour;
std::string					WWidgetManager::m_clipboardTextData;
int							WWidgetManager::m_iCurrentTextureID;
Texture*					WWidgetManager::m_pCurrentTexture = NULL;
Texture*					WWidgetManager::m_pTextureCoreUI = NULL;
H_FONT						WWidgetManager::m_pCurrentFont = NULL;

unsigned int				WWidgetManager::CHARACTER_WIDTH;
unsigned int				WWidgetManager::CHARACTER_HEIGHT;
Glyph						WWidgetManager::m_GlyphArray[ASCII_END_INDEX - ASCII_START_INDEX];

HCURSOR					WWidgetManager::m_hCurArrow;
HCURSOR					WWidgetManager::m_hCurIBeam;
HCURSOR					WWidgetManager::m_hCurCross;
HCURSOR					WWidgetManager::m_hCurSizeWE;
HCURSOR					WWidgetManager::m_hCurSizeNESW;
HCURSOR					WWidgetManager::m_hCurSizeNWSE;

VertexV3F_T2F_C4UB*		WWidgetManager::m_VB;
glm::mat4				WWidgetManager::m_Mat2DOrthogonalTransform;
//////////////////////////////////////////////////////////////////////////////////////////////

WWidgetManager::WWidgetManager() 
{
}

WWidgetManager* WWidgetManager::getInstance()
{
	if (m_pInstance == NULL)
	{
		m_pInstance = new WWidgetManager();
	}

	return m_pInstance;
}

void WWidgetManager::init(int32_t iScreenWidth, int32_t iScreenHeight)
{
	m_iScreenWidth = iScreenWidth;
	m_iScreenHeight = iScreenHeight;

	WComponentFactory::Get()->Register("WContainer", &WContainer::Create);
	WComponentFactory::Get()->Register("WStatic", &WStatic::Create);
	WComponentFactory::Get()->Register("WButton", &WButton::Create);
	WComponentFactory::Get()->Register("WCheckbox", &WCheckbox::Create);
	WComponentFactory::Get()->Register("WTextField", &WTextField::Create);
	WComponentFactory::Get()->Register("WScrollbar", &WScrollbar::Create);
	WComponentFactory::Get()->Register("WTextBox", &WTextBox::Create);
	WComponentFactory::Get()->Register("WListBox", &WListBox::Create);
	WComponentFactory::Get()->Register("WComboBox", &WComboBox::Create);
	WComponentFactory::Get()->Register("WConsoleLog", &WConsoleLog::Create);
	WComponentFactory::Get()->Register("WWindow", &WWindow::Create);
	WComponentFactory::Get()->Register("WTabbedPane", &WTabbedPane::Create);
	WComponentFactory::Get()->Register("WFrame", &WFrame::Create);
	WComponentFactory::Get()->Register("WTree", &WTree::Create);
	WComponentFactory::Get()->Register("WTable", &WTable::Create);
	WComponentFactory::Get()->Register("WDummy", &WDummy::Create);
	WComponentFactory::Get()->Register("WCanvas", &WCanvas::Create);
	WComponentFactory::Get()->Register("WInspectorTab", &WInspectorTab::Create);
	WComponentFactory::Get()->Register("WInspector", &WInspector::Create);
	WComponentFactory::Get()->Register("WGraph", &WGraph::Create);
	WComponentFactory::Get()->Register("WSprite", &WSprite::Create);

	if (NOT loadMainWindowDefaults())
	{
		MessageBox(NULL, "Error", "Error loading WWidgetManager!!!", MB_OK);
	}

	m_pBaseWindow = CreateComponent(	"WWindow", 
										"Title", 
										0, 
										0,
										0, 
										800, 
										600,
										NULL, 
										(HMENU)ROOT_WINDOW_ID,
										(LPVOID)ID_TYPE_WND_CX);
	((WWindow*)m_pBaseWindow)->setVisible(true);
	resetColor();

	m_hCurArrow = LoadCursor(NULL, IDC_ARROW);
	m_hCurIBeam = LoadCursor(NULL, IDC_IBEAM);
	m_hCurCross = LoadCursor(NULL, IDC_CROSS);
	m_hCurSizeWE = LoadCursor(NULL, IDC_SIZEWE);
	m_hCurSizeNESW = LoadCursor(NULL, IDC_SIZENESW);
	m_hCurSizeNWSE = LoadCursor(NULL, IDC_SIZENWSE);

	float fOrtho[16];
	{
		float oneOverHalfWidth = 1.0f/(m_iScreenWidth >> 1);
		float oneOverHalfHeight = 1.0f/(m_iScreenHeight >> 1);

		fOrtho[0] = oneOverHalfWidth;	fOrtho[1] = 0;					fOrtho[2] = 0;		fOrtho[3] = - 1.0f;
		fOrtho[4] = 0;					fOrtho[5] = -oneOverHalfHeight;	fOrtho[6] = 0;		fOrtho[7] = 1.0f;
		fOrtho[8] = 0;					fOrtho[9] = 0;					fOrtho[10] = 1;		fOrtho[11] = 0;
		fOrtho[12] = 0;					fOrtho[13] = 0;					fOrtho[14] = 0;		fOrtho[15] = 1;

		//m_Mat2DOrthogonalTransform = glm::transpose(glm::make_mat4(fOrtho));
	}
	
	glUseProgram(m_iProgramID);
	{
		m_Mat2DOrthogonalTransform = glm::ortho(0.0f, (float)m_iScreenWidth, (float)m_iScreenHeight, 0.0f);

		int mat2DOrthogonalTransformLoc = glGetUniformLocation(m_iProgramID, "mat2DOrthogonalTransform");
		glUniformMatrix4fv(mat2DOrthogonalTransformLoc, 1, GL_FALSE, glm::value_ptr(m_Mat2DOrthogonalTransform));

		glUniform1i(glGetUniformLocation(m_iProgramID, "uCoreTexture"), 0);
	}
}

bool WWidgetManager::loadMainWindowDefaults() 
{
	if (NOT readMap(CORE_FONT_UI_TTF_INFO))
	{
		return FALSE;
	}

	Texture* pTexture = Texture::createEx(CORE_TEXTURE_UI, false);
	if (pTexture == NULL)
	{
		return FALSE;
	}

	m_pTextureCoreUI = pTexture;

	H_FONT hFont = loadFont(CORE_FONT_UI_TTF, 10, 96);
	if (hFont == NULL)
	{
		return FALSE;
	}

	m_pCurrentFont = hFont;

	m_ColorWhiteUV.u = 675;
	m_ColorWhiteUV.v = 65;

	m_VB = new VertexV3F_T2F_C4UB[SPR_MAX * 6];
	{
		glGenVertexArrays(1, &m_iVAO);
		glBindVertexArray(m_iVAO);

		glGenBuffers(1, &m_iVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexV3F_T2F_C4UB) * SPR_MAX, NULL, GL_DYNAMIC_DRAW);
	}

	initShaders(m_iProgramID);

	return true;
}

bool WWidgetManager::selectFont(H_FONT hFont) 
{
	if (hFont == NULL)
	{
		return false;
	}

	m_pCurrentFont = hFont;
}

H_FONT WWidgetManager::loadFont(const char* sFontFile, unsigned int iFontSize, unsigned int iFontDPI) 
{
	//Rosemary_DroidSans
	//Crisp
	//Consolas
	//Walkway_Black
	//Rosemary Roman
	//Kingdom_Hearts_Font
	//Inspyratta
	//DroidSansMono
	//diagoth
	//DejaVuSans
	//Comic Sans MS
	//ROSEMARY_DROIDSANS-BOLD

	char str[255] = "";
	memset(str, 0, 255);
	sprintf(str, CORE_FONT_UI_TTF_PATH, sFontFile, iFontSize, iFontDPI);

	TTFFontEncoder* ttfEncoder = new TTFFontEncoder();
	ttfEncoder->encode(str, iFontSize, iFontDPI, true);

	sprintf(str, CORE_FONT_UI_TTF_EXPORT_NAME, sFontFile, iFontSize, iFontDPI);
	CCString sFontFileName = str;
	sFontFileName += ".tga";

	Texture* pTexture = Texture::createEx(sFontFileName.c_str(), false);
	if (pTexture == NULL)
	{
		return FALSE;
	}

	///////////////////////////////////////////////////
	sFontFileName.replace((char*)"tga", (char*)"dat");
	RandomAccessFile* rafIn = new RandomAccessFile();
	bool bCanRead = rafIn->openForRead(sFontFileName.c_str());

	if(bCanRead) 
	{
		CHARACTER_WIDTH = rafIn->readInt();
		CHARACTER_HEIGHT = rafIn->readInt();
		int iSize = rafIn->readInt();
		for(int i = 0; i < iSize; i++) 
		{
			m_GlyphArray[i].read(rafIn);
		}
	}

	m_GlyphArray[0].width = CHARACTER_WIDTH;
	rafIn->close();
	///////////////////////////////////////////////////

	return (H_FONT)pTexture;
}

void WWidgetManager::setGLStates() 
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	WContainer* pWContainer = (WContainer*)m_pBaseWindow;
	setClip(	pWContainer->getLeft(), 
				pWContainer->getTop(), 
				pWContainer->getWidth(), 
				pWContainer->getHeight());
}

void WWidgetManager::clearScreen() 
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void WWidgetManager::update(float deltaTimeMs) 
{
	if(m_pBaseWindow) 
	{
		///////////////////////////////////////
		setGLStates();
		///////////////////////////////////////

		///////////////////////////////////////
		((WContainer*)m_pBaseWindow)->frameUpdate(deltaTimeMs);
		((WContainer*)m_pBaseWindow)->frameRender();

		flush();
		m_FrameCount++;
		///////////////////////////////////////
	}
}

void WWidgetManager::flush()
{
	if (m_pCurrentTexture != NULL)
	{
		glActiveTexture(GL_TEXTURE0);
		m_pCurrentTexture->bind();

		glUseProgram(m_iProgramID);
		glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
		
		glBufferSubData(GL_ARRAY_BUFFER, 0, m_SpriteCount * sizeof(VertexV3F_T2F_C4UB), m_VB);
		
		//Position attribute
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3F_T2F_C4UB), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		
		// Colour attribute
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexV3F_T2F_C4UB), (GLvoid*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// Texture attribute
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3F_T2F_C4UB), (GLvoid*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glDrawArrays(GL_TRIANGLES, 0, m_SpriteCount);

		m_pCurrentTexture->unbind();
		m_SpriteCount = 0;
	}
}

bool WWidgetManager::isMousePressed(int32_t iKey)
{
	return InputManager::get()->isMousePressed(iKey);
}

bool WWidgetManager::isMouseReleased(int32_t iKey)
{
	return InputManager::get()->isMouseReleased(iKey);
}

void WWidgetManager::simulateMousePress(uint32_t iKey)
{
	InputManager::get()->onMousePressed(iKey);
}

void WWidgetManager::simulateMouseRelease(uint32_t iKey)
{
	InputManager::get()->onMouseReleased(iKey);
}

void WWidgetManager::simulateMouseMove(double dXPos, double dYPos)
{
	InputManager::get()->onMouseMoved(dXPos, dYPos);
}

bool WWidgetManager::isModifierKeyDown(int32_t iModifierKey)
{
	return InputManager::get()->isModifierKeyDown(iModifierKey);
}

void WWidgetManager::keyPressed(unsigned int iVirtualKeycode, unsigned short ch) 
{
	if (m_pBaseWindow)
	{
		((WContainer*)m_pBaseWindow)->onKeyBDown(iVirtualKeycode, ch);
	}
}

void WWidgetManager::keyReleased(unsigned int iVirtualKeycode, unsigned short ch) 
{
	if (m_pBaseWindow)
	{
		((WContainer*)m_pBaseWindow)->onKeyBUp(iVirtualKeycode, ch);
	}
}

void WWidgetManager::onMouseDown(int mCode, int x, int y) 
{
	((WContainer*)m_pBaseWindow)->onMouseDown(x, y, mCode);
}

void WWidgetManager::onMouseHover(int mCode, int x, int y) 
{
	if(m_pBaseWindow) 
	{
		((WContainer*)m_pBaseWindow)->onMouseHover(mCode, x, y, lastMouseX, lastMouseY);

		lastMouseX = x;
		lastMouseY = y;
	}
}

void WWidgetManager::onMouseMove(int mCode, int x, int y) 
{
	if(m_pBaseWindow) 
	{
		((WContainer*)m_pBaseWindow)->onMouseMove(mCode, x, y, lastMouseX, lastMouseY);

		lastMouseX = x;
		lastMouseY = y;
	}
}

void WWidgetManager::onMouseUp(int mCode, int x, int y) 
{
	if (m_pBaseWindow)
	{
		((WContainer*)m_pBaseWindow)->onMouseUp(x, y, mCode);
	}
}

void WWidgetManager::onMouseRDown(int mCode, int x, int y) 
{
}

void WWidgetManager::onMouseRUp(int mCode, int x, int y) 
{
}

void WWidgetManager::onMouseWheel(WPARAM wParam, LPARAM lParam) 
{
	if (m_pBaseWindow)
	{
		((WContainer*)m_pBaseWindow)->onMouseWheel(wParam, lParam);
	}
}

void WWidgetManager::GetClipBounds(RectF* reclaimRect) 
{
	reclaimRect->X = WWidgetManager::m_ClipRect.X;
	reclaimRect->Y = WWidgetManager::m_ClipRect.Y;
	reclaimRect->Width = WWidgetManager::m_ClipRect.Width;
	reclaimRect->Height = WWidgetManager::m_ClipRect.Height;
}

void WWidgetManager::setClip(int x, int y, int w, int h ) 
{
	WWidgetManager::m_ClipRect.X = x;
	WWidgetManager::m_ClipRect.Y = y;
	WWidgetManager::m_ClipRect.Width = w;
	WWidgetManager::m_ClipRect.Height = h;
}

void WWidgetManager::SET_CLIP(int x, int y , int width, int height) 
{
	WWidgetManager::GetClipBounds(&m_reclaimRect);

	RectF clipRect(x, y, width, height);
	RectF::Intersect(clipRect, m_reclaimRect, clipRect);

	WWidgetManager::setClip(clipRect.X, clipRect.Y, clipRect.Width, clipRect.Height);
}

void WWidgetManager::RESET_CLIP() 
{
	WWidgetManager::setClip(m_reclaimRect.X, m_reclaimRect.Y, m_reclaimRect.Width, m_reclaimRect.Height);
}

bool WWidgetManager::initShaders(GLint& iProgramID)
{
	bool bSuccess = false;

	GLuint iVertexShaderID;
	GLuint iFragmentShaderID;

	bSuccess = compileShaderFromFile(GL_VERTEX_SHADER, CORE_SPRITE_VERT_SHADER, iVertexShaderID);
	if (bSuccess)
	{
		bSuccess = compileShaderFromFile(GL_FRAGMENT_SHADER, CORE_SPRITE_FRAG_SHADER, iFragmentShaderID);
		if (bSuccess)
		{
			GL_ASSERT(iProgramID = glCreateProgram());
			GL_ASSERT(glAttachShader(iProgramID, iVertexShaderID));
			GL_ASSERT(glAttachShader(iProgramID, iFragmentShaderID));
			GL_ASSERT(glLinkProgram(iProgramID));
			bSuccess = checkCompileErrors(iProgramID, "LINK");

			// Delete shaders after linking
			GL_ASSERT(glDeleteShader(iVertexShaderID));
			GL_ASSERT(glDeleteShader(iFragmentShaderID));
			GL_ASSERT(glDeleteShader(iFragmentShaderID));
		}
	}

	return bSuccess;
}

bool WWidgetManager::compileShaderFromFile(GLenum iShaderType, const char* sShaderFile, GLuint& iShaderID)
{
	std::string sShaderSource = "";
	sShaderSource = RandomAccessFile::readAll(sShaderFile);
	if (sShaderSource.c_str() == NULL) 
	{
		GP_ERROR("Failed to read vertex shader from file '%s'.", sShaderFile);
		return -1;
	}

	return compileShader(iShaderType, sShaderSource.c_str(), iShaderID);
}

bool WWidgetManager::compileShader(GLenum iShaderType, const char* sShaderSource, GLuint& iShaderID)
{
	bool bSuccess = false;

	iShaderID = glCreateShader(iShaderType);
	glShaderSource(iShaderID, 1, &sShaderSource, NULL);
	glCompileShader(iShaderID);
	bSuccess = checkCompileErrors(iShaderID, "COMPILE");
	//glGetShaderiv(iShaderID, GL_COMPILE_STATUS, &iSuccess);

	return bSuccess;
}

bool WWidgetManager::checkCompileErrors(GLuint iShaderID, std::string sType)
{
	GLint bSuccess;
	GLchar infoLog[1024];
	if (sType == "COMPILE")
	{
		glGetShaderiv(iShaderID, GL_COMPILE_STATUS, &bSuccess);
		if (!bSuccess)
		{
			glGetShaderInfoLog(iShaderID, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << sType << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
	else
	{
		glGetProgramiv(iShaderID, GL_LINK_STATUS, &bSuccess);
		if (!bSuccess)
		{
			glGetProgramInfoLog(iShaderID, 1024, NULL, infoLog);
			std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << sType << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}

	return bSuccess;
}

bool WWidgetManager::readMap(const char* filePathAndName) 
{
	/////////////////// LOADING GAME FONTS
	RandomAccessFile* rafIn = new RandomAccessFile();
	bool bCanRead = rafIn->openForRead(filePathAndName);

	if (NOT bCanRead)
	{
		return false;
	}

	CCString singleLine;

	int iWidgetCount = 0;
	int READ_STATE	 = 0;

	while(rafIn->isEOF() == 0) 
	{
		singleLine = (char*)rafIn->readLine();
		singleLine.trim();

		if(CCString::startsWith(singleLine.c_str(), "WIDGET")) 
		{
			WIDGET*			widget = new WIDGET();
			BASESKIN*		baseSkin = NULL;
			CHILD*			child = NULL;
			CLIENT_AREA*	clientArea = NULL;
			SIMPLE_TEXT*	simpleText = NULL;

			sscanf(singleLine.c_str(), "WIDGET %s {\n", &widget->widgetName);

			while(true) 
			{
				singleLine = (char*)rafIn->readLine();
				singleLine.trim();

				if(READ_STATE == STATE_READ_WIDGET && CCString::startsWith(singleLine.c_str(), "}")) 
				{
					m_pWidgets.push_back(widget);
					break;
				}
				else
				if(CCString::startsWith(singleLine.c_str(), "BaseSkin")) 
				{
					baseSkin = new BASESKIN();
					READ_STATE = STATE_READ_BASESKIN;
				}
				else
				if(CCString::startsWith(singleLine.c_str(), "Child")) 
				{
					child = new CHILD();
					READ_STATE = STATE_READ_CHILD;
				}
				else
				if(CCString::startsWith(singleLine.c_str(), "ClientArea")) 
				{
					clientArea = new CLIENT_AREA();
					READ_STATE = STATE_READ_CLIENTAREA;
				}
				else
				if(CCString::startsWith(singleLine.c_str(), "SimpleText")) 
				{
					simpleText = new SIMPLE_TEXT();
					READ_STATE = STATE_READ_SIMPLE_TEXT;
				}

				switch(READ_STATE) 
				{
					case STATE_READ_WIDGET:
					{
						if (CCString::startsWith(singleLine.c_str(), "size")) 
						{
							sscanf(singleLine.c_str(), "size \"%f %f\"\n", &widget->widgetSize.width, &widget->widgetSize.height);
						}
					}
					break;
					case STATE_READ_CHILD:
					{
						if(CCString::startsWith(singleLine.c_str(), "}")) 
						{
							widget->childWindows.push_back(child);
							READ_STATE = STATE_READ_WIDGET;
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "skin")) 
						{
							sscanf(	singleLine.c_str(), "skin %s\n", &child->sName);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "size")) 
						{
							sscanf(	singleLine.c_str(), "size \"%f %f\"\n", &child->parentSize.width, &child->parentSize.height);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "posOffset")) 
						{
							sscanf(	singleLine.c_str(), "posOffset \"%f %f %f %f\"\n", &child->posOffsets.x, &child->posOffsets.y, &child->posOffsets.w, &child->posOffsets.h);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "skinOffset")) 
						{
							sscanf(	singleLine.c_str(), "skinOffset \"%f %f %f %f\"\n", &child->skinOffsets.x, &child->skinOffsets.y, &child->skinOffsets.w, &child->skinOffsets.h);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "align")) 
						{
							ALIGN* align = new ALIGN();
							char hAlign[255];
							char vAlign[255];
							sscanf(	singleLine.c_str(), "align %s %s\n", &hAlign, &vAlign);

							child->align.eHAlign = getWidgetHAlign(hAlign);
							child->align.eVAlign = getWidgetVAlign(vAlign);
						}
					}
					break;
					case STATE_READ_CLIENTAREA:
					{
						if(CCString::startsWith(singleLine.c_str(), "}")) 
						{
							widget->clientAreas.push_back(clientArea);
							READ_STATE = STATE_READ_WIDGET;
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "skin")) 
						{
							sscanf(	singleLine.c_str(), "skin %s\n", &clientArea->sName);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "size")) 
						{
							sscanf(	singleLine.c_str(), "size \"%f %f\"\n", &clientArea->parentSize.width, &clientArea->parentSize.height);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "posOffset")) 
						{
							sscanf(	singleLine.c_str(), "posOffset \"%f %f %f %f\"\n", &clientArea->posOffsets.x, &clientArea->posOffsets.y, &clientArea->posOffsets.w, &clientArea->posOffsets.h);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "skinOffset")) 
						{
							sscanf(	singleLine.c_str(), "skinOffset \"%f %f %f %f\"\n", &clientArea->skinOffsets.x, &clientArea->skinOffsets.y, &clientArea->skinOffsets.w, &clientArea->skinOffsets.h);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "align")) 
						{
							ALIGN* align = new ALIGN();
							char hAlign[255];
							char vAlign[255];
							sscanf(	singleLine.c_str(), "align %s %s\n", &hAlign, &vAlign);

							clientArea->align.eHAlign = getWidgetHAlign(hAlign);
							clientArea->align.eVAlign = getWidgetVAlign(vAlign);
						}
					}
					break;
					case STATE_READ_SIMPLE_TEXT:
					{
						if(CCString::startsWith(singleLine.c_str(), "}")) 
						{
							widget->strings.push_back(simpleText);
							READ_STATE = STATE_READ_WIDGET;
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "string")) 
						{
							sscanf(	singleLine.c_str(), "string %s\n", &simpleText->string);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "posOffset")) 
						{
							sscanf(	singleLine.c_str(), "posOffset \"%f %f %f %f\"\n", &simpleText->posOffsets.x, &simpleText->posOffsets.y, &simpleText->posOffsets.w, &simpleText->posOffsets.h);
						}
						//else
						//if(CCString::startsWith(singleLine.c_str(), "font")) 
						//{
						//	sscanf(	singleLine.c_str(), "font %s\n", &simpleText->font);
						//}
						else
						if(CCString::startsWith(singleLine.c_str(), "align")) 
						{
							ALIGN* align = new ALIGN();
							char hAlign[255];
							char vAlign[255];
							sscanf(	singleLine.c_str(), "align %s %s\n", &hAlign, &vAlign);

							simpleText->align.eHAlign = getWidgetHAlign(hAlign);
							simpleText->align.eVAlign = getWidgetVAlign(vAlign);
						}
					}
					break;
					case STATE_READ_BASESKIN:
					{
						if(CCString::startsWith(singleLine.c_str(), "}")) 
						{
							widget->skins.push_back(baseSkin);
							READ_STATE = STATE_READ_WIDGET;
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "size")) 
						{
							sscanf(	singleLine.c_str(), "size \"%f %f\"\n", &baseSkin->parentSize.width, &baseSkin->parentSize.height);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "posOffset")) 
						{
							sscanf(	singleLine.c_str(), "posOffset \"%f %f %f %f\"\n", &baseSkin->posOffsets.x, &baseSkin->posOffsets.y, &baseSkin->posOffsets.w, &baseSkin->posOffsets.h);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "skinOffset")) 
						{
							sscanf(	singleLine.c_str(), "skinOffset \"%f %f %f %f\"\n", &baseSkin->skinOffsets.x, &baseSkin->skinOffsets.y, &baseSkin->skinOffsets.w, &baseSkin->skinOffsets.h);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "align")) 
						{
							ALIGN* align = new ALIGN();
							char hAlign[255];
							char vAlign[255];
							sscanf(	singleLine.c_str(), "align %s %s\n", &hAlign, &vAlign);

							baseSkin->align.eHAlign = getWidgetHAlign(hAlign);
							baseSkin->align.eVAlign = getWidgetVAlign(vAlign);
						}
						else
						if(CCString::startsWith(singleLine.c_str(), "name")) 
						{
							sscanf(	singleLine.c_str(), "name %s\n", &baseSkin->name);
						}
					}
					break;
				}
			}
		}
	}

	/*
	for(int i = 0; i < m_pWidgets.size(); i++) {
		WIDGET* widget = (WIDGET*)m_pWidgets[i];
		
		printf("widgetName = %s\n",widget->widgetName);
		for(int i = 0; i < widget->childWindows.size(); i++) {
			CHILD* child = (CHILD*)widget->childWindows[i];
			printf("\tchildName = %s\n", child->sName);
			printf("\t\tchild PosOffsets = %f %f %f %f\n", child->posOffsets.x, child->posOffsets.y, child->posOffsets.w, child->posOffsets.h);
			printf("\t\tskin Offsets = %f %f %f %f\n", child->skinOffsets.x, child->skinOffsets.y, child->skinOffsets.w, child->skinOffsets.h);
		}
		for(int i = 0; i < widget->skins.size(); i++) {
			BASESKIN* baseskin = (BASESKIN*)widget->skins[i];
			printf("\tskinName = %s\n", baseSkin->name);
			printf("\t\tskin PosOffsets = %f %f %f %f\n", baseSkin->posOffsets.x, baseSkin->posOffsets.y, baseSkin->posOffsets.w, baseSkin->posOffsets.h);
			printf("\t\tskin Offsets = %f %f %f %f\n", baseSkin->skinOffsets.x, baseSkin->skinOffsets.y, baseSkin->skinOffsets.w, baseSkin->skinOffsets.h);
		}
	}
	*/

	return true;
}

H_ALIGN WWidgetManager::getWidgetHAlign(char* sAlign) 
{
	if(strcmp(sAlign, "Left") == 0)				return H_LEFT;
	else if(strcmp(sAlign, "HCenter") == 0)		return H_CENTER;
	else if(strcmp(sAlign, "Right") == 0)		return H_RIGHT;
	else if(strcmp(sAlign, "HStretch") == 0)	return H_STRETCH;

	return H_LEFT;
}

V_ALIGN WWidgetManager::getWidgetVAlign(char* sAlign) 
{
	if(strcmp(sAlign, "Top") == 0)				return V_TOP;
	else if(strcmp(sAlign, "VCenter") == 0)		return V_CENTER;
	else if(strcmp(sAlign, "Bottom") == 0)		return V_BOTTOM;
	else if(strcmp(sAlign, "VStretch") == 0)	return V_STRETCH;

	return V_TOP;
}

int WWidgetManager::getTextAlignInt(char* sAlign) 
{
	if(strcmp(sAlign, "Left") == 0)			return TXT_LEFT;
	else if(strcmp(sAlign, "Top") == 0)		return TXT_TOP;
	else if(strcmp(sAlign, "Right") == 0)	return TXT_RIGHT;
	else if(strcmp(sAlign, "Bottom") == 0)	return TXT_BOTTOM;
	else if(strcmp(sAlign, "VCenter") == 0)	return TXT_VCENTER;
	else if(strcmp(sAlign, "HCenter") == 0)	return TXT_HCENTER;

	return TXT_LEFT;
}

void WWidgetManager::renderSkin(WIDGET* widget, BASESKIN* skinPtr, RectF* wndRect) 
{
	RectF skinRect;
	RectF idealRect;

	float parentW = widget->widgetSize.width;
	float parentH = widget->widgetSize.height;
	H_ALIGN hAlign = H_LEFT;
	V_ALIGN vAlign = V_TOP;

	skinRect.X = skinPtr->skinOffsets.x;
	skinRect.Y = skinPtr->skinOffsets.y;
	skinRect.Width = skinPtr->skinOffsets.w;
	skinRect.Height = skinPtr->skinOffsets.h;

	idealRect.X = skinPtr->posOffsets.x;
	idealRect.Y = skinPtr->posOffsets.y;
	idealRect.Width = skinPtr->posOffsets.w;
	idealRect.Height = skinPtr->posOffsets.h;

	hAlign = skinPtr->align.eHAlign;
	vAlign = skinPtr->align.eVAlign;

	RectF destRect;
	getDestinationRect(	destRect,
						parentW,
						parentH,
						wndRect,
						&idealRect, 
						hAlign, 
						vAlign
					);
	
	drawQuadU(m_pTextureCoreUI, destRect.X, destRect.Y, destRect.Width, destRect.Height, skinRect.X, skinRect.Y, skinRect.Width, skinRect.Height);
}

void WWidgetManager::renderChild(WIDGET* widget, CHILD* childPtr, RectF* wndRect) 
{
	RectF idealRect;

	float parentW = widget->widgetSize.width;
	float parentH = widget->widgetSize.height;
	H_ALIGN hAlign = H_LEFT;
	V_ALIGN vAlign = V_TOP;

	idealRect.X = childPtr->posOffsets.x;
	idealRect.Y = childPtr->posOffsets.y;
	idealRect.Width = childPtr->posOffsets.w;
	idealRect.Height = childPtr->posOffsets.h;

	hAlign = childPtr->align.eHAlign;
	vAlign = childPtr->align.eVAlign;

	RectF destRect;
	getDestinationRect(	destRect,
						parentW,
						parentH,
						wndRect,
						&idealRect, 
						hAlign, 
						vAlign
					);

	renderWidget(childPtr->sName, &destRect );
}

void WWidgetManager::renderChild(WIDGET* widget, CHILD* childPtr, RectF* wndRect, float x, float y) 
{
	RectF idealRect;

	float parentW = widget->widgetSize.width;
	float parentH = widget->widgetSize.height;
	H_ALIGN hAlign = H_LEFT;
	V_ALIGN vAlign = V_TOP;

	idealRect.X = childPtr->posOffsets.x;
	idealRect.Y = childPtr->posOffsets.y;
	idealRect.Width = childPtr->posOffsets.w;
	idealRect.Height = childPtr->posOffsets.h;

	hAlign = childPtr->align.eHAlign;
	vAlign = childPtr->align.eVAlign;

	RectF destRect;
	getDestinationRect(	destRect,
						parentW,
						parentH,
						wndRect,
						&idealRect, 
						hAlign, 
						vAlign
						);
	
	destRect.X = x;
	destRect.Y = y;

	renderWidget(childPtr->sName, &destRect );
}

void WWidgetManager::renderClientArea(WIDGET* widget, int i, RectF* wndRect) 
{
	float parentW = widget->widgetSize.width;
	float parentH = widget->widgetSize.height;

	RectF skinRect;
	RectF idealRect;
	RectF destRect;

	H_ALIGN hAlign = H_LEFT;
	V_ALIGN vAlign = V_TOP;

	if(widget->clientAreas.size() > 0 && i < widget->clientAreas.size()) {

			idealRect.X = widget->clientAreas[i]->posOffsets.x;
			idealRect.Y = widget->clientAreas[i]->posOffsets.y;
			idealRect.Width = widget->clientAreas[i]->posOffsets.w;
			idealRect.Height = widget->clientAreas[i]->posOffsets.h;

			hAlign = widget->clientAreas[i]->align.eHAlign;
			vAlign = widget->clientAreas[i]->align.eVAlign;

			getDestinationRect(	destRect,
								parentW,
								parentH,
								wndRect,
								&idealRect, 
								hAlign, 
								vAlign
								);
			Rect clientRect(destRect.X, destRect.Y, destRect.Width, destRect.Height);
			WWidgetManager::getInstance()->fillRect(1.0f, 1.0f, 0.0f, 1.0f, &clientRect);
	}
}

void WWidgetManager::renderWidget(const char* sWidgetName, RectF* wndRect) 
{
	int widgetId = getWidgetID(sWidgetName);
	WIDGET* widget = (WIDGET*)m_pWidgets[widgetId];
	
	if(widget->skins.size() > 0) 
	{
		for(int j = 0; j < widget->skins.size(); j++) 
		{
			renderSkin(widget, widget->skins[j], wndRect);
		}
	}
}

void WWidgetManager::getDestinationRect(	RectF& destRect, 
											float parentW, 
											float parentH, 
											RectF* wndRect, 
											RectF* idealRect, 
											H_ALIGN hAlign, 
											V_ALIGN vAlign) 
{	
	destRect.X = wndRect->X + idealRect->X;
	destRect.Y = wndRect->Y + idealRect->Y;
	destRect.Width = wndRect->Width;
	destRect.Height = wndRect->Height;

	switch(hAlign) 
	{
		case H_LEFT:
		{
			destRect.Width = idealRect->Width;
		}
		break;
		case H_CENTER:
		{
			destRect.X = (wndRect->X + wndRect->Width) - ((parentW - idealRect->X)/2);
			destRect.Width = idealRect->Width;
		}
		break;
		case H_RIGHT:
		{
			destRect.X = wndRect->X + wndRect->Width - (parentW - idealRect->X);
			destRect.Width = idealRect->Width;
		}
		break;
		case H_STRETCH:
		{
			int wW = wndRect->Width - (parentW - (idealRect->X + idealRect->Width));
			destRect.Width = wW - idealRect->X;
		}
		break;
	}

	switch(vAlign) 
	{
		case V_TOP:
		{
			destRect.Height = idealRect->Height;
		}
		break;
		case V_CENTER:
		{
			destRect.Y = (wndRect->Y + wndRect->Height) - ((parentH - idealRect->Y)/2);
			destRect.Height = idealRect->Height;
		}
		break;
		case V_BOTTOM:
		{
			destRect.Y = wndRect->Y + wndRect->Height - (parentH - idealRect->Y);
			destRect.Height = idealRect->Height;
		}
		break;
		case V_STRETCH:
		{
			float hH = wndRect->Height - (parentH - (idealRect->Y + idealRect->Height));
			destRect.Height = hH - idealRect->Y;
		}
		break;
	}
}

WIDGET* WWidgetManager::getWidget(const char* sWidgetName) 
{
	for(int i = 0; i < m_pWidgets.size(); i++) 
	{
		WIDGET* widget = (WIDGET*)m_pWidgets[i];
		if (strcmp(sWidgetName, widget->widgetName) == 0)
		{
			return widget;
		}
	}

	return NULL;
}

int WWidgetManager::getWidgetID(const char* sWidgetName) 
{
	for(int i = 0; i < m_pWidgets.size(); i++) 
	{
		WIDGET* widget = (WIDGET*)m_pWidgets[i];
		if (strcmp(sWidgetName, widget->widgetName) == 0)
		{
			return i;
		}
	}

	return -1;
}

float WWidgetManager::getWidgetWidth(const char* sWidgetName) 
{
	WIDGET* widget = getWidget(sWidgetName);
	float width = 0.0f;

	if(widget) 
	{
		width = widget->widgetSize.width;
	}

	return width;
}

float WWidgetManager::getWidgetHeight(const char* sWidgetName) 
{
	WIDGET* widget = getWidget(sWidgetName);
	float height = 0.0f;

	if(widget) 
	{
		height = widget->widgetSize.height;
	}

	return height;
}

void WWidgetManager::setColor(float r, float g, float b, float a)
{
	m_RenderColour.r = r;
	m_RenderColour.g = g;
	m_RenderColour.b = b;
	m_RenderColour.a = a;
}

void WWidgetManager::resetColor() 
{ 
	m_RenderColour.r = 1.0f;
	m_RenderColour.g = 1.0f;
	m_RenderColour.b = 1.0f;
	m_RenderColour.a = 1.0f;
}

void WWidgetManager::drawQuadU(	Texture* pTexture, 
								float posX, float posY, float posW, float posH,
								float texX, float texY, float texW, float texH
) {
	if (pTexture != m_pCurrentTexture)
	{
		WWidgetManager::getInstance()->flush();
	}

	m_pCurrentTexture = pTexture;

	GLuint TEX_WIDTH = m_pCurrentTexture->getWidth();
	GLuint TEX_HEIGHT = m_pCurrentTexture->getHeight();
	GLfloat tX = texX/TEX_WIDTH, tY = texY/TEX_HEIGHT, tW = texW/TEX_WIDTH, tH = texH/TEX_HEIGHT;

	RectF drawRect(posX, posY, posW, posH);
	RectF posIntersectRect(0, 0, posW, posH);
	RectF::Intersect(posIntersectRect, m_ClipRect, drawRect);

	float fLeft = posIntersectRect.X;
	float fTop = posIntersectRect.Y;
	float fRight = posIntersectRect.X + posIntersectRect.Width;
	float fBottom = posIntersectRect.Y + posIntersectRect.Height;

	if(posIntersectRect.Width > 0 && posIntersectRect.Height > 0) 
	{
		////////// If IntersectRect width is less than texture width
		if(posIntersectRect.Width < texW) 
		{
			texW = posIntersectRect.Width;
			tW = texW/TEX_WIDTH;

			////////// If posX < clipRectX, shift texture X equal to the diff in X
			if(posIntersectRect.X > posX) 
			{
				texX += (posIntersectRect.X - posX);
				tX = texX/TEX_WIDTH;
			}
		}

		////////// If IntersectRect height is less than texture height
		if(posIntersectRect.Height < texH) 
		{
			texH = posIntersectRect.Height;
			tH = texH/TEX_HEIGHT;
			
			////////// If posY < clipRectY, shift texture Y equal to the diff in Y
			if(posIntersectRect.Y > posY) 
			{
				texY += (posIntersectRect.Y - posY);
				tY = texY/TEX_HEIGHT;
			}
		}

		float zZ = 0.0f;

		//////////////////////////////////////////////////////////////////////////////////////
		// Right Top
		{
			m_VB[m_SpriteCount + 0].vx = fRight;
			m_VB[m_SpriteCount + 0].vy = fTop;
			m_VB[m_SpriteCount + 0].r = m_RenderColour.r;
			m_VB[m_SpriteCount + 0].g = m_RenderColour.g;
			m_VB[m_SpriteCount + 0].b = m_RenderColour.b;
			m_VB[m_SpriteCount + 0].a = m_RenderColour.a;
			m_VB[m_SpriteCount + 0].tu = tX + tW;
			m_VB[m_SpriteCount + 0].tv = 1 - tY;
		}

		// Right Bottom
		{
			m_VB[m_SpriteCount + 1].vx = fRight;
			m_VB[m_SpriteCount + 1].vy = fBottom;
			m_VB[m_SpriteCount + 1].r = m_RenderColour.r;
			m_VB[m_SpriteCount + 1].g = m_RenderColour.g;
			m_VB[m_SpriteCount + 1].b = m_RenderColour.b;
			m_VB[m_SpriteCount + 1].a = m_RenderColour.a;
			m_VB[m_SpriteCount + 1].tu = tX + tW;
			m_VB[m_SpriteCount + 1].tv = 1 - (tY + tH);
		}

		// Left Bottom
		{
			m_VB[m_SpriteCount + 2].vx = fLeft;
			m_VB[m_SpriteCount + 2].vy = fBottom;
			m_VB[m_SpriteCount + 2].r = m_RenderColour.r;
			m_VB[m_SpriteCount + 2].g = m_RenderColour.g;
			m_VB[m_SpriteCount + 2].b = m_RenderColour.b;
			m_VB[m_SpriteCount + 2].a = m_RenderColour.a;
			m_VB[m_SpriteCount + 2].tu = tX;
			m_VB[m_SpriteCount + 2].tv = 1 - (tY + tH);
		}
		//////////////////////////////////////////////////////////////////////////////////////

		// Left Top
		{	
			m_VB[m_SpriteCount + 3].vx = fLeft;
			m_VB[m_SpriteCount + 3].vy = fTop;
			m_VB[m_SpriteCount + 3].r = m_RenderColour.r;
			m_VB[m_SpriteCount + 3].g = m_RenderColour.g;
			m_VB[m_SpriteCount + 3].b = m_RenderColour.b;
			m_VB[m_SpriteCount + 3].a = m_RenderColour.a;
			m_VB[m_SpriteCount + 3].tu = tX;
			m_VB[m_SpriteCount + 3].tv = 1 - tY;
		}

		// Right Top
		{
			m_VB[m_SpriteCount + 4].vx = fRight;
			m_VB[m_SpriteCount + 4].vy = fTop;
			m_VB[m_SpriteCount + 4].r = m_RenderColour.r;
			m_VB[m_SpriteCount + 4].g = m_RenderColour.g;
			m_VB[m_SpriteCount + 4].b = m_RenderColour.b;
			m_VB[m_SpriteCount + 4].a = m_RenderColour.a;
			m_VB[m_SpriteCount + 4].tu = tX + tW;
			m_VB[m_SpriteCount + 4].tv = 1 - tY;
		}

		// Left Bottom
		{
			m_VB[m_SpriteCount + 5].vx = fLeft;
			m_VB[m_SpriteCount + 5].vy = fBottom;
			m_VB[m_SpriteCount + 5].r = m_RenderColour.r;
			m_VB[m_SpriteCount + 5].g = m_RenderColour.g;
			m_VB[m_SpriteCount + 5].b = m_RenderColour.b;
			m_VB[m_SpriteCount + 5].a = m_RenderColour.a;
			m_VB[m_SpriteCount + 5].tu = tX;
			m_VB[m_SpriteCount + 5].tv = 1 - (tY + tH);
		}
		//////////////////////////////////////////////////////////////////////////////////////
		
		m_SpriteCount += 6;
	}
}

int WWidgetManager::getStringWidthTillPos(const char* cStr, int iPos) 
{
	int iRetWidth = 0;
	if(cStr != NULL && iPos <= strlen(cStr)) 
	{
		for(int i = 0; i < iPos; i++) 
		{
			iRetWidth += getCharWidth(cStr[i]);
		}
	}

	return iRetWidth;
}

int WWidgetManager::getGlyphU(int c) 
{
	if(c >= ASCII_START_INDEX && c <= ASCII_END_INDEX) 
	{
		return m_GlyphArray[c-32].uvCoords[0];
	}

	return -1;
}

int WWidgetManager::getGlyphV(int c) 
{
	if(c >= ASCII_START_INDEX && c <= ASCII_END_INDEX) 
	{
		return m_GlyphArray[c-32].uvCoords[1];
	}

	return -1;
}

void WWidgetManager::drawStringFont(const char* cStr, int x, int y, int anchor) 
{
	int strWidth = getStringWidthTillPos(cStr, strlen(cStr));

	int X = 0, Y = 0;

	int HLEFT = 0;
	int HCENTER = 1;
	int HRIGHT = 2;

	int xX = x;
	int yY = y;

	if(anchor == HCENTER)		xX = xX - (strWidth/2);
	else if(anchor == HRIGHT)	xX = xX - strWidth;

	for(int i = 0 ; i < strlen(cStr); i++) 
	{
		char c = cStr[i];

		if(c == '\r') 
		{
			continue;
		}

		int CHAR_WIDTH = getCharWidth(c);
		if(c > ' ') 
		{
			X = getGlyphU(c);
			Y = getGlyphV(c);

			//WWidgetManager::setColor(0xff000000);//ABGR
			WWidgetManager::drawFont(xX, yY, CHAR_WIDTH, WWidgetManager::CHARACTER_HEIGHT, X, Y);
			//WWidgetManager::resetColor();

			xX += CHAR_WIDTH;
		}
		else
		if(c == ' ' || c == '\t') 
		{
			xX += CHAR_WIDTH;
		}
		else
		if(c == '\n') 
		{
			xX = x;
			yY += WWidgetManager::CHARACTER_HEIGHT + 3;
		}
	}
}

int WWidgetManager::getCharWidth(int c) 
{
	if(c >= 32 && c <= 126) 
	{
		unsigned int width = m_GlyphArray[c - 32].width;

		if(c == ' ' || c == '\t')	(width >>= 2);
		return						(width + 1);
	}

	return m_GlyphArray[0].width;
}

void WWidgetManager::fillRect(Color* color, Rect* rect) 
{
	fillRect(color->r, color->g, color->b, color->a, rect);
}

void WWidgetManager::fillRect(float r, float g, float b, float a, Rect* rect) 
{
#ifdef USE_GL_10
	glPushAttrib( GL_CURRENT_BIT );
	
	glColor4f(r/255.0, g/255.0, b/255.0, a/255.0);

	float x = (float)rect->X;
	float y = (float)rect->Y;
	float w = (float)rect->Width;
	float h = (float)rect->Height;

	glBegin(GL_QUADS);						// Draw A Quad
		glVertex3f(x,	y,		0);			// Top Left
		glVertex3f(x,	y+h,	0);			// Top Right
		glVertex3f(x+w, y+h,	0);			// Bottom Right
		glVertex3f(x+w, y,		0);			// Bottom Left
	glEnd();								// Done Drawing The Quad

	glPopAttrib();
#endif
	setColor(r, g, b, a);
	drawQuadU(m_pTextureCoreUI, rect->X, rect->Y, rect->Width, rect->Height, m_ColorWhiteUV.u, m_ColorWhiteUV.v, 1, 1);
	resetColor();
}

void WWidgetManager::drawRect(float r, float g, float b, float a, Rect* rect, int iStrokeWidth)
{
	drawHorizontalLine(r, g, b, a, rect->X, rect->Y, rect->Width, iStrokeWidth);
	drawVerticalLine(r, g, b, a, rect->X + rect->Width - iStrokeWidth, rect->Y, rect->Height, iStrokeWidth);
	drawHorizontalLine(r, g, b, a, rect->X, rect->Y + rect->Height - iStrokeWidth, rect->Width, iStrokeWidth);
	drawVerticalLine(r, g, b, a, rect->X, rect->Y, rect->Height, iStrokeWidth);
}

void WWidgetManager::drawHorizontalLine(float r, float g, float b, float a, int x, int y, int width, int iStrokeWidth) 
{	
	setColor(r, g, b, a);
	drawQuadU(m_pTextureCoreUI, x, y, width, iStrokeWidth, m_ColorWhiteUV.u, m_ColorWhiteUV.v, 1, 1);
	resetColor();
}

void WWidgetManager::drawVerticalLine(float r, float g, float b, float a, int x, int y, int height, int iStrokeWidth) 
{
	setColor(r, g, b, a);
	drawQuadU(m_pTextureCoreUI, x, y, iStrokeWidth, height, m_ColorWhiteUV.u, m_ColorWhiteUV.v, 1, 1);
	resetColor();
}

void WWidgetManager::drawVerticalLine(float x1, float y1, float x2, float y2) 
{
	drawQuadU(m_pTextureCoreUI, x1, y1, 1, (y2 - y1), 675, 50, 1, 1);
}

int WWidgetManager::getNextWhitespacePos(const char* str, int curPos, bool isLeft) 
{
	int len = 0;
	if(isLeft) 
	{
		for(int i = curPos; i > 0; i--) 
		{
			char ch = str[i];
			if(ch == '\r')								continue;
			if(ch == ' ' || ch == '\t' || ch == '\n')	return i;
		}

		return 0;
	}
	else 
	{
		if (curPos == strlen(str))
		{
			return strlen(str);
		}

		for(int i = curPos; i < strlen(str); i++) 
		{
			char ch = str[i];
			if(ch == '\r')								continue;
			if(ch == ' ' || ch == '\t' || ch == '\n')	return i;
		}

		return (strlen(str) - 1);
	}
}

void WWidgetManager::drawFont(int xX, int yY, int charW, int charH, int tX, int tY) 
{
	drawQuadU((Texture*)m_pCurrentFont, xX, yY, charW, charH, tX, tY, charW, charH);
}

void WWidgetManager::setCallback(UI_FUNC_CALLBACK* wndProc) 
{
	if (wndProc != nullptr)
	{
		m_lpfnWWndProc = wndProc;
	}
}

L_RESULT WWidgetManager::onEvent(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	if (m_lpfnWWndProc != nullptr)
	{
		return (*m_lpfnWWndProc)(hWnd, msg, wParam, lParam);
	}
}

H_WND WWidgetManager::GetWindowQ(UINT ID_WINDOW)
{
	if (ID_WINDOW >= 0)
	{
		return GetWindow(ID_WINDOW);
	}

	return NULL;
}

BOOL WWidgetManager::ShowWindowQ(H_WND hWnd, int nCmdShow)
{
	BOOL bRet = FALSE;
	if (hWnd != NULL)
	{
		WComponent* pComp = (WComponent*)hWnd;
		bRet = pComp->isVisible();
		pComp->setVisible(nCmdShow);
	}

	return bRet;
}

H_WND WWidgetManager::GetParentQ(H_WND hWnd)
{
	H_WND hParent = NULL;
	if (hWnd != NULL)
	{
		WComponent* pComp = (WComponent*)hWnd;
		hParent = (H_WND)pComp->getParent();
	}

	return hParent;
}

H_WND WWidgetManager::GetWindow(int ID_WINDOW) 
{
	if (ID_WINDOW == 0)
	{
		return (H_WND)m_pBaseWindow;
	}
	else
	{
		return ((WContainer*)m_pBaseWindow)->GetWindow(ID_WINDOW);
	}
}

H_WND WWidgetManager::FindWindowQ(LPCSTR lpClassName, LPCSTR lpWindowName) 
{
	return ((WContainer*)m_pBaseWindow)->FindWindowQ(lpClassName, lpWindowName);
}

void WWidgetManager::setCursor(int MOUSE_IDC) 
{
	switch(MOUSE_IDC) 
	{
		case  IDC__ARROW:
		{
			::SetCursor(m_hCurArrow);
		}
		break;
		case  IDC__IBEAM:
		{
			::SetCursor(m_hCurIBeam);
		}
		break;
		case  IDC__CROSS:
		{
			::SetCursor(m_hCurCross);
		}
		break;
		case  IDC__SIZEWE:
		{
			::SetCursor(m_hCurSizeWE);
		}
		break;
		case  IDC__SIZENESW:
		{
			::SetCursor(m_hCurSizeNESW);
		}
		break;
		case  IDC__SIZENWSE:
		{
			::SetCursor(m_hCurSizeNWSE);
		}
		break;
	}
}

WWidgetManager::~WWidgetManager() 
{
}
#endif