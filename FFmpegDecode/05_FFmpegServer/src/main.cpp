#include "Network/NetDefines.h"
#include <stdlib.h>
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "FFmpeg/FFmpeg.h"
#include "Common/Defines.h"
#include "Network/NetServer.h"
#include "Buffer/BufferedStreamWriter.h"

#define NOT !

struct DisplayHeader
{
	char		MAGIC[4] = { 'D', 'I', 'S', 'P' };
	uint32_t	m_iWidth;
	uint32_t	m_iHeight;
	uint32_t	m_iExtraDataSize;
};

struct DataHeader
{
	char		MAGIC[4] = { 'D', 'A', 'T', 'A' };
	size_t		m_iDataSize;
};

// Bartholomew ==> C++ Real-time video processing
// https://www.youtube.com/playlist?list=PLKucWgGjAuTbobNC28EaR9lbGQTVyD9IS
//
// ffmpeg - libav - tutorial - https://github.com/leandromoreira/ffmpeg-libav-tutorial
class VideoServer : public GameEngine
{
	public:
		enum EState
		{
			Invalid = -1,
			InitVideo,
			InitServer,
			StreamVideo,
		};

		VideoServer(const char* sInputVideoFile)
		: m_sInputVideoFile(sInputVideoFile)
		{
			setState(EState::InitVideo);

			// Create a window of the size of our loaded video.
			createWindow(m_pState.m_iWidth, m_pState.m_iHeight, "FFmpeg Video Server!");
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
			switch (m_eState)
			{
				case EState::InitServer:
				{
					if (m_pNetServer.netInitialize())
					{
						if (m_pNetServer.netListen(IPV4_LOCALHOST, PORT, 1) > 0)
						{
							DisplayHeader sDisplayHeader;
							sDisplayHeader.m_iWidth = m_pState.m_iWidth;
							sDisplayHeader.m_iHeight = m_pState.m_iHeight;
							sDisplayHeader.m_iExtraDataSize = m_pState.m_pAVStream->codecpar->extradata_size;
						
							m_pNetServer.netSendAll(	(uint8_t*)&sDisplayHeader, 
														sizeof(DisplayHeader));

							m_pNetServer.netSendAll(	m_pState.m_pAVStream->codecpar->extradata, 
														sDisplayHeader.m_iExtraDataSize);

							setState(EState::StreamVideo);
						}
					}
				}
				break;
				case EState::StreamVideo:
				{
					syncVideo();

					static std::function<void(AVPacket*)> m_fSendPacket = [this](AVPacket* pAVPacket)
					{
						// Send AVPAcket->data
						{
							m_sDataHeader.m_iDataSize = pAVPacket->size;
							m_pNetServer.netSendAll(	(uint8_t*)&m_sDataHeader, 
														sizeof(DataHeader));
							
							m_pNetServer.netSendAll(	pAVPacket->data,
														m_sDataHeader.m_iDataSize);
						}
					};

					FFmpeg::readFrame(m_pState, m_fSendPacket);
				}
				break;
			}
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

		void setState(EState eState)
		{
			m_eState = eState;
			switch (m_eState)
			{
				case EState::InitVideo:
				{
					if (FFmpeg::openReader(m_pState, m_sInputVideoFile.c_str()))
					{
						setState(EState::InitServer);
					}
				}
				break;
			}
		}

	protected:
	private:
		std::string			m_sInputVideoFile;
		VideoReaderState	m_pState;
		NetServer			m_pNetServer;

		DataHeader			m_sDataHeader;
		EState				m_eState;
};

int main(int argc, char** argv)
{
	VideoServer videoPlayer("../res/Hot Sexy Lady Animation.mp4");

	exit(EXIT_SUCCESS);
}