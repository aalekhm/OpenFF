#pragma once
#include "Common/Defines.h"
#include "Engine/Timer.h"
#include <functional>
#include <unordered_map>
#include "Renderer/Graphics.h"

class GameEngine
{
	public:
	protected:
							GameEngine();
		void				createWindow(uint32_t iWidth, uint32_t iHeight, const char* sWindowTitle);

		virtual void		onCreate() = 0;
		virtual void		onFirstFrame() = 0;
		virtual void		onUpdate(uint32_t iDeltaTimeMs, uint64_t lElapsedTime) = 0;
		virtual void		onPaint(Graphics* pGraphics) = 0;
		virtual void		onPostPaint() = 0;
		virtual void		onDestroy() = 0;
		virtual void		onWindowResize(int32_t iNewWidth, int32_t iNewHeight) = 0;

		Graphics*			getGraphics();
		void				destroyWindow();

		uint64_t			getElapsedTime(DURATION_TYPE eDURATION_TYPE);
		void				setClearColour(Pixel pPixel);

		void				resizeWindow(int32_t iNewWidth, int32_t iNewHeight);

		static void			runOnMainThread(std::string sIdentifier, std::function<void()>&& pCallable);

		uint32_t			m_iWidth;
		uint32_t			m_iHeight;

		static std::function<void(int, int)>	m_fWindowResizeCallback;
	private:
		static void			onKeyCallback(GLFWwindow* pWindow, int iKey, int iScanCode, int iAction, int iModifierKeys);
		static void			onUnicodeCharCallback(GLFWwindow* pWindow, unsigned int iUnicodeChar);
		static void			onUnicodeCharModifierCallback(GLFWwindow* window, unsigned int iUnicodeChar, int iModifier);
		static void			onMouseButtonCallback(GLFWwindow* pWindow, int iButton, int iAction, int iModifierKeys);
		static void			onMouseCursorCallback(GLFWwindow* pWindow, double xpos, double ypos);
		static void			onDropCallback(GLFWwindow* pWindow, int count, const char** paths);

		void				init(uint32_t iWidth, uint32_t iHeight, const char* sWindowTitle);
		void				initGLFW(const char* sWindowTitle);

		void				onPostUpdate();

		void				update();

		void				executeMainThreadCallables();

		static GameEngine*										m_pInstance;

		GLFWwindow*												m_pGLFWWindow;
		Timer													m_EngineElapsedTimer;
		Timer													m_UpdateTimer;

		std::unique_ptr<Graphics>								m_pGraphics = nullptr;
		std::function<void(Graphics*)>							m_fRenderCallback = nullptr;
		std::function<void()>									m_fPostRenderCallback = nullptr;

		std::unordered_map<std::string, std::function<void()>>	m_vMainThreadCallables;
};