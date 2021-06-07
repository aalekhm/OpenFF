#include <stdlib.h>
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "FFmpeg/FFmpeg.h"
#include "Common/Defines.h"
#include <sstream>

#define NOT !

#define SRC_INPUT_WIDTH			640
#define SRC_INPUT_HEIGHT		960

#define DEST_OUTPUT_WIDTH		320
#define DEST_OUTPUT_HEIGHT		480

#define FRAMERATE				60

#define STRINGYFY(__value__)		#__value__
#define WxH(__width__, __height__)	STRINGYFY(__width__ ##x ## __height__)

class FFmpegScreenCapture : public GameEngine
{
	public:
		enum EFFmpegState
		{
			FFmpeg_Invalid = -1,
			FFmpeg_StreamFomDesktop_Init,
			FFmpeg_StreamToDesktop_Continous,
			FFmpeg_Exit
		};

		FFmpegScreenCapture()
		{
			setState(EFFmpegState::FFmpeg_StreamFomDesktop_Init);

			// Create a window of the size of our loaded video.
			createWindow(DEST_OUTPUT_WIDTH, DEST_OUTPUT_HEIGHT, "FFmpeg Video Client!");
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
			handleState(m_eState);
		}

		virtual void onPaint(Graphics* pGraphics)
		{
			streamToDesktop(pGraphics);
		}

		virtual void onDestroy()
		{
			FFmpeg::closeReader(m_pFFmpegState_StreamFromDesktop);
		}

		void setState(EFFmpegState eState)
		{
			m_eState = eState;
		}

		void streamToDesktop(Graphics* pGraphics)
		{
			pGraphics->clear(m_pFFmpegState_StreamFromDesktop.m_pRawFrameData);
		}

		bool initStreamFromDesktop(	FFmpegState& pFFmpegState,
									int32_t iSrcX, 
									int32_t iSrcY, 
									int32_t iSrcW, 
									int32_t iSrcH, 
									int32_t iDestW, 
									int32_t iDestH,
									int32_t iFramerate,
									AVPixelFormat IN_AVPixelFormat,
									AVPixelFormat OUT_AVPixelFormat)
		{
			bool bInitSuccessful = false;

			auto& pAVFormatContext	= pFFmpegState.m_pAVFormatContext;
			auto& pAVCodecContext	= pFFmpegState.m_pAVCodecContext;
			auto& pAVFrame			= pFFmpegState.m_pAVFrame;
			auto& pSwsContext		= pFFmpegState.m_pSwsContext;
			auto& pRawFrameData		= pFFmpegState.m_pRawFrameData;

			// Set the Window dimensions.
			pFFmpegState.m_iSrcWidth = iSrcW;
			pFFmpegState.m_iSrcHeight = iSrcH;
			pFFmpegState.m_iDstWidth = iDestW;
			pFFmpegState.m_iDstHeight = iDestH;

			// Initialize & register all the input and output devices.
			avdevice_register_all();

			// Query the 'Input Format' of the stream.
			AVInputFormat* pAVInputFormat = av_find_input_format(INPUT_READER_FORMAT);
			if (pAVInputFormat)
			{
				// Set the 'Format Options' of the 'Input Format'.
				AVDictionary* pFormatOptions = nullptr;
				{
					std::stringstream sStringStream;

					sStringStream << iSrcX;
					av_dict_set(&pFormatOptions, "offset_x", sStringStream.str().c_str(), 0);
					sStringStream.str("");

					sStringStream << iSrcY;
					av_dict_set(&pFormatOptions, "offset_y", sStringStream.str().c_str(), 0);
					sStringStream.str("");

					sStringStream << iFramerate;
					av_dict_set(&pFormatOptions, "framerate", sStringStream.str().c_str(), 0);
					sStringStream.str("");

					sStringStream << SRC_INPUT_WIDTH << 'x' << SRC_INPUT_HEIGHT;
					av_dict_set(&pFormatOptions, "video_size", sStringStream.str().c_str(), 0);
					sStringStream.str("");
				}

				// Try to open the 'Input Stream' of a specified format & options.
				if (avformat_open_input(&pAVFormatContext, INPUT_URL, pAVInputFormat, &pFormatOptions) == 0)
				{
					// Dump the 'Input Format Context" to the specified 'Input URL'
					av_dump_format(pAVFormatContext, 0, INPUT_URL, DUMP_TYPE_INPUT);

					// Free the 'Format Options' as we no longer need it.
					av_dict_free(&pFormatOptions);

					// Get an appropriate 'Decoder' that will decode the 'Input Stream'.
					// Desktop input stream returns 'Packets' in a '.bmp' format.
					AVCodec* pInput_AVCodec = avcodec_find_decoder(AV_CODEC_ID_BMP);
					if (pInput_AVCodec != nullptr)
					{
						// Allocate a 'Codec Context' depending on the specified 'Decoder'.
						pAVCodecContext = avcodec_alloc_context3(pInput_AVCodec);
						if(pAVCodecContext != nullptr)
						{
							pAVCodecContext->pix_fmt = OUT_AVPixelFormat;
							pAVCodecContext->width = iDestW;
							pAVCodecContext->height = iDestH;

							// Initialize the 'Codec Context' to use the given 'Codec'.
							if (NOT avcodec_open2(pAVCodecContext, pInput_AVCodec, NULL))
							{
								// Allocate a 'Frame' for future use.
								pAVFrame = av_frame_alloc();

								// Set the SwS Context' for scaling the input
								pSwsContext = sws_getContext(	iSrcW,					// Source Width of the Video Frame
																iSrcH,					// Source Height of the Video Frame
																IN_AVPixelFormat,		// Pixel Format of the Video Frame
																iDestW,					// Destination Width of the Video Frame
																iDestH,					// Destination Height of the Video Frame
																OUT_AVPixelFormat,		// Destination Image format
																SWS_BILINEAR,			// Specify the Algorithm use to do the rescaling
																NULL,
																NULL,
																NULL);

								if(pSwsContext != nullptr)
								{
									pRawFrameData = (uint8_t*)std::malloc(iDestW * iDestH * 4);
									bInitSuccessful = true;
								}
							}
						}
					}
				}
			}

			return bInitSuccessful;
		}
		
		bool streamFromDesktop(FFmpegState& pFFmpegState)
		{
			bool bCaptureSuccessfull = false;
			{
				auto& pAVFormatContext	= pFFmpegState.m_pAVFormatContext;
				auto& pAVCodecContext	= pFFmpegState.m_pAVCodecContext;
				auto& pAVFrame			= pFFmpegState.m_pAVFrame;
				auto& pSwsContext		= pFFmpegState.m_pSwsContext;
				auto& pRawFrameData		= pFFmpegState.m_pRawFrameData;

				// Read the incoming 'Packet'
				AVPacket packet;
				if (av_read_frame(pAVFormatContext, &packet) == 0)
				{
					// Send the 'Packet' to the 'Decoder'
					if (avcodec_send_packet(pAVCodecContext, &packet) == 0)
					{
						// Receive the 'Decoded' 'Frame'
						if (avcodec_receive_frame(pAVCodecContext, pAVFrame) == 0)
						{
							// Construct a 'Raw' pixel frame & save it in our 'm_pRawFrameData'.
							{
								uint8_t* pDest[4] = { pRawFrameData, NULL, NULL, NULL };
								int iDestLineSize[4] = { DEST_OUTPUT_WIDTH * 4, 0, 0, 0 };
							
								sws_scale(	pSwsContext, 
											pAVFrame->data, 
											pAVFrame->linesize, 
											0, 
											pAVFrame->height, 
											pDest, 
											iDestLineSize);
							}

							// De-Reference the 'Packet' as we no longer need it.
							av_packet_unref(&packet);

							bCaptureSuccessfull = true;
						}
					}
				}
			}

			return bCaptureSuccessfull;
		}

		void handleState(EFFmpegState eState)
		{
			switch (m_eState)
			{
				case EFFmpegState::FFmpeg_StreamFomDesktop_Init:
				{
					bool bInitSuccessful = initStreamFromDesktop(	m_pFFmpegState_StreamFromDesktop,
																	0, 
																	0,
																	SRC_INPUT_WIDTH, 
																	SRC_INPUT_HEIGHT, 
																	DEST_OUTPUT_WIDTH, 
																	DEST_OUTPUT_HEIGHT, 
																	FRAMERATE,
																	AV_PIX_FMT_BGR0,
																	AV_PIX_FMT_RGBA);

					if (bInitSuccessful)
					{
						setState(EFFmpegState::FFmpeg_StreamToDesktop_Continous);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_StreamToDesktop_Continous:
				{
					if (NOT streamFromDesktop(m_pFFmpegState_StreamFromDesktop))
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_Exit:
				{

				}
				break;
			}
		}
	protected:
	private:
		FFmpegState			m_pFFmpegState_StreamFromDesktop;
		EFFmpegState		m_eState;
};

int main(int argc, char** argv)
{
	FFmpegScreenCapture screenRecorder;

	exit(EXIT_SUCCESS);
}