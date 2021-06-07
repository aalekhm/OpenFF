#include <stdlib.h>
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "FFmpeg/FFmpeg.h"

#define NOT !

// Bartholomew ==> C++ Real-time video processing
// https://www.youtube.com/playlist?list=PLKucWgGjAuTbobNC28EaR9lbGQTVyD9IS
//
// ffmpeg - libav - tutorial - https://github.com/leandromoreira/ffmpeg-libav-tutorial

class VideoPlayer : public GameEngine
{
	public:
		VideoPlayer(const char* sInputVideoFile)
		{
			if (NOT FFmpeg::openReader(m_pState, sInputVideoFile))
			{
				std::cout << "Couldn't load video file\n";
			}

			if (NOT FFmpeg::readFrame(m_pState))
			{
				std::cout << "Couldn't load video frame\n";
			}

			// Create a window of the size of our loaded video.
			createWindow(m_pState.m_iWidth, m_pState.m_iHeight, "PixSoR Window!");
		}

		virtual void onCreate()
		{
			setClearColour(Pixel(255, 255, 0, 255));
		}

		virtual void onUpdate(uint32_t iDeltaTimeMs, uint64_t lElapsedTime)
		{
			FFmpeg::readFrame(m_pState);
		}

		virtual void onPaint(Graphics* pGraphics)
		{
			pGraphics->clear(m_pState.m_pRawFrameData);
		}

		virtual void onDestroy()
		{
			FFmpeg::closeReader(m_pState);
		}

	protected:
	private:
		VideoReaderState	m_pState;
};

int main(int argc, char** argv)
{
	VideoPlayer videoPlayer("../res/Hot Sexy Lady Animation.mp4");

	exit(EXIT_SUCCESS);
}