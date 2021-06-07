#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include <memory>

#define FRAME_START \
{ \
	TIMER_STOP(m_UpdateTimer); \
	TIMER_START(m_UpdateTimer); \
} \

#define UPDATE_PROLOGUE \
{ \
	FRAME_START \
} \

#define UPDATE_EPILOGUE \
{ \
	glfwSwapBuffers(m_pGLFWWindow); \
	glfwPollEvents(); \
} \


void GameEngine::createWindow(uint32_t iWidth, uint32_t iHeight, const char* sWindowTitle)
{
	init(iWidth, iHeight, sWindowTitle);
	onCreate();
	update();
}

void GameEngine::destroyWindow()
{
	glfwSetWindowShouldClose(m_pGLFWWindow, GLFW_TRUE);
}

void GameEngine::init(uint32_t iWidth, uint32_t iHeight, const char* sWindowTitle)
{
	TIMER_START_NEW(t)
	{
		m_iWidth = iWidth;
		m_iHeight = iHeight;

		initGLFW(sWindowTitle);

		m_pGraphics = Graphics::create(m_iWidth, m_iHeight);
		m_fRenderCallback = std::bind(&GameEngine::onPaint, this, std::placeholders::_1);
		m_fPostRenderCallback = std::bind(&GameEngine::onPostPaint, this);

		TIMER_START(m_EngineElapsedTimer)
	}
	TIMER_STOP(t)

	LOG_CONSOLE("Time Taken by init() : " << t.duration(DURATION_TYPE::milliseconds))
}

void error_callback(int error, const char* msg)
{
	std::string s;
	s = " [" + std::to_string(error) + "] " + msg + '\n';
	std::cerr << s << std::endl;
}

void GameEngine::initGLFW(const char* sWindowTitle)
{
	glfwSetErrorCallback(error_callback);

	m_pGLFWWindow = nullptr;
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	m_pGLFWWindow = glfwCreateWindow(m_iWidth, m_iHeight, sWindowTitle, nullptr, nullptr);
	if (m_pGLFWWindow == nullptr)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(m_pGLFWWindow,				onKeyCallback);
	glfwSetCharCallback(m_pGLFWWindow,				onUnicodeCharCallback);
	glfwSetCharModsCallback(m_pGLFWWindow,			onUnicodeCharModifierCallback);
	glfwSetMouseButtonCallback(m_pGLFWWindow,		onMouseButtonCallback);
	glfwSetCursorPosCallback(m_pGLFWWindow,			onMouseCursorCallback);
	
	// This function makes the OpenGL or OpenGL ES context of the specified window current on the calling thread.
	glfwMakeContextCurrent(m_pGLFWWindow);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		exit(EXIT_FAILURE);
	}

	glfwSwapInterval(1);
}

void GameEngine::update()
{
	FRAME_START
	onFirstFrame();
	while (!glfwWindowShouldClose(m_pGLFWWindow))
	{
		UPDATE_PROLOGUE
		{
			// Update & Render Game here...
			onUpdate(m_UpdateTimer.duration(DURATION_TYPE::milliseconds), getElapsedTime(DURATION_TYPE::milliseconds));
			m_pGraphics->paint( m_fRenderCallback );
			m_fPostRenderCallback();
		}
		UPDATE_EPILOGUE
	}

	onDestroy();
	glfwDestroyWindow(m_pGLFWWindow);
}

void GameEngine::setClearColour(Pixel pPixel)
{
	m_pGraphics->setClearColour(pPixel);
}

uint64_t GameEngine::getElapsedTime(DURATION_TYPE eDURATION_TYPE)
{
	TIMER_STOP(m_EngineElapsedTimer)
	return m_EngineElapsedTimer.duration(eDURATION_TYPE);
}

void GameEngine::onKeyCallback(GLFWwindow* pWindow, int iKey, int iScanCode, int iAction, int iModifierKeys)
{
	if (iAction == GLFW_PRESS)
	{
		InputManager::get()->onKeyPressed(iKey, iScanCode, iAction, iModifierKeys);
	}
	else
	if (iAction == GLFW_RELEASE)
	{
		InputManager::get()->onKeyReleased(iKey, iScanCode, iAction, iModifierKeys);
	}
}

void GameEngine::onUnicodeCharCallback(GLFWwindow* window, unsigned int iUnicodeChar)
{
	InputManager::get()->onUnicodeChar(iUnicodeChar);
}

void GameEngine::onUnicodeCharModifierCallback(GLFWwindow* window, unsigned int iUnicodeChar, int iModifier)
{
	std::cout << iUnicodeChar << " " << iModifier << "\n";
	InputManager::get()->onUnicodeCharModifier(iUnicodeChar, iModifier);
}

void GameEngine::onMouseButtonCallback(GLFWwindow* pWindow, int iButton, int iAction, int iModifierKeys)
{
	if (iAction == GLFW_PRESS)
	{
		InputManager::get()->onMousePressed(iButton);
	}
	else
	if (iAction == GLFW_RELEASE)
	{
		InputManager::get()->onMouseReleased(iButton);
	}
}

void GameEngine::onMouseCursorCallback(GLFWwindow* pWindow, double xpos, double ypos)
{
	InputManager::get()->onMouseMoved(xpos, ypos);
}

Graphics* GameEngine::getGraphics()
{
	return m_pGraphics.get();
}
