#pragma once
#ifdef USE_YAGUI
#include <string>
#include <map>

#include "Engine/UI/WWidgetManager.h"

#include "Engine/UI/WStatic.h"
#include "Engine/UI/WButton.h"
#include "Engine/UI/WCheckbox.h"
#include "Engine/UI/WTextField.h"
#include "Engine/UI/WTextBox.h"
#include "Engine/UI/WListBox.h"
#include "Engine/UI/WComboBox.h"
#include "Engine/UI/WConsoleLog.h"
#include "Engine/UI/WWindow.h"
#include "Engine/UI/WTabbedPane.h"
#include "Engine/UI/WFrame.h"
#include "Engine/UI/WTree.h"
#include "Engine/UI/WTable.h"
#include "Engine/UI/WDummy.h"
#include "Engine/UI/WCanvas.h"
#include "Engine/UI/WInspectorTab.h"
#include "Engine/UI/WInspector.h"
#include "Engine/UI/WGraph.h"
#include "Engine/UI/WSprite.h"

typedef H_WND			(__stdcall* CreateFn)(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int width, int height, H_WND hwndParent, HMENU hMenu, LPVOID lpParam); 
static H_WND			CreateComponentEx(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int width, int height, H_WND hwndParent, HMENU hMenu, LPVOID lpParam);
static H_WND			CreateComponent(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int width, int height, H_WND hwndParent, HMENU hMenu, LPVOID lpParam);
static H_FONT			CreateFontQ(const char* sFontFile, unsigned int iFontSize, unsigned int iFontDPI);
static bool				SelectFontQ(H_FONT hFont);
static LRESULT			SendMessageQ(H_WND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void				DrawString(LPCSTR lpStr, int x, int y, int anchor);
static void				FillRect(Rect* rect, int r, int g, int b, int a);
static void				DrawRect(Rect* rect, int r, int g, int b, int a, int iStrokeWidth = 1);
static void				SetColorQ(int r, int g, int b, int a);
static void				ResetColorQ();
static void				SetClip(int x, int y, int width, int height);
static int				GetStringWidthTillPos(LPCSTR cStr, int iPos);
static int				GetCharWidth(int c);
static int				GetGlyphU(int c);
static int				GetGlyphV(int c);

struct WComponentFactory 
{
	private:
							WComponentFactory();
							WComponentFactory(const WComponentFactory&);
		WComponentFactory&	operator=(const WComponentFactory&) { return *this; }

		typedef std::map<std::string, CreateFn> ComponentMap;
	public:
		~WComponentFactory() 
		{ 
			m_WComponentMap.clear(); 
		}
		
		ComponentMap		m_WComponentMap;

		static WComponentFactory* Get() 
		{
			static WComponentFactory factoryInstance;
			return &factoryInstance;
		}

		void Register(const std::string& lpClassName, CreateFn pfnCreate);
};

H_WND CreateComponentEx(	const char* lpClassName, 
							const char* lpWindowName, 
							DWORD dwStyle, 
							int x, 
							int y, 
							int width, 
							int height, 
							H_WND hwndParent, 
							HMENU hMenu, 
							LPVOID lpParam) 
{
	dwStyle |= WM_CUSTOM;
	return CreateComponent(lpClassName, lpWindowName, dwStyle, x, y, width, height, hwndParent, hMenu, lpParam);
}

H_WND CreateComponent(	const char* lpClassName, 
						const char* lpWindowName, 
						DWORD dwStyle, 
						int x, 
						int y, 
						int width, 
						int height, 
						H_WND hwndParent, 
						HMENU hMenu, 
						LPVOID lpParam) 
{
	WComponentFactory* factory = WComponentFactory::Get();

	std::map<std::string, CreateFn>::iterator itr = factory->m_WComponentMap.find(lpClassName);
	if( itr != factory->m_WComponentMap.end() ) 
	{
		return itr->second(lpClassName, lpWindowName, dwStyle, x, y, width, height, hwndParent, hMenu, lpParam);
	}

	return NULL;
}

H_FONT CreateFontQ(const char* sFontFile, unsigned int iFontSize, unsigned int iFontDPI) 
{
	return WWidgetManager::getInstance()->loadFont(sFontFile, iFontSize, iFontDPI);
}

bool SelectFontQ(H_FONT hFont) 
{
	return WWidgetManager::getInstance()->selectFont(hFont);
}

LRESULT SendMessageQ(H_WND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	if(hwnd != NULL) 
	{
		const char* lpClassName = ((WComponent*)hwnd)->getClassName();

		WComponentFactory* factory = WComponentFactory::Get();
		std::map<std::string, CreateFn>::iterator itr = factory->m_WComponentMap.find(lpClassName);
		if( itr != factory->m_WComponentMap.end() ) 
		{
			return ((WComponent*)hwnd)->OnSendMessage(msg, wParam, lParam);
		}
	}

	return WM__ERR;
}

void DrawString(LPCSTR lpStr, int x, int y, int anchor) 
{
	WWidgetManager::getInstance()->drawStringFont(lpStr, x, y, anchor);
}

void FillRect(Rect* rect, int r, int g, int b, int a) 
{
	WWidgetManager::getInstance()->fillRect(r, g, b, a, rect);
}

void DrawRect(Rect* rect, int r, int g, int b, int a, int iStrokeWidth) 
{
	WWidgetManager::getInstance()->drawRect(r, g, b, a, rect, iStrokeWidth);
}

void SetColorQ(int r, int g, int b, int a) 
{
	WWidgetManager::setColor(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

void ResetColorQ() 
{
	WWidgetManager::resetColor();
}

void SetClip(int x, int y, int width, int height) 
{
	WWidgetManager::getInstance()->setClip(x, y, width, height);
}

int GetStringWidthTillPos(LPCSTR cStr, int iPos) 
{
	return WWidgetManager::getInstance()->getStringWidthTillPos(cStr, iPos);
}

int GetCharWidth(int c) 
{
	return WWidgetManager::getInstance()->getCharWidth(c);
}

int GetGlyphU(int c) 
{
	return WWidgetManager::getInstance()->getGlyphU(c);
}

int GetGlyphV(int c) 
{
	return WWidgetManager::getInstance()->getGlyphV(c);
}
#endif