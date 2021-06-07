#pragma once

#ifdef USE_YAGUI

#include "Engine/UI/widgetdef.h"
#include "Common/Defines.h"
#include "Engine/Texture.h"
#include "UIDefines.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include <functional>

#define TEXTURE_CORE				0
#define TEXTURE_FONT_ROSEMARY_16	1
#define MAX_WIDGETS					80

typedef L_RESULT (__stdcall* YAGUICallback)(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef std::function<L_RESULT(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)>	UI_FUNC_CALLBACK;

struct WWidgetManager 
{
	public:
									~WWidgetManager();
		static WWidgetManager*		getInstance();
		
		static UI_FUNC_CALLBACK*	m_lpfnWWndProc;
		H_WND						m_pBaseWindow;

		static void					setCallback(UI_FUNC_CALLBACK* wndProc);
		H_WND						GetWindow(int ID_WINDOW);
		H_WND						FindWindowQ(LPCSTR lpClassName, LPCSTR lpWindowName);

		void						init(int32_t iScreenWidth, int32_t iScreenHeight);
		void						update(uint32_t iDeltaTimeMs, uint64_t lElapsedTime);
		void						paint();

		H_WND						GetWindowQ(UINT ID_WINDOW);
		H_WND						GetWindowQ(H_WND hWnd, UINT ID_WINDOW);
		BOOL						ShowWindowQ(H_WND hWnd, int nCmdShow);
		H_WND						GetParentQ(H_WND hWnd);

		bool						isMousePressed(int32_t iKey);
		bool						isMouseReleased(int32_t iKey);
		void						simulateMousePress(uint32_t iKey);
		void						simulateMouseRelease(uint32_t iKey);
		void						simulateMouseMove(double dXPos, double dYPos);

		bool						isModifierKeyDown(int32_t iModifierKey);
		void						keyPressed(unsigned int iVirtualKeycode, unsigned short ch);
		void						keyReleased(unsigned int iVirtualKeycode, unsigned short ch);
		void						onMouseDown(int mCode, int x, int y);
		void						onMouseUp(int mCode, int x, int y);
		void						onMouseRDown(int mCode, int x, int y);
		void						onMouseRUp(int mCode, int x, int y);
		void						onMouseHover(int mCode, int x, int y);
		void						onMouseMove(int mCode, int x, int y);
		void						onMouseWheel(WPARAM wParam, LPARAM lParam);

		void						setGLStates();
		void						flush();
		static void					drawFont(int x, int y, int charW, int charH, int tX, int tY);
		static int					getGlyphU(int c);
		static int					getGlyphV(int c);

		static void					renderWidget(const char* sWidgetName, RectF* WndRect);
		static void					renderSkin(WIDGET* widget, BASESKIN* skinPtr, RectF* wndRect);
		static void					renderChild(WIDGET* widget, CHILD* childPtr, RectF* wndRect);
		static void					renderChild(WIDGET* widget, CHILD* childPtr, RectF* wndRect, float x, float y);
		static void					renderClientArea(WIDGET* widget, int i, RectF* wndRect);

		static WIDGET*				getWidget(const char* sWidgetName);
		static int					getWidgetID(const char* sWidgetName);
		static float				getWidgetWidth(const char* sWidgetName);
		static float				getWidgetHeight(const char* sWidgetName);
		static void					getDestinationRect(RectF& destRect, float parentW, float parentH, RectF* wndRect, RectF* idealRect, H_ALIGN hAlign, V_ALIGN vAlign);
		bool						loadMainWindowDefaults();

		static void					setClip(int x, int y, int w, int h );
		static void					GetClipBounds(RectF* reclaimRect);
		static void					SET_CLIP(int x, int y , int width, int height);
		static void					RESET_CLIP();
		static RectF				m_reclaimRect;
		
		static void					drawStringFont(const char* cStr, int x, int y, int anchor);
		static int					getCharWidth(int c);
		static int					getStringWidthTillPos(const char* cStr, int iPos);
		void						fillRect(Color* color, Rect* rect);
		void						fillRect(float r, float g, float b, float a, Rect* rect);
		void						drawRect(float r, float g, float b, float a, Rect* rect, int iStrokeWidth = 1);
		void						drawHorizontalLine(float r, float g, float b, float a, int x, int y, int width, int iStrokeWidth);
		void						drawVerticalLine(float r, float g, float b, float a, int x, int y, int height, int iStrokeWidth);
		static void					drawVerticalLine(float x1, float y1, float x2, float y2);
		static int					getNextWhitespacePos(const char* str, int curPos, bool isLeft);

		static void					clearScreen();
		static void					setCursor(int MOUSE_IDC);

		static void					drawQuadU(	Texture* texture, 
												float posX, float posY, float posW, float posH,
												float texX, float texY, float texW, float texH);
		static void					setColor(float r, float g, float b, float a);
		static void					resetColor();

		static H_FONT				loadFont(const char* sFontFileName, unsigned int iFontSize, unsigned int iFontDPI);
		static bool					selectFont(H_FONT hFont);

		static L_RESULT				onEvent(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static H_WND				getWindow(int wndID);

		static std::string			m_clipboardTextData;

		static WWidgetManager*		m_pInstance;
		static int					m_SpriteCount;
		static int					m_FrameCount;
		static Color				m_RenderColour;
		
		static VertexV3F_T2F_C4UB*	m_VB;
		static unsigned int			CHARACTER_WIDTH;
		static unsigned int			CHARACTER_HEIGHT;
		static Glyph				m_GlyphArray[];

	private:
									WWidgetManager();
		bool						readMap(const char* filePathAndName);

		void						initOpenGLBuffers();
		bool						initShaders(GLint& iProgramID);
		GLint						compileShaderFromFile(GLenum iShaderType, const char* sShaderFile, GLuint& iShaderID);
		GLint						compileShader(GLenum iShaderType, const char* sShaderSource, GLuint& iShaderID);

		H_ALIGN						getWidgetHAlign(char* sAlign);
		V_ALIGN						getWidgetVAlign(char* sAlign);
		int							getTextAlignInt(char* sAlign);

		static std::vector<WIDGET*>	m_pWidgets;

		static RectF				m_ClipRect;
		static int					m_iCurrentTextureID;
		static Texture*				m_pCurrentTexture;
		static Texture*				m_pTextureCoreUI;
		static H_FONT				m_pCurrentFont;

		ColorUV						m_ColorWhiteUV;

		int							lastMouseX;
		int							lastMouseY;

		static HCURSOR				m_hCurArrow;
		static HCURSOR				m_hCurIBeam;
		static HCURSOR				m_hCurCross;
		static HCURSOR				m_hCurSizeWE;
		static HCURSOR				m_hCurSizeNESW;
		static HCURSOR				m_hCurSizeNWSE;

		GLuint						m_iVAO;
		GLuint						m_iVBO;
		GLint						m_iProgramID;

		static float				m_fOneOverWidth;
		static float				m_fOneOverHeight;

		static glm::mat4			m_Mat2DOrthogonalTransform;

		int32_t						m_iScreenWidth;
		int32_t						m_iScreenHeight;
};
#endif