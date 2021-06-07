#include "Network/NetDefines.h"
#include <stdlib.h>
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "FFmpeg/FFmpeg.h"
#include "Common/Defines.h"
#include "Network/NetClient.h"
#include "Buffer/BufferData.hpp"

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
class VideoClient : public GameEngine
{
	public:
		enum EState
		{
			Invalid = -1,
			InitClient,
			InitDisplay,
			StreamVideo,
		};

		VideoClient()
		{
			setState(EState::InitClient);

			// Create a window of the size of our loaded video.
			createWindow(m_pState.m_iWidth, m_pState.m_iHeight, "FFmpeg Video Client!");
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
			int32_t iRet = -1;
			switch (m_eState)
			{
				case EState::StreamVideo:
				{
					// Receive 'AVPacket' size.
					m_pNetClient.netRecvAll(	&m_sDataHeader,
												sizeof(DataHeader));

					// Receive 'AVPacket' contents.
					BufferData pBufferData(m_sDataHeader.m_iDataSize);
					m_pNetClient.netRecvAll(	pBufferData.m_pBuffer,
												m_sDataHeader.m_iDataSize);

					// Start 'AVPacket' decoding.
					{

						// Create a new 'AVPacket' for our use.
						if((iRet = av_new_packet(m_pState.m_pAVPacket, m_sDataHeader.m_iDataSize)) == 0)
						{
							// Copy the received 'AVPacket' contents.
							memcpy(m_pState.m_pAVPacket->data, pBufferData.m_pBuffer, m_sDataHeader.m_iDataSize);
							{
								// Check 'AVPacket' data validity
								{
									uint8_t* pDataIn = m_pState.m_pAVPacket->data;
									int iSizeIn = m_pState.m_pAVPacket->size;
									uint8_t* pDataOut = NULL;
									int iSizeOut = 0;
									
									iRet = av_parser_parse2(	m_pState.m_pAVCodecParserContext,
																m_pState.m_pAVCodecContext, 
																&pDataOut, &iSizeOut,
																pDataIn, iSizeIn,
																AV_NOPTS_VALUE, AV_NOPTS_VALUE, -1);
									assert(iRet == iSizeIn);
									(void)iRet;
									assert(iSizeOut == iSizeIn);
								}

								if (m_pState.m_pAVCodecParserContext->key_frame == 1)
								{
									m_pState.m_pAVPacket->flags |= AV_PKT_FLAG_KEY;
								}

								// Send 'AVPacket' for decoding.
								if ((iRet = avcodec_send_packet(m_pState.m_pAVCodecContext, m_pState.m_pAVPacket)) == 0)
								{
									// Extract the 'AVFrame' from the 'AVPacket'.
									if ((iRet = avcodec_receive_frame(m_pState.m_pAVCodecContext, m_pState.m_pAVFrame)) == 0)
									{
										// Construct a 'Raw' pixel frame & save it in our 'm_pRawFrameData'.
										uint8_t* pDest[4] = { m_pState.m_pRawFrameData, NULL, NULL, NULL };
										int iDestLineSize[4] = { m_pState.m_pAVFrame->width * 4, 0, 0, 0 };

										sws_scale(m_pState.m_pSwsContext, m_pState.m_pAVFrame->data, m_pState.m_pAVFrame->linesize, 0, m_pState.m_pAVFrame->height, pDest, iDestLineSize);
									}
								}
								else
								{
									ASSERT("ERROR avcodec_send_packet ", iRet);
								}
							}

							// DO NOT forget to dereference the 'AVPacket'.
							av_packet_unref(m_pState.m_pAVPacket);
						}
						else
						{
							ASSERT("Couldn't allocate av_new_packet", iRet);
						}
					}
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
				case EState::InitClient:
				{
					if (m_pNetClient.netInitialize())
					{
						if (m_pNetClient.netConnect(IPV4_LOCALHOST, PORT))
						{
							setState(EState::InitDisplay);
						}
					}
				}
				break;
				case EState::InitDisplay:
				{
					DisplayHeader sDisplayHeader;
					m_pNetClient.netRecvAll(&sDisplayHeader, sizeof(DisplayHeader));

					uint8_t* pExtraData = (uint8_t*)std::malloc(sDisplayHeader.m_iExtraDataSize);
					m_pNetClient.netRecvAll(pExtraData, sDisplayHeader.m_iExtraDataSize);

					m_pState.m_iWidth = sDisplayHeader.m_iWidth;
					m_pState.m_iHeight = sDisplayHeader.m_iHeight;

					{
						m_pState.m_pRawFrameData = (uint8_t*)std::malloc(m_pState.m_iWidth * m_pState.m_iHeight * 4);
						memset(m_pState.m_pRawFrameData, 0, m_pState.m_iWidth * m_pState.m_iHeight * 4);

						m_pState.m_pAVFormatContext = avformat_alloc_context();
						if(m_pState.m_pAVFormatContext != nullptr)
						{
							m_pState.m_pAVCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
							if(m_pState.m_pAVCodec != nullptr)
							{
								m_pState.m_pAVCodecParserContext = av_parser_init(AV_CODEC_ID_H264);
								if(m_pState.m_pAVCodecParserContext != nullptr)
								{
									m_pState.m_pAVCodecParserContext->flags |= PARSER_FLAG_COMPLETE_FRAMES;

									m_pState.m_pAVCodecContext = avcodec_alloc_context3(m_pState.m_pAVCodec);
									if(m_pState.m_pAVCodecContext != nullptr)
									{
										m_pState.m_pAVCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
										m_pState.m_pAVCodecContext->width = m_pState.m_iWidth;
										m_pState.m_pAVCodecContext->height = m_pState.m_iHeight;

										m_pState.m_pAVCodecContext->extradata_size = sDisplayHeader.m_iExtraDataSize;
										m_pState.m_pAVCodecContext->extradata = pExtraData;
							
										if (NOT avcodec_open2(m_pState.m_pAVCodecContext, m_pState.m_pAVCodec, NULL))
										{
											m_pState.m_pAVPacket = av_packet_alloc();
											m_pState.m_pAVFrame = av_frame_alloc();

											if(m_pState.m_pSwsContext == nullptr)
											{
												m_pState.m_pSwsContext = sws_getContext(	m_pState.m_iWidth,			// Source Width of the Video Frame
																							m_pState.m_iHeight,			// Source Height of the Video Frame
																							AV_PIX_FMT_YUV420P,			// Pixel Format of the Video Frame
																							m_pState.m_iWidth,			// Destination Width of the Video Frame
																							m_pState.m_iHeight,			// Destination Height of the Video Frame
																							AV_PIX_FMT_RGB0,			// Destination Image format
																							SWS_BILINEAR,				// Specify the Algorithm use to do the rescaling
																							NULL,
																							NULL,
																							NULL);
												
												if (NOT m_pState.m_pSwsContext)
												{
													ASSERT("Couldn't get sws_getContext ", m_pState.m_pSwsContext);
												}
											}
										}
										else
										{
											ASSERT("Couldn't open avcodec_open2 ", 0);
										}
									}
									else
									{
										ASSERT("Couldn't open avcodec_alloc_context3 ", 0);
									}
								}
								else
								{
									ASSERT("Couldn't initialize av_parser_init ", 0);
								}
							}
							else
							{
								ASSERT("Couldn't find avcodec_find_decoder ", 0);
							}
						}
						else
						{
							ASSERT("Couldn't allocate avformat_alloc_context ", 0);
						}
					}

					setState(EState::StreamVideo);
				}
				break;
			}
		}
	protected:
	private:
		VideoReaderState	m_pState;
		NetClient			m_pNetClient;

		DataHeader			m_sDataHeader;
		EState				m_eState;
};

int main(int argc, char** argv)
{
	VideoClient videoClient;

	exit(EXIT_SUCCESS);
}