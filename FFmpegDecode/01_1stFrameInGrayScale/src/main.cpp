#include <stdlib.h>
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "FFmpeg/FFmpeg.h"

#define NOT !

// Bartholomew ==> C++ Real-time video processing
// https://www.youtube.com/playlist?list=PLKucWgGjAuTbobNC28EaR9lbGQTVyD9IS
//
// ffmpeg - libav - tutorial - https://github.com/leandromoreira/ffmpeg-libav-tutorial

class MyEngine : public GameEngine
{
	public:
		MyEngine()
		{
			if (!FFmpeg::loadFrame("../res/Hot Sexy Lady Animation.mp4", m_iWidth, m_iHeight, &m_pFrameData))
			{
				std::cout << "Couldn't load video file\n";
			}

			// Create a window of the size of our loaded video.
			createWindow(m_iWidth, m_iHeight, "PixSoR Window!");
		}

		virtual void onCreate()
		{
			setClearColour(Pixel(255, 255, 0, 255));

		}

		virtual void onUpdate(uint32_t iDeltaTimeMs, uint64_t lElapsedTime)
		{
		}

		virtual void onPaint(Graphics* pGraphics)
		{
			pGraphics->clear(m_pFrameData);
		}

		virtual void onDestroy()
		{

		}


	protected:
	private:
		uint8_t*	m_pFrameData = nullptr;
};

int main(int argc, char** argv)
{
	MyEngine me;

	exit(EXIT_SUCCESS);
}