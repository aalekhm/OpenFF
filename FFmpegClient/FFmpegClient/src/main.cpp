#include "Network/NetDefines.h"
#include <stdlib.h>
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "FFmpeg/FFmpeg.h"
#include "Common/Defines.h"
#include "Buffer/BufferData.hpp"

#define NOT !
#define DEST_SCALE				0.5

// Bartholomew ==> C++ Real-time video processing
// https://www.youtube.com/playlist?list=PLKucWgGjAuTbobNC28EaR9lbGQTVyD9IS
//
// ffmpeg - libav - tutorial - https://github.com/leandromoreira/ffmpeg-libav-tutorial
class FFmpegClient : public GameEngine
{
	public:
		enum EState
		{
			FFmpeg_Invalid = -1,
			FFmpeg_NetworkClient_Init,
			FFmpeg_StreamToDesktop_Init,
			FFmpeg_StreamFromNetwork_Continous,
			FFmpeg_Exit
		};

		FFmpegClient()
		{
			handleState(EState::FFmpeg_NetworkClient_Init);
			handleState(EState::FFmpeg_StreamToDesktop_Init);

			// Create a window of the size of our loaded video.
			createWindow(m_pFFmpegState_StreamToDesktop.m_iDstWidth, m_pFFmpegState_StreamToDesktop.m_iDstHeight, "FFmpeg Video Client!");
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
			FFmpeg::closeReader(m_pFFmpegState_StreamToDesktop);
		}

		void streamToDesktop(Graphics* pGraphics)
		{
			pGraphics->clear(m_pFFmpegState_StreamToDesktop.m_pRawFrameData);
		}

		void syncVideo()
		{
			double fPresentationTimeInSeconds = m_pFFmpegState_StreamToDesktop.m_lPresentationTimeStamp * (double)(m_pFFmpegState_StreamToDesktop.m_TimeBase.num) / (double)(double)(m_pFFmpegState_StreamToDesktop.m_TimeBase.den);
			while (fPresentationTimeInSeconds > glfwGetTime())
			{
				glfwWaitEventsTimeout(fPresentationTimeInSeconds - glfwGetTime());
			}
		}

		void setState(EState eState)
		{
			m_eState = eState;
		}

		bool initStreamToDesktop(	FFmpegState& pFFmpegState, 
									int32_t iSourceWidth, 
									int32_t iSourceHeight, 
									int32_t iDestWidth,
									int32_t iDestHeight,
									bool bHasExtraData,
									int32_t iExtraDataSize,
									uint8_t* pExtraData,
									AVPixelFormat IN_AVPixelFormat,
									AVPixelFormat OUT_AVPixelFormat)
		{
			bool bInitSuccessful = false;

			auto& pAVFormatContext		= pFFmpegState.m_pAVFormatContext;
			auto& pAVCodecContext		= pFFmpegState.m_pAVCodecContext;
			auto& pAVCodec				= pFFmpegState.m_pAVCodec;
			auto& pAVPacket				= pFFmpegState.m_pAVPacket;
			auto& pAVFrame				= pFFmpegState.m_pAVFrame;
			auto& pSwsContext			= pFFmpegState.m_pSwsContext;
			auto& pRawFrameData			= pFFmpegState.m_pRawFrameData;
			auto& pAVCodecParserContext	= pFFmpegState.m_pAVCodecParserContext;
			auto& iSrcWidth				= pFFmpegState.m_iSrcWidth;
			auto& iSrcHeight			= pFFmpegState.m_iSrcHeight;
			auto& iDstWidth				= pFFmpegState.m_iDstWidth;
			auto& iDstHeight			= pFFmpegState.m_iDstHeight;

			iSrcWidth = iSourceWidth;
			iSrcHeight = iSourceHeight;
			iDstWidth = iDestWidth;
			iDstHeight = iDestHeight;

			pRawFrameData = (uint8_t*)std::malloc(iDestWidth * iDestHeight * 4);
			memset(pRawFrameData, 0, iDestWidth * iDestHeight * 4);

			pAVFormatContext = avformat_alloc_context();
			if(pAVFormatContext != nullptr)
			{
				pAVCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
				if(pAVCodec != nullptr)
				{
					pAVCodecParserContext = av_parser_init(AV_CODEC_ID_H264);
					if(pAVCodecParserContext != nullptr)
					{
						pAVCodecParserContext->flags |= PARSER_FLAG_COMPLETE_FRAMES;

						pAVCodecContext = avcodec_alloc_context3(pAVCodec);
						if(pAVCodecContext != nullptr)
						{
							pAVCodecContext->pix_fmt = OUT_AVPixelFormat;
							pAVCodecContext->width = iDestWidth;
							pAVCodecContext->height = iDestHeight;

							if(bHasExtraData)
							{
								pAVCodecContext->extradata_size = iExtraDataSize;
								pAVCodecContext->extradata = pExtraData;
							}
							
							if (NOT avcodec_open2(pAVCodecContext, pAVCodec, NULL))
							{
								pAVPacket = av_packet_alloc();
								pAVFrame = av_frame_alloc();

								if(pSwsContext == nullptr)
								{
									pSwsContext = sws_getContext(	iSourceWidth,				// Source Width of the Video Frame
																	iSourceHeight,				// Source Height of the Video Frame
																	IN_AVPixelFormat,			// Pixel Format of the Video Frame
																	iDestWidth,					// Destination Width of the Video Frame
																	iDestHeight,				// Destination Height of the Video Frame
																	OUT_AVPixelFormat,			// Destination Image format
																	SWS_BILINEAR,				// Specify the Algorithm use to do the rescaling
																	NULL,
																	NULL,
																	NULL);
												
									if (pSwsContext)
									{
										bInitSuccessful = true;
									}
								}
							}
						}
					}
				}
			}
			return bInitSuccessful;
		}

		bool initNetworkClient(FFmpegNetworkState& pFFmpegNetworkState, int64_t iNetworkAddress, int32_t iNetworkPort)
		{
			bool bInitSuccessful = false;

			auto& pNetClient		= pFFmpegNetworkState.m_NetClient;
			auto& iIPAddress		= pNetClient.m_iIPAddress;
			auto& iPort				= pNetClient.m_iPort;
			auto& sDisplayHeader	= pFFmpegNetworkState.m_DisplayHeader;
			auto& pExtraData		= pFFmpegNetworkState.m_pExtraData;

			iIPAddress = iNetworkAddress;
			iPort = iNetworkPort;

			if (pNetClient.netInitialize())
			{
				if (pNetClient.netConnect(iIPAddress, iPort))
				{
					bool bHasExtraData = false;

					// Receive 'Handshake' header.
					{
						pNetClient.netRecvAll(&sDisplayHeader, sizeof(DisplayHeader));

						if (sDisplayHeader.m_iExtraDataSize > 0)
						{
							bHasExtraData = true;

							pExtraData = (uint8_t*)std::malloc(sDisplayHeader.m_iExtraDataSize);
							pNetClient.netRecvAll(pExtraData, sDisplayHeader.m_iExtraDataSize);
						}

						bInitSuccessful = true;
					}
				}
			}

			return bInitSuccessful;
		}

		void continousStreamFromNetwork(	FFmpegNetworkState& pFFmpegNetworkState,
											FFmpegState& pFFmpegState)
		{
			BufferData pBufferData;
			auto& sDataHeader = pFFmpegNetworkState.m_DataHeader;

			// Receive 'AVPacket'.
			{
				// 'AVPacket' size.	
				pFFmpegNetworkState.m_NetClient.netRecvAll(	&sDataHeader,
															sizeof(DataHeader));

				// 'AVPacket' contents.
				pBufferData.setSize(sDataHeader.m_iDataSize);
				pFFmpegNetworkState.m_NetClient.netRecvAll(	pBufferData.m_pBuffer,
															sDataHeader.m_iDataSize);
			}

			auto& pAVCodecContext		= pFFmpegState.m_pAVCodecContext;
			auto& pAVFrame				= pFFmpegState.m_pAVFrame;
			auto& pAVCodecParserContext	= pFFmpegState.m_pAVCodecParserContext;
			auto& pRawFrameData			= pFFmpegState.m_pRawFrameData;
			auto& pSwsContext			= pFFmpegState.m_pSwsContext;

			// Start 'AVPacket' decoding.
			{
				int32_t iRet = -1;

				AVPacket pAVPacket;
				// Create a new 'AVPacket' for our use.
				if(av_new_packet(&pAVPacket, sDataHeader.m_iDataSize) == 0)
				{
					// Copy the received 'AVPacket' contents.
					memcpy(pAVPacket.data, pBufferData.m_pBuffer, sDataHeader.m_iDataSize);
					{
						// Check 'AVPacket' data validity
						{
							uint8_t* pDataIn = pAVPacket.data;
							int iSizeIn = pAVPacket.size;
							uint8_t* pDataOut = NULL;
							int iSizeOut = 0;
									
							iRet = av_parser_parse2(	pAVCodecParserContext,
														pAVCodecContext, 
														&pDataOut, &iSizeOut,
														pDataIn, iSizeIn,
														AV_NOPTS_VALUE, AV_NOPTS_VALUE, -1);
							assert(iRet == iSizeIn);
							(void)iRet;
							assert(iSizeOut == iSizeIn);
						}

						if (pAVCodecParserContext->key_frame == 1)
						{
							pAVPacket.flags |= AV_PKT_FLAG_KEY;
						}

						// Send 'AVPacket' for decoding.
						if (avcodec_send_packet(pAVCodecContext, &pAVPacket) == 0)
						{
							// Extract the 'AVFrame' from the 'AVPacket'.
							if (avcodec_receive_frame(pAVCodecContext, pAVFrame) == 0)
							{
								// Construct a 'Raw' pixel frame & save it in our 'm_pRawFrameData'.
								uint8_t* pDest[4] = { pRawFrameData, NULL, NULL, NULL };
								int iDestLineSize[4] = { pFFmpegState.m_iDstWidth * 4, 0, 0, 0 };

								sws_scale(pSwsContext, pAVFrame->data, pAVFrame->linesize, 0, pFFmpegState.m_iSrcHeight, pDest, iDestLineSize);
							}
						}
					}

					// DO NOT forget to dereference the 'AVPacket'.
					av_packet_unref(&pAVPacket);
				}
			}
		}

		void handleState(EState eState)
		{
			switch (eState)
			{
				case EState::FFmpeg_NetworkClient_Init:
				{
					bool bInitSuccessful = initNetworkClient(m_pFFmpegNetworkState_Client, IPV4_LOCALHOST, PORT);

					if (bInitSuccessful)
					{
						setState(EState::FFmpeg_StreamToDesktop_Init);
					}
					else
					{
						setState(EState::FFmpeg_Exit);
					}
				}
				break;
				case EState::FFmpeg_StreamToDesktop_Init:
				{
					bool bInitSuccessful = initStreamToDesktop(	m_pFFmpegState_StreamToDesktop,
																m_pFFmpegNetworkState_Client.m_DisplayHeader.m_iWidth,
																m_pFFmpegNetworkState_Client.m_DisplayHeader.m_iHeight,
																m_pFFmpegNetworkState_Client.m_DisplayHeader.m_iWidth * DEST_SCALE,
																m_pFFmpegNetworkState_Client.m_DisplayHeader.m_iHeight * DEST_SCALE,
																(m_pFFmpegNetworkState_Client.m_DisplayHeader.m_iExtraDataSize > 0),
																m_pFFmpegNetworkState_Client.m_DisplayHeader.m_iExtraDataSize,
																m_pFFmpegNetworkState_Client.m_pExtraData,
																AV_PIX_FMT_YUV420P,
																AV_PIX_FMT_RGBA);

					if (bInitSuccessful)
					{
						setState(EState::FFmpeg_StreamFromNetwork_Continous);
					}
					else
					{
						setState(EState::FFmpeg_Exit);
					}
				}
				break;
				case EState::FFmpeg_StreamFromNetwork_Continous:
				{
					continousStreamFromNetwork(m_pFFmpegNetworkState_Client, m_pFFmpegState_StreamToDesktop);
				}
				break;
				case EState::FFmpeg_Exit:
				{

				}
				break;
			}
		}
	protected:
	private:
		FFmpegState			m_pFFmpegState_StreamToDesktop;
		FFmpegNetworkState	m_pFFmpegNetworkState_Client;

		EState				m_eState;
};

int main(int argc, char** argv)
{
	FFmpegClient videoClient;

	exit(EXIT_SUCCESS);
}