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

			// Create a window of the size of our loaded video.
			createWindow(m_pState.m_iWidth, m_pState.m_iHeight, "FFmpeg Video Player!");
		}

		virtual void onCreate()
		{
			setClearColour(Pixel(255, 255, 0, 255));
		}

		virtual void onFirstFrame()
		{
			glfwSetTime(0.0);
		}

		virtual void onUpdate(uint32_t iDeltaTimeMs, uint64_t lElapsedTime)
		{
			syncVideo();
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

		void syncVideo()
		{
			double fPresentationTimeInSeconds = m_pState.m_lPresentationTimeStamp * (double)(m_pState.m_TimeBase.num) / (double)(double)(m_pState.m_TimeBase.den);
			while (fPresentationTimeInSeconds > glfwGetTime())
			{
				glfwWaitEventsTimeout(fPresentationTimeInSeconds - glfwGetTime());
			}
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