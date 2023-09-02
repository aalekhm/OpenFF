#include <stdlib.h>
#include <sstream>
#include <thread>
#include <future>
#include "Common/Defines.h"
#include "Common/ObjectQueue.hpp"
#include "Network/NetDefines.h"
#include "Network/NetServer.h"
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "Engine/UI/WWidgetManager.h"
#include "Engine/UI/WComponentFactory.h"
#include "Engine/Texture.h"
#include "FFmpeg/FFmpeg.h"
#include "Buffer/BufferedStreamWriter.h"
#include "Buffer/BufferData.hpp"
#include "QRCode/QrCode.hpp"

using namespace qrcodegen;

#define NOT !

#define DROID_SPLASH_WND_W				250
#define DROID_SPLASH_WND_H				250
#define DROID_UI_WND_H					300
#define DROID_UI_WND_GUTTER_2W			40
#define DROID_UI_WND_GUTTER_2H			80
#define DROID_UI_BUTTON_W				150
#define DROID_UI_COMPONENT_H			25

#define FRAMERATE						60
#define DEST_SCALE						0.40f
#define SERVER_CONNECT_DELAY			1500

#define DEVICE_SERVER_PATH				"/data/local/tmp/"
#define DEVICE_SOCKET_NAME				"AndroidRemoteControl"

#define ANDROID_SERVER_BINARY			"remote-server"
#define ANDROID_CLIENT_BINARY			"remote-client"
#define ANDROID_SERVER_PACKAGE_NAME		"com.ea.remotecontrol.RemoteServer"
#define ANDROID_CLIENT_PACKAGE_NAME		"com.ea.remotecontrol.RemoteClient"

#define RESOURCES						"../res/"

#define ID_TABBEDPANE_OPTIONS			2000
#define ID_BUTTON_RECORD				2001
#define ID_BUTTON_STOP					2002
#define ID_LABEL_FILENAME				2003
#define ID_TEXTFIELD_FILENAME			2004
#define ID_LABEL_DIRECTORY				2005
#define ID_TEXTFILED_DIRECTORY			2006
#define ID_CANVAS_QRCODE				2007
#define ID_BUTTON_FREEZE_ROTATION		2008
#define ID_BUTTON_THAW_ROTATION			2009
#define ID_LABEL_IMAGE_EXPORT_TYPE		2010
#define ID_CB_IMAGE_EXPORT_TYPE			2011
#define ID_BUTTON_DIRECTORY_LISTING		2012
#define ID_BUTTON_DIRECTORY_TREE		2013

#define MAGIC_KEYBOARD_EVENT			'K'
#define MAGIC_MOUSE_EVENT				'M'
#define MAGIC_START_SENDING_FRAME		'S'
#define MAGIC_FREEZE_ROTATION_EVENT		'F'
#define MAGIC_THAW_ROTATION_EVENT		'T'
#define MAGIC_QUIT_EVENT				'Q'
#define MAGIC_DIRECTORY_LISTING			'D'

HANDLE	m_HProc;
const float_t ONE_OVER_DEST_SCALE		= 1.0f / DEST_SCALE;

#pragma pack(1)
struct SWriteInfo
{
	char		MAGIC;		// 'K' - Keyboard Input
							// 'M' - Mouse Input
	int8_t		m_iSize;	// Size of the next data to follow
};

#pragma pack(1)
struct SPointerInfo
{
	uint8_t		m_iAction;
	uint8_t		m_iKey;
	uint32_t	m_iPosX;
	uint32_t	m_iPosY;
};

#pragma pack(1)
struct SKeyInfo
{
	uint8_t		m_iAction;
	uint32_t	m_iAndroidKeyCode;
	uint32_t	m_iAndroidMetaModifier;
};

// Bartholomew ==> C++ Real-time video processing
// https://www.youtube.com/playlist?list=PLKucWgGjAuTbobNC28EaR9lbGQTVyD9IS
//
// ffmpeg - libav - tutorial - https://github.com/leandromoreira/ffmpeg-libav-tutorial
class FFmpegYAGuiDroidCapture : public GameEngine
{
	public:
		enum EFFmpegState
		{
			FFmpeg_Invalid = -1,
			
			Droid_Splash,
			Android_StartTCPIP,
			Android_Connect,

			Android_PushServer,
			FFmpeg_PCClient_Init,

			Android_PushClient,
			FFmpeg_PCServer_Init,

			FFmpeg_AndroidStreamToDesktop_Init,
			FFmpeg_StreamFromNetwork_Continous,
			FFmpeg_StreamToDisk_Init,

			FFmpeg_Exit,
		};

		enum EStreamState
		{
			Invalid = -1,
			NetworkStream,
			NetworkStream_Record,
			FlushToDisk,
			FlushToDiskAndExit,
		};

		FFmpegYAGuiDroidCapture(std::string sInputVideoFile, std::string sIPAddress, ULONG ulIPAddress)
		: m_sInputVideoFile(sInputVideoFile)
		, IPV4_IPADDRESS_S(sIPAddress)
		, IPV4_IPADDRESS_UL(ulIPAddress)
		, m_bExitNetworkThread(false)
		{
			m_PacketQueue.PacketQueueInit();

			setState(EFFmpegState::Droid_Splash);
			createWindow(	DROID_SPLASH_WND_W, 
							DROID_SPLASH_WND_H,
							"Droid Capture!");
		}

		virtual void onCreate()
		{
			initYAGui();
			initCallbacks();
		}

		virtual void onFirstFrame()
		{
			glfwSetTime(0.0);
		}

		virtual void onUpdate(uint32_t iDeltaTimeMs, uint64_t lElapsedTime)
		{
			handleState(m_eState);
			
			m_pWidgetManager->update(iDeltaTimeMs, lElapsedTime);

			if (InputManager::get()->isKeyPressed(GLFW_KEY_ESCAPE))
			{
				if (m_eStreamState == EStreamState::NetworkStream_Record)
				{
					setStreamState(EStreamState::FlushToDiskAndExit);
				}
				else
				{
					m_bExitNetworkThread = true;
				}
			}
		}

		virtual void onPaint(Graphics* pGraphics)
		{
			streamToDesktop(pGraphics);
		}

		virtual void onPostPaint()
		{
			glViewport(0, 0, m_iWidth, m_iHeight);
			m_pWidgetManager->paint();
		}
		
		virtual void onWindowResize(int32_t iNewWidth, int32_t iNewHeight)
		{
			m_iWidth = iNewWidth;
			m_iHeight = iNewHeight;

			// Resize YAGui
			resizeYAGui(iNewWidth, iNewHeight);

			// Cleanup FFMepg & Restart
			FFmpeg::closeReader(m_pFFmpegState_StreamToDesktop);
			FFmpeg::closeReader(m_pFFmpegState_StreamToDisk);
			setState(EFFmpegState::FFmpeg_AndroidStreamToDesktop_Init);
		}

		virtual void onDestroy()
		{
			cleanUp();
		}

		void cleanUp()
		{
			sendMagic(MAGIC_QUIT_EVENT);

			adbRemoveBinary(ANDROID_SERVER_BINARY, DEVICE_SERVER_PATH);
			adbRemoveBinary(ANDROID_CLIENT_BINARY, DEVICE_SERVER_PATH);

			NetBase& netBase = getNetworkState();
			netBase.netClose();
			
			FFmpeg::closeReader(m_pFFmpegState_StreamToDesktop);
			FFmpeg::closeReader(m_pFFmpegState_StreamToDisk);
			
			m_PacketQueue.Clear();

			CloseHandle(m_HProc);
		}

		void streamToDesktop(Graphics* pGraphics)
		{
			if (m_PacketQueue.m_iNumberOfPackets > 0)
			{
				if (m_PacketQueue.PacketQueueGet(&m_AVPacketReusable) > 0)
				{
					decodeVideoPacket(m_pFFmpegState_StreamToDesktop, &m_AVPacketReusable);
				}
			}

			pGraphics->clear(m_pFFmpegState_StreamToDesktop.m_pRawFrameData);
		}

		void streamToNetwork(	FFmpegNetworkState& pFFmpegNetworkState, 
								FFmpegState& pIN_FFmpegState, 
								FFmpegState& pOUT_FFmpegState)
		{
			auto& sDataHeader		= pFFmpegNetworkState.m_DataHeader;
			auto& pNetServer		= pFFmpegNetworkState.m_NetServer;

			auto& pRawFrameData		= pIN_FFmpegState.m_pRawFrameData;

			auto& pAVCodecContext	= pOUT_FFmpegState.m_pAVCodecContext;
			auto& pAVFrame			= pOUT_FFmpegState.m_pAVFrame;
			auto& pSwsContext		= pOUT_FFmpegState.m_pSwsContext;
			auto& iFrameCounter		= pOUT_FFmpegState.m_iFrameCounter;
			
			// From RGBA to YUV
			int inLinesize[1] = { 4 * pAVCodecContext->width };
			sws_scale(	pSwsContext,
						&pRawFrameData,
						inLinesize,
						0,
						pAVCodecContext->height,
						pAVFrame->data,
						pAVFrame->linesize);

			pAVFrame->pts = (1.0 / 30.0) * 90000 * (iFrameCounter++);

			if (avcodec_send_frame(pAVCodecContext, pAVFrame) < 0)
			{
				setState(EFFmpegState::FFmpeg_Exit);
			}

			AV_TIME_BASE;
			AVPacket pAVPacketOut;
			{
				av_init_packet(&pAVPacketOut);
				pAVPacketOut.data = NULL;
				pAVPacketOut.size = 0;
				pAVPacketOut.flags |= AV_PKT_FLAG_KEY;

				if (NOT avcodec_receive_packet(pAVCodecContext, &pAVPacketOut))
				{
					// Send AVPAcket->data
					sDataHeader.m_iDataSize = pAVPacketOut.size;
					pNetServer.netSendAll(	(uint8_t*)&sDataHeader,
											sizeof(SDataHeader));

					pNetServer.netSendAll(	pAVPacketOut.data,
											sDataHeader.m_iDataSize);

					av_packet_unref(&pAVPacketOut);
				}
			}
		}

		void streamFromNetwork(	FFmpegNetworkState& pFFmpegNetworkState,
								FFmpegState& pFFmpegState)
		{
			BufferData pBufferData;
			NetBase& netBase = getNetworkState();

			// Receive 'AVPacket'.
			{
				SMagicHeader mHeader;
				netBase.netRecvAll(&mHeader, sizeof(SMagicHeader));

				if (std::string(mHeader.MAGIC) == "DROID")
				{
					netBase.netRecvAll(&pFFmpegNetworkState.m_AndroidHeader, sizeof(SDroidHeader));
					processDroidHeader(pFFmpegNetworkState);
				}
				else
				if (std::string(mHeader.MAGIC) == "DATA")
				{
					SDataHeader& dataHeader = pFFmpegNetworkState.m_DataHeader;
					netBase.netRecvAll(&dataHeader, sizeof(SDataHeader));

					pBufferData.setSize(dataHeader.m_iDataSize);
					netBase.netRecvAll(pBufferData.m_pBuffer, dataHeader.m_iDataSize);

					processNetworkPacket(pFFmpegState, pBufferData);
				}
			}
		}

		void processDroidHeader(FFmpegNetworkState& pFFmpegNetworkState)
		{
			resizeWindow(pFFmpegNetworkState.m_AndroidHeader.m_iWidth * DEST_SCALE, pFFmpegNetworkState.m_AndroidHeader.m_iHeight * DEST_SCALE);
		}

		void processNetworkPacket(	FFmpegState& pFFmpegState, 
									BufferData& pBufferData)
		{
			// Start 'AVPacket' decoding.
			{
				AVPacket pAVPacket;
				int32_t iErrorCode = createAVPacket(pBufferData, &pAVPacket);
				if(iErrorCode >= 0)
				{
					m_PacketQueue.PacketQueuePut(&pAVPacket);
				}
			}
		}

		int32_t createAVPacket(BufferData& pBufferData, AVPacket* pAVPacket)
		{
			int32_t iErrorCode = -1;

			// Create a new 'AVPacket' for our use.
			iErrorCode = av_new_packet(pAVPacket, pBufferData.m_iBufferSize);
			if (iErrorCode >= 0)
			{
				// Copy the received 'AVPacket' contents.
				memcpy(pAVPacket->data, pBufferData.m_pBuffer, pBufferData.m_iBufferSize);
			}

			return iErrorCode;
		}

		void decodeVideoPacket(FFmpegState& pFFmpegState, AVPacket* pAVPacket)
		{
			auto& pAVCodecContext		= pFFmpegState.m_pAVCodecContext;
			auto& pAVFrame				= pFFmpegState.m_pAVFrame;
			auto& pAVCodecParserContext	= pFFmpegState.m_pAVCodecParserContext;
			auto& pRawFrameData			= pFFmpegState.m_pRawFrameData;
			auto& pSwsContext			= pFFmpegState.m_pSwsContext;

			int32_t iErrorCode = -1;
			{
				//// Optional
				//// Check 'AVPacket' data validity
				//{
				//	uint8_t* pDataIn = pAVPacket->data;
				//	int iSizeIn = pAVPacket->size;
				//	uint8_t* pDataOut = NULL;
				//	int iSizeOut = 0;
				//
				//	iErrorCode = av_parser_parse2(	pAVCodecParserContext,
				//									pAVCodecContext,
				//									&pDataOut, &iSizeOut,
				//									pDataIn, iSizeIn,
				//									AV_NOPTS_VALUE, AV_NOPTS_VALUE, -1);
				//	assert(iErrorCode == iSizeIn);
				//	(void)iErrorCode;
				//	assert(iSizeOut == iSizeIn);
				//}

				if (pAVCodecParserContext->key_frame == 1)
				{
					pAVPacket->flags |= AV_PKT_FLAG_KEY;
				}

				// Send 'AVPacket' for decoding.
				iErrorCode = avcodec_send_packet(pAVCodecContext, pAVPacket);
				while (iErrorCode >= 0)
				{
					// Extract the 'AVFrame' from the 'AVPacket'.
					iErrorCode = avcodec_receive_frame(pAVCodecContext, pAVFrame);
					if (iErrorCode >= 0)
					{
						// Construct a 'Raw' pixel frame & save it in our 'm_pRawFrameData'.
						uint8_t* pDest[4] = { pRawFrameData, NULL, NULL, NULL };
						int iDestLineSize[4] = { pFFmpegState.m_iDstWidth * 4, 0, 0, 0 };

						sws_scale(pSwsContext, pAVFrame->data, pAVFrame->linesize, 0, pFFmpegState.m_iSrcHeight, pDest, iDestLineSize);
					}
					//else
					//if (iErrorCode < 0 && iErrorCode == AVERROR(EAGAIN)/* && iErrorCode != AVERROR_EOF*/)
					//{
					//	BufferData errBuff;
					//	errBuff.setSize(256);
					//	av_strerror(iErrorCode, (char*)errBuff.m_pBuffer, 256);
					//	
					//	LOG_CONSOLE("Error extracting AVFrame, iErrorCode " << (char*)errBuff.m_pBuffer);
					//}
				}
			}

			// DO NOT forget to dereference the 'AVPacket'.
			av_packet_unref(pAVPacket);
		}

		void streamToDisk(FFmpegState& pIN_FFmpegState, FFmpegState& p_OUTFFmpegState)
		{
			auto& pAVFormatContext	= p_OUTFFmpegState.m_pAVFormatContext;
			auto& pAVCodecContext	= p_OUTFFmpegState.m_pAVCodecContext;
			auto& pAVOutputFormat	= p_OUTFFmpegState.m_pAVOutputFormat;
			auto& pAVFrame			= p_OUTFFmpegState.m_pAVFrame;
			auto& pSwsContext		= p_OUTFFmpegState.m_pSwsContext;
			auto& iFrameCounter		= p_OUTFFmpegState.m_iFrameCounter;

			// From RGBA to YUV
			int inLinesize[1] = { 4 * pAVCodecContext->width };
			sws_scale(	pSwsContext,
						&pIN_FFmpegState.m_pRawFrameData,
						inLinesize,
						0,
						pAVCodecContext->height,
						pAVFrame->data,
						pAVFrame->linesize);

			pAVFrame->pts = (1.0 / 30.0) * 90000 * (iFrameCounter++);

			if (avcodec_send_frame(pAVCodecContext, pAVFrame) < 0)
			{
				setState(EFFmpegState::FFmpeg_Exit);
			}

			AV_TIME_BASE;
			AVPacket packetOut;
			{
				av_init_packet(&packetOut);
				packetOut.data = NULL;
				packetOut.size = 0;
				packetOut.flags |= AV_PKT_FLAG_KEY;
			}

			if (NOT avcodec_receive_packet(pAVCodecContext, &packetOut))
			{
				static int counter = 0;
				//if (counter == 0) {
				//	FILE *fp = fopen("dump_first_frame1.dat", "wb");
				//	fwrite(packetOut.data, packetOut.size, 1, fp);
				//	fclose(fp);
				//}

				//LOG_CONSOLE("pkt key: " << (packetOut.flags & AV_PKT_FLAG_KEY) << " " << packetOut.size << " " << (counter++));
				uint8_t* size = ((uint8_t*)packetOut.data);
				//LOG_CONSOLE("first: " << (int)size[0] << " " << (int)size[1] << " " << (int)size[2] << " " << (int)size[3] << " " << (int)size[4] << " " << (int)size[5] << " " << (int)size[6] << " " << (int)size[7]);

				av_interleaved_write_frame(pAVFormatContext, &packetOut);
				av_packet_unref(&packetOut);
			}
		}

		void flushToFile(FFmpegState& pFFmpegState)
		{
			auto& pAVFormatContext	= pFFmpegState.m_pAVFormatContext;
			auto& pAVCodecContext	= pFFmpegState.m_pAVCodecContext;
			auto& pAVOutputFormat	= pFFmpegState.m_pAVOutputFormat;

			//DELAYED FRAMES
			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.data = NULL;
			pkt.size = 0;

			for (;;) 
			{
				avcodec_send_frame(pAVCodecContext, NULL);
				if (avcodec_receive_packet(pAVCodecContext, &pkt) == 0)
				{
					av_interleaved_write_frame(pAVFormatContext, &pkt);
					av_packet_unref(&pkt);
				}
				else {
					break;
				}
			}

			av_write_trailer(pAVFormatContext);
			if (!(pAVOutputFormat->flags & AVFMT_NOFILE))
			{
				int err = avio_close(pAVFormatContext->pb);
				if (err < 0) 
				{
					LOG_CONSOLE("Failed to close file" << err);
				}
			}
		}

		void setState(EFFmpegState eState)
		{
			m_ePrevState = m_eState;
			m_eState = eState;
		}

		void setStreamState(EStreamState eState)
		{
			m_eStreamState = eState;
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
									pSwsContext = sws_getContext(	iSourceWidth,			// Source Width of the Video Frame
																	iSourceHeight,			// Source Height of the Video Frame
																	IN_AVPixelFormat,		// Pixel Format of the Video Frame
																	iDestWidth,				// Destination Width of the Video Frame
																	iDestHeight,			// Destination Height of the Video Frame
																	OUT_AVPixelFormat,		// Destination Image format
																	SWS_BILINEAR,			// Specify the Algorithm use to do the rescaling
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

		bool initStreamToDisk(	FFmpegState& pFFmpegState, 
								const char* sFileName,
								int32_t iSrcW,
								int32_t iSrcH,
								int32_t iDestW,
								int32_t iDestH,
								int32_t iFramerate,
								AVPixelFormat IN_AVPixelFormat,
								AVPixelFormat OUT_AVPixelFormat)
		{
			// Making final width & height disible by '2'.
			iSrcW = ceil(iSrcW / 2) * 2;
			iSrcH = ceil(iSrcH / 2) * 2;
			iDestW = ceil(iDestW / 2) * 2;
			iDestH = ceil(iDestH / 2) * 2;

			bool bInitSuccessful = false;

			auto& pAVFormatContext	= pFFmpegState.m_pAVFormatContext;
			auto& pAVCodecContext	= pFFmpegState.m_pAVCodecContext;
			auto& pAVOutputFormat	= pFFmpegState.m_pAVOutputFormat;
			auto& pAVStream			= pFFmpegState.m_pAVStream;
			auto& pAVFrame			= pFFmpegState.m_pAVFrame;
			auto& pSwsContext		= pFFmpegState.m_pSwsContext;
			auto& iFrameCounter		= pFFmpegState.m_iFrameCounter;

			iFrameCounter = 0;
			pAVOutputFormat = av_guess_format(nullptr, sFileName, nullptr);
			if (pAVOutputFormat != nullptr)
			{
				if (avformat_alloc_output_context2(	&pAVFormatContext,
													pAVOutputFormat, 
													nullptr, 
													sFileName) == 0)
				{
					AVCodec* pAVCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
					if (pAVCodec != nullptr)
					{
						pAVStream = avformat_new_stream(pAVFormatContext, pAVCodec);
						if (pAVStream != nullptr)
						{
							pAVStream->codecpar->codec_id = AV_CODEC_ID_H264;
							pAVStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
							pAVStream->codecpar->width = iDestW;
							pAVStream->codecpar->height = iDestH;
							pAVStream->codecpar->format = OUT_AVPixelFormat;
							//pAVStream->codecpar->bit_rate = 2000 * 1000;

							pAVCodecContext = avcodec_alloc_context3(pAVCodec);
							if (pAVCodecContext != nullptr)
							{
								avcodec_parameters_to_context(pAVCodecContext, pAVStream->codecpar);
								pAVCodecContext->time_base.num = 1;
								pAVCodecContext->time_base.den = 1;
								pAVCodecContext->max_b_frames = 2;
								//pAVCodecContext->gop_size = 12;
								pAVCodecContext->framerate.num = iFramerate;
								pAVCodecContext->framerate.den = 1;

								if (pAVStream->codecpar->codec_id == AV_CODEC_ID_H264) 
								{
									av_opt_set(pAVCodecContext, "preset", "ultrafast", 0);
								}
								else 
								if (pAVStream->codecpar->codec_id == AV_CODEC_ID_H265)
								{
									av_opt_set(pAVCodecContext, "preset", "ultrafast", 0);
								}

								avcodec_parameters_from_context(pAVStream->codecpar, pAVCodecContext);
								if (avcodec_open2(pAVCodecContext, pAVCodec, NULL) == 0)
								{
									if (NOT (pAVOutputFormat->flags & AVFMT_NOFILE))
									{
										if (avio_open(&pAVFormatContext->pb, sFileName, AVIO_FLAG_WRITE) >= 0)
										{
											if (avformat_write_header(pAVFormatContext, NULL) >= 0)
											{
												av_dump_format(pAVFormatContext, 0, sFileName, DUMP_TYPE_OUTPUT);

												pAVFrame = av_frame_alloc();
												pAVFrame->format = AV_PIX_FMT_YUV420P;
												pAVFrame->width = pAVCodecContext->width;
												pAVFrame->height = pAVCodecContext->height;
												if (av_frame_get_buffer(pAVFrame, 32) == 0)
												{
													pSwsContext = sws_getContext(	iSrcW,
																					iSrcH,
																					IN_AVPixelFormat, 
																					iDestW,
																					iDestH,
																					OUT_AVPixelFormat, 
																					SWS_BICUBIC, 
																					0, 0, 0);

													bInitSuccessful = true;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			return bInitSuccessful;
		}

		bool initPCServer(	FFmpegNetworkState& pFFmpegNetworkState,
							FFmpegState& pFFmpegState, 
							int64_t iNetworkAddress, 
							int32_t iNetworkPort,
							bool bHasExtraData = false)
		{
			bool bInitSuccessful = false;

			auto& pNetServer		= pFFmpegNetworkState.m_NetServer;
			auto& iIPAddress		= pNetServer.m_iIPAddress;
			auto& iPort				= pNetServer.m_iPort;
			auto& sAndroidHeader	= pFFmpegNetworkState.m_AndroidHeader;
			
			iIPAddress = iNetworkAddress;
			iPort = iNetworkPort;

			if (pNetServer.netInitialize())
			{
				if (pNetServer.netListen(iIPAddress, iPort, 1))
				{
					bInitSuccessful = true;
				}
			}

			return bInitSuccessful;
		}

		bool initPCClient(	FFmpegNetworkState& pFFmpegNetworkState,
							FFmpegState& pFFmpegState, 
							int64_t iNetworkAddress, 
							int32_t iNetworkPort,
							bool bHasExtraData = false)
		{
			bool bInitSuccessful = false;

			auto& pNetClient		= pFFmpegNetworkState.m_NetClient;
			auto& iIPAddress		= pNetClient.m_iIPAddress;
			auto& iPort				= pNetClient.m_iPort;
			auto& sAndroidHeader	= pFFmpegNetworkState.m_AndroidHeader;
			
			iIPAddress = iNetworkAddress;
			iPort = iNetworkPort;

			if (pNetClient.netInitialize())
			{
				if (pNetClient.netConnect(iIPAddress, iPort))
				{
					// Receive 'Handshake' header.
					{
						pNetClient.netRecvAll(&sAndroidHeader, sizeof(SDroidHeader));
						bInitSuccessful = true;
					}
				}
			}

			return bInitSuccessful;
		}

		wchar_t* utf8_to_wide_char(const char *utf8) 
		{
			int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
			if (NOT len) 
			{
				return NULL;
			}

			wchar_t* wide = (wchar_t*)std::malloc(len * sizeof(wchar_t));
			if (NOT wide) 
			{
				return NULL;
			}

			MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, len);

			return wide;
		}

		void convert(std::string& sStr, std::wstring& sWStr)
		{
			std::wstring wsTemp(sStr.begin(), sStr.end());
			sWStr = wsTemp;
		}

		bool createProcess(HANDLE& OUT hProcHandle, const char* sCmdToExecute)
		{
			STARTUPINFOW sProcStartupInfo;
			PROCESS_INFORMATION sProcInfo;
			{
				memset(&sProcStartupInfo, 0, sizeof(sProcStartupInfo));
				sProcStartupInfo.cb = sizeof(sProcStartupInfo);
			}

			int32_t iFlags = 0;

			std::wstring pLPWStr_sCmdToExecute;
			convert(std::string(sCmdToExecute), pLPWStr_sCmdToExecute);

			bool bSuccessful = CreateProcessW(	NULL, 
												(wchar_t*)pLPWStr_sCmdToExecute.c_str(), 
												NULL, NULL, FALSE, iFlags, NULL, NULL, 
												&sProcStartupInfo,
												&sProcInfo);
			if (bSuccessful && hProcHandle != nullptr)
			{
				hProcHandle = sProcInfo.hProcess;
			}

			return bSuccessful;
		}

		template <typename T>
		std::string getString(T lastParam)
		{
			return lastParam;
		}

		template <typename First, typename... Rest>
		std::string getString(First firstParam, Rest... restParams)
		{
			std::string sString = firstParam;
			sString.append( getString(restParams...) );

			return sString;
		}

		HANDLE adbExecute(std::string sAdbParams)
		{
			HANDLE hProc;
			createProcess(hProc, sAdbParams.c_str());

			return hProc;
		}

		void adbClearAllSockets()
		{
			std::stringstream sAdbCommand;

			sAdbCommand << RESOURCES << "adb.exe forward --remove-all";
			adbExecute(sAdbCommand.str());
			Sleep(200);

			sAdbCommand.str("");

			sAdbCommand << RESOURCES << "adb.exe reverse --remove-all";
			adbExecute(sAdbCommand.str());
			Sleep(200);
		}

		bool adbForward(int32_t iPort, const char* sDeviceSocketName)
		{
			adbClearAllSockets();

			std::stringstream sAdbForward;
			sAdbForward << RESOURCES
						<< "adb.exe forward"
						<< " "
						<< "tcp:"
						<< iPort
						<< " "
						<< "localabstract:"
						<< sDeviceSocketName;

			return adbExecute(sAdbForward.str());
		}

		bool adbReverse(int32_t iPort, const char* sDeviceSocketName)
		{
			adbClearAllSockets();

			std::stringstream sAdbReverse;
			sAdbReverse << RESOURCES
						<< "adb.exe reverse"
						<< " "
						<< "localabstract:"
						<< sDeviceSocketName
						<< " "
						<< "tcp:"
						<< iPort;

			return adbExecute(sAdbReverse.str());
		}

		HANDLE adbExecuteServer(const char* pServerName, const char* pDeviceDestPath, const char* pFullPackageName, std::string sIPAddress, int32_t iPort)
		{
			std::stringstream sAdbExecute;
			sAdbExecute << RESOURCES
						<< "adb.exe shell CLASSPATH="
						<< pDeviceDestPath
						<< pServerName
						<< " "
						<< "app_process"
						<< " "
						<< "/"
						<< " "
						<< pFullPackageName
						<< " "
						<< sIPAddress
						<< " "
						<< iPort;

			return adbExecute(sAdbExecute.str());
		}

		bool adbRemoveBinary(const char* pServerName, const char* pDeviceDestPath)
		{
			std::stringstream sAdbRemoveBinary;
			sAdbRemoveBinary	<< RESOURCES
								<< "adb.exe shell rm"
								<< " "
								<< pDeviceDestPath
								<< pServerName;

			return adbExecute(sAdbRemoveBinary.str());
		}

		bool adbPushBinary(const char* pServerName, const char* pDeviceDestPath)
		{
			std::stringstream sAdbPushBinary;
			sAdbPushBinary	<< RESOURCES
							<< "adb.exe push"
							<< " "
							<< RESOURCES
							<< pServerName
							<< " "
							<< pDeviceDestPath;

			return adbExecute(sAdbPushBinary.str());
		}

		void handleState(EFFmpegState eState)
		{
			switch (eState)
			{
				case Droid_Splash:
				{
					setState(EFFmpegState::Android_PushClient);
				}
				break;
				case Android_StartTCPIP:
				{
					std::stringstream sAdbExecute;
					sAdbExecute << RESOURCES
								<< "adb.exe tcpip 5555";

					if (adbExecute(sAdbExecute.str()))
					{
						setState(EFFmpegState::Android_Connect);
					}
				}
				break;
				case Android_Connect:
				{
					std::stringstream sAdbExecute;
					sAdbExecute << RESOURCES
								<< "adb.exe connect 192.168.0.101:5555";

					if (adbExecute(sAdbExecute.str()))
					{
						setState(EFFmpegState::Android_PushClient);
					}
				}
				break;
				case Android_PushServer:
				{
					bool bPushSuccessful = adbPushBinary(ANDROID_SERVER_BINARY, DEVICE_SERVER_PATH);
					if (bPushSuccessful)
					{
						adbForward(PORT, DEVICE_SOCKET_NAME);
						m_HProc = adbExecuteServer(ANDROID_SERVER_BINARY, DEVICE_SERVER_PATH, ANDROID_SERVER_PACKAGE_NAME, IPV4_IPADDRESS_S, PORT);

						handleState(EFFmpegState::FFmpeg_PCClient_Init);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_PCClient_Init:
				{
					Sleep(SERVER_CONNECT_DELAY);
					bool bInitSuccessful = initPCClient(	m_pFFmpegNetworkState,
															m_pFFmpegState_StreamToDesktop,
															LOCALHOST,
															PORT, 
															false);
					if (bInitSuccessful)
					{
						setState(EFFmpegState::FFmpeg_AndroidStreamToDesktop_Init);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case Android_PushClient:
				{
					bool bPushSuccessful = adbPushBinary(ANDROID_CLIENT_BINARY, DEVICE_SERVER_PATH);
					if (bPushSuccessful)
					{
						adbReverse(PORT, DEVICE_SOCKET_NAME);
						m_HProc = adbExecuteServer(ANDROID_CLIENT_BINARY, DEVICE_SERVER_PATH, ANDROID_CLIENT_PACKAGE_NAME, IPV4_IPADDRESS_S, PORT);

						setState(EFFmpegState::FFmpeg_PCServer_Init);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_PCServer_Init:
				{
					ULONG ulIPAddress = 0;
					std::string sIPAddress = NetBase::getDynamicIPAddress(ulIPAddress);

					bool bInitSuccessful = initPCServer(	m_pFFmpegNetworkState,
															m_pFFmpegState_StreamToDesktop,
															IPV4_IPADDRESS_UL,
															PORT, 
															false);
					if (bInitSuccessful)
					{
						setState(EFFmpegState::FFmpeg_StreamFromNetwork_Continous);
						setStreamState(EStreamState::NetworkStream);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_AndroidStreamToDesktop_Init:
				{
					bool bInitSuccessful = initStreamToDesktop(	m_pFFmpegState_StreamToDesktop,
																m_pFFmpegNetworkState.m_AndroidHeader.m_iWidth,
																m_pFFmpegNetworkState.m_AndroidHeader.m_iHeight,
																m_pFFmpegNetworkState.m_AndroidHeader.m_iWidth * DEST_SCALE,
																m_pFFmpegNetworkState.m_AndroidHeader.m_iHeight * DEST_SCALE,
																false,
																0,
																nullptr,
																AV_PIX_FMT_YUV420P,
																AV_PIX_FMT_RGBA);

					if (bInitSuccessful)
					{
						setState(EFFmpegState::FFmpeg_StreamToDisk_Init);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_StreamToDisk_Init:
				{
					bool bInitSuccessful = false;
					{
						setStreamState(EStreamState::Invalid);

						static int32_t iFileCount = 0;
						std::stringstream sInputVideoFile;
						{
							WWidgetManager* pWWidgetManager = WWidgetManager::getInstance();
							{
								std::string sFileName = "", sDirectory = "";
								H_WND hTxt_FileName = pWWidgetManager->GetWindowQ(ID_TEXTFIELD_FILENAME);
								{
									SendMessageQ(hTxt_FileName, WM__GETTEXT, (WPARAM)&sFileName, NULL);
								}
						
								H_WND hTxt_Directory = pWWidgetManager->GetWindowQ(ID_TEXTFILED_DIRECTORY);
								{
									SendMessageQ(hTxt_Directory, WM__GETTEXT, (WPARAM)&sDirectory, NULL);
								}
						
								sInputVideoFile << sDirectory << "\\" << sFileName << m_iRecordFileCount << ".mp4";
							}
						}

						bInitSuccessful = initStreamToDisk(	m_pFFmpegState_StreamToDisk,
															sInputVideoFile.str().c_str(),
															m_pFFmpegNetworkState.m_AndroidHeader.m_iWidth * DEST_SCALE,
															m_pFFmpegNetworkState.m_AndroidHeader.m_iHeight * DEST_SCALE,
															m_pFFmpegNetworkState.m_AndroidHeader.m_iWidth * DEST_SCALE,
															m_pFFmpegNetworkState.m_AndroidHeader.m_iHeight * DEST_SCALE,
															FRAMERATE,
															AV_PIX_FMT_RGBA,
															AV_PIX_FMT_YUV420P);
					}

					if (bInitSuccessful)
					{
						sendHandshake();

						setState(EFFmpegState::FFmpeg_StreamFromNetwork_Continous);
						setStreamState(EStreamState::NetworkStream);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_StreamFromNetwork_Continous:
				{
					if (m_pNetworkThread == nullptr)
					{
						m_fNetworkThreadFunc = [&]()
						{
							while (NOT m_bExitNetworkThread)
							{
								switch (m_eStreamState)
								{
									case EStreamState::NetworkStream:
									{
										streamFromNetwork(m_pFFmpegNetworkState, m_pFFmpegState_StreamToDesktop);
									}
									break;
									case EStreamState::NetworkStream_Record:
									{
										streamFromNetwork(m_pFFmpegNetworkState, m_pFFmpegState_StreamToDesktop);
										streamToDisk(m_pFFmpegState_StreamToDesktop, m_pFFmpegState_StreamToDisk);
									}
									break;									
									case EStreamState::FlushToDisk:
									case EStreamState::FlushToDiskAndExit:
									{
										flushToFile(m_pFFmpegState_StreamToDisk);

										if (m_eStreamState == EStreamState::FlushToDiskAndExit)
										{
											m_bExitNetworkThread = true;
										}
										else
										{
											m_iRecordFileCount++;

											setState(FFmpeg_StreamToDisk_Init);
											{
												H_WND hBtn_Record = WWidgetManager::getInstance()->GetWindowQ(ID_BUTTON_RECORD);
												SendMessageQ(hBtn_Record, BM__ENABLE, NULL, NULL);
												H_WND hBtn_Stop = WWidgetManager::getInstance()->GetWindowQ(ID_BUTTON_STOP);
												SendMessageQ(hBtn_Stop, BM__DISABLE, NULL, NULL);
											}
										}

										setStreamState(FFmpegYAGuiDroidCapture::EStreamState::Invalid);
									}
									break;
								}
							}

							if (m_bExitNetworkThread)
							{
								setState(EFFmpegState::FFmpeg_Exit);
							}
						};

						m_pNetworkThread = std::make_unique<std::thread>(m_fNetworkThreadFunc);
						m_pNetworkThread->detach();
					}
				}
				break;
				case EFFmpegState::FFmpeg_Exit:
				{
					destroyWindow();
				}
				break;
			}
		}

		void onMouseCallback(int32_t iAction, int32_t iKey, double dXPos, double dYPos) 
		{
			if (iAction == GLFW_PRESS && iKey == GLFW_MOUSE_BUTTON_RIGHT)
			{
				m_bShowUI = NOT m_bShowUI;
				{
					H_WND hWnd_TabbedPane = WWidgetManager::getInstance()->GetWindowQ(ID_TABBEDPANE_OPTIONS);
					SendMessageQ(hWnd_TabbedPane, m_bShowUI ? WM__SHOW : WM__HIDE, NULL, NULL);
				}
			}

			if (m_bShowUI)
			{
				switch (iAction)
				{
					case GLFW_PRESS:
					{
						m_pWidgetManager->onMouseDown(iKey, dXPos, dYPos);
					}
					break;
					case GLFW_RELEASE:
					{
						m_pWidgetManager->onMouseUp(iKey, dXPos, dYPos);
					}
					break;
					case GLFW_REPEAT:
					{
						if(iKey == 0)	// Mouse Move
							m_pWidgetManager->onMouseHover(iKey, dXPos, dYPos);
						else
						if (iKey > 0)	// Left Mouse Move
							m_pWidgetManager->onMouseMove(iKey, dXPos, dYPos);
					}
					break;
				}
			}
			else
			{
				NetBase& netBase = getNetworkState();
				SWriteInfo pSWriteInfo;

				pSWriteInfo.MAGIC = MAGIC_MOUSE_EVENT;
				pSWriteInfo.m_iSize = sizeof(SPointerInfo);
				netBase.netSendAll(	(uint8_t*)&pSWriteInfo,
									sizeof(SWriteInfo));

				SPointerInfo pSPointerInfo;
				{
					pSPointerInfo.m_iAction = iAction;
					pSPointerInfo.m_iKey = iKey;
					pSPointerInfo.m_iPosX = dXPos * ONE_OVER_DEST_SCALE;
					pSPointerInfo.m_iPosY = dYPos * ONE_OVER_DEST_SCALE;
					netBase.netSendAll(	(uint8_t*)&pSPointerInfo,
										sizeof(SPointerInfo));
				}
			}
		}

		void onAndroidKeyboardCallback(int32_t iAction, int32_t iAndroidKeyCode, int32_t iAndroidMetaModifier)
		{
			if (NOT m_bShowUI)
			{
				NetBase& netBase = getNetworkState();
				SWriteInfo pSWriteInfo;
				
				pSWriteInfo.MAGIC = MAGIC_KEYBOARD_EVENT;
				pSWriteInfo.m_iSize = sizeof(SKeyInfo);
				netBase.netSendAll(	(uint8_t*)&pSWriteInfo,
									sizeof(SWriteInfo));
				
				SKeyInfo pSKeyInfo;
				{
					pSKeyInfo.m_iAction = iAction;
					pSKeyInfo.m_iAndroidKeyCode = iAndroidKeyCode;
					pSKeyInfo.m_iAndroidMetaModifier = iAndroidMetaModifier;
					netBase.netSendAll(	(uint8_t*)&pSKeyInfo,
										sizeof(pSKeyInfo));
				}
			}
		}

		void onKeyboardCallback(int32_t iAction, int32_t iKeyCode, int32_t iScanCode, int32_t iModifierKeys)
		{
			WWidgetManager* pWidgetManager = WWidgetManager::getInstance();
			bool CONTROL_ON = pWidgetManager->isModifierKeyDown(GLFW_MOD_CONTROL);
			bool SHIFT_ON = pWidgetManager->isModifierKeyDown(GLFW_MOD_SHIFT);

			if(m_bShowUI)
			{
				switch (iAction)
				{
					case GLFW_PRESS:
					{
						if (iKeyCode == GLFW_KEY_UP
							||
							iKeyCode == GLFW_KEY_DOWN
							||
							iKeyCode == GLFW_KEY_LEFT
							||
							iKeyCode == GLFW_KEY_RIGHT
							||
							iKeyCode == GLFW_KEY_HOME
							||
							iKeyCode == GLFW_KEY_END
							||
							iKeyCode == GLFW_KEY_BACKSPACE
							||
							iKeyCode == GLFW_KEY_DELETE
							||
							iKeyCode == GLFW_KEY_PAGE_UP
							||
							iKeyCode == GLFW_KEY_PAGE_DOWN
						) {
							m_pWidgetManager->keyPressed(iKeyCode, 0);
						}
						else
						if (CONTROL_ON)
						{
							if (iKeyCode == 'A' || iKeyCode == 'X' || iKeyCode == 'C' || iKeyCode == 'V')
							{
								m_pWidgetManager->keyPressed(iKeyCode, 0);
							}
						}
					}
					break;
					case GLFW_RELEASE:
					{
					}
					break;
				}
			}
			else
			{
				switch (iAction)
				{
					case GLFW_RELEASE:
					{
						if (CONTROL_ON)
						{
							// Take Screenshot
							if (iKeyCode == 'P' || iKeyCode == 'p')
							{
								H_WND hWnd_ScreenshotImageTypeCB = WWidgetManager::getInstance()->GetWindowQ(ID_CB_IMAGE_EXPORT_TYPE);
								STBImgType::Type eSTBImgType = (STBImgType::Type)SendMessageQ(hWnd_ScreenshotImageTypeCB, CBM__GETCURSEL, NULL, NULL);

								std::string sCurrTime = Timer::getCurrentTimeAndDate();
								std::replace(sCurrTime.begin(), sCurrTime.end(), ':', '-');

								std::string sFileExtn = STBImgType::ToString(eSTBImgType);

								std::stringstream sFileName;
								sFileName << "../res/Screenshot_" << sCurrTime << "." << sFileExtn;
								Texture::saveToDisk(sFileName.str(), eSTBImgType, m_iWidth, m_iHeight, Texture::STBImgFormat::RGBA_4, m_pFFmpegState_StreamToDesktop.m_pRawFrameData);
							}
						}
					}
					break;
				}
			}
		}

		void onUnicodeCharCallback(uint32_t iUnicodeChar)
		{
			if (m_bShowUI)
			{
				m_pWidgetManager->keyPressed(0, iUnicodeChar);
			}
		}

		void sendMagic(char ch)
		{
			NetBase& netBase = getNetworkState();
			SWriteInfo pSWriteInfo;
			{
				pSWriteInfo.MAGIC = ch;
				pSWriteInfo.m_iSize = 0;
				netBase.netSendAll(	(uint8_t*)&pSWriteInfo,
									sizeof(SWriteInfo));
			}
		}

		void sendHandshake()
		{
			// Handshake Response - Start Sending Visual Stream
			{
				NetBase& netBase = getNetworkState();
				SWriteInfo pSWriteInfo;
				pSWriteInfo.MAGIC = MAGIC_START_SENDING_FRAME;
				pSWriteInfo.m_iSize = 0;
				netBase.netSendAll(	(uint8_t*)&pSWriteInfo,
									sizeof(SWriteInfo));

				LOG_CONSOLE("Start Sending Frame Meta");
			}
		}

		NetBase& getNetworkState()
		{
			NetBase& netClient = m_pFFmpegNetworkState.m_NetClient;
			NetBase& netServer = m_pFFmpegNetworkState.m_NetServer;
			return (netClient.m_bIsConnected) ? netClient : netServer;
		}

		void initCallbacks()
		{
			m_fMouseFuncCallback = std::bind(	&FFmpegYAGuiDroidCapture::onMouseCallback,
												this, 
												std::placeholders::_1, 
												std::placeholders::_2, 
												std::placeholders::_3, 
												std::placeholders::_4);

			m_fKeyboardFuncCallback = std::bind(	&FFmpegYAGuiDroidCapture::onKeyboardCallback,
													this, 
													std::placeholders::_1, 
													std::placeholders::_2,
													std::placeholders::_3, 
													std::placeholders::_4);

			m_fAndroidKeyboardFuncCallback = std::bind(	&FFmpegYAGuiDroidCapture::onAndroidKeyboardCallback,
														this, 
														std::placeholders::_1, 
														std::placeholders::_2,
														std::placeholders::_3);

			m_fUnicodeCharFuncCallback = std::bind(	&FFmpegYAGuiDroidCapture::onUnicodeCharCallback,
													this,
													std::placeholders::_1);
			
			InputManager::get()->addMouseListener(&m_fMouseFuncCallback);
			InputManager::get()->addKeyboardListener(&m_fKeyboardFuncCallback);
			InputManager::get()->addUnicodeCharListener(&m_fUnicodeCharFuncCallback);
			InputManager::get()->addAndroidKeyboardListener(&m_fAndroidKeyboardFuncCallback);
		}

		void initYAGui()
		{
			m_pWidgetManager = WWidgetManager::getInstance();
			{
				m_fUIFuncCallback = std::bind(	&FFmpegYAGuiDroidCapture::onUICallback,
												this,
												std::placeholders::_1,
												std::placeholders::_2,
												std::placeholders::_3,
												std::placeholders::_4);

				WWidgetManager::setCallback(&m_fUIFuncCallback);
				m_pWidgetManager->init(DROID_SPLASH_WND_W, DROID_SPLASH_WND_H);
			}
		}

		void resizeYAGui(int32_t iNewWidth, int32_t iNewHeight)
		{
			WWidgetManager* pWWidgetManager = WWidgetManager::getInstance();
			{
				pWWidgetManager->resizeWindow(iNewWidth, iNewHeight);
				{
					H_WND hTabbedPane = pWWidgetManager->GetWindowQ(ID_TABBEDPANE_OPTIONS);
					if (hTabbedPane != NULL)
					{
						DWORD dSize = (((iNewHeight - DROID_UI_WND_GUTTER_2H) & 0xFFFF) << 16) | ((iNewWidth - DROID_UI_WND_GUTTER_2W) & 0xFFFF);
						SendMessageQ(	hTabbedPane,
										WM__SETSIZE,
										NULL,
										dSize);
					}
				}
			}
		}

		void updateRecordVisuals(uint64_t lElapsedTime)
		{
			int32_t iVal = (lElapsedTime / 500);
			m_bRenderRecordVisuals = ((iVal % 2) == 0);
		}

		void renderRecordVisuals()
		{
			WWidgetManager* pWWidgetManager = WWidgetManager::getInstance();

			int32_t iLineSize = 50, iStrokeSize = 10, iOffsetX = 20, iOffsetY = 20;
			int32_t iX = 0, iY = 0;
			float fAlpha = 1.0f * m_bRenderRecordVisuals;

			// Left-Top Edge
			iX = iOffsetX;
			iY = iOffsetY;
			pWWidgetManager->drawHorizontalLine(1.0, 0.0, 0.0, fAlpha, iX, iY, iLineSize, iStrokeSize);
			pWWidgetManager->drawVerticalLine(1.0, 0.0, 0.0, fAlpha, iX, iY, iLineSize, iStrokeSize);

			// Right-Top Edge
			iX = m_iWidth - iLineSize - iOffsetX;
			iY = iOffsetY;
			pWWidgetManager->drawHorizontalLine(1.0, 0.0, 0.0, fAlpha, iX, iY, iLineSize, iStrokeSize);
			iX = m_iWidth - iOffsetX - iStrokeSize;
			pWWidgetManager->drawVerticalLine(1.0, 0.0, 0.0, fAlpha, iX, iY, iLineSize, iStrokeSize);

			// Left-Bottom Edge
			iX = iOffsetX;
			iY = m_iHeight - iOffsetY - iStrokeSize;
			pWWidgetManager->drawHorizontalLine(1.0, 0.0, 0.0, fAlpha, iX, iY, iLineSize, iStrokeSize);
			iX = iOffsetX;
			iY = m_iHeight - iLineSize - iOffsetY;
			pWWidgetManager->drawVerticalLine(1.0, 0.0, 0.0, fAlpha, iX, iY, iLineSize, iStrokeSize);

			// Right-Bottom Edge
			iX = m_iWidth - iLineSize - iOffsetX;
			iY = m_iHeight - iOffsetY - iStrokeSize;
			pWWidgetManager->drawHorizontalLine(1.0, 0.0, 0.0, fAlpha, iX, iY, iLineSize, iStrokeSize);
			iX = m_iWidth - iOffsetX - iStrokeSize;
			iY = m_iHeight - iLineSize - iOffsetY;
			pWWidgetManager->drawVerticalLine(1.0, 0.0, 0.0, fAlpha, iX, iY, iLineSize, iStrokeSize);
		}

		L_RESULT CALLBACK onUICallback(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
				case WN__CREATE:
				{
					switch (wParam)
					{
						case ROOT_WINDOW_ID:
						{
							WComponent* pComp = (WComponent*)hWnd;
							pComp->m_RenderColor = Color(1.0, 1.0f, 1.0f, 0.0f);
							addUIElements(hWnd);
						}
						break;
						case ID_BUTTON_DIRECTORY_TREE:
						{
							WTree* pTree = (WTree*)hWnd;
							if (pTree != nullptr)
							{
								//TREEITEM* pRoot = pTree->getRoot();
								//{
								//	TREEITEM* it = new TREEITEM();
								//	it->m_bIsLeaf = true;
								//	it->setName("Leaf 0");
								//	pTree->addToParent(it);
								//}
							}
						}
						break;
					}
				}
				break;
				case WN__UPDATE:
				{
					if (wParam == -1)	// WWidgetManager
					{	
						if (m_eStreamState == EStreamState::NetworkStream_Record)
						{
							updateRecordVisuals(lParam);
						}
					}
				}
				break;
				case WN__PAINT:
				{
					// If it's a 'WWidgetManager'
					if (wParam == -1 && lParam == -1)
					{
						if (m_eStreamState == EStreamState::NetworkStream_Record)
						{
							renderRecordVisuals();
						}
					}
					else
					{
						switch (wParam)
						{
							case ID_CANVAS_QRCODE:
							{
								Rect canvasRect;
								int iRet = SendMessageQ(hWnd, WM__GETRECT, (WPARAM)NULL, (LPARAM)&canvasRect);

								FillRect(&canvasRect, 255, 255, 255, 255);
								//DrawRect(&boundsRect, 255, 0,	0,	 255, 3);

								std::string sQRText = "192.168.40.96:8888";
								std::stringstream sTitle;
								sTitle << sQRText.c_str();

								SetColorQ(255, 0, 0, 255);
								DrawString(sTitle.str().c_str(), canvasRect.X + (canvasRect.Width >> 1), canvasRect.Y + 5, 1);
								ResetColorQ();

								renderQRCode(sQRText, 5, canvasRect);
							}
							break;
						}
					}
				}
				break;
				case WM_BTN_LBUTTONUP:
				{
					switch (wParam)
					{
						case ID_BUTTON_RECORD:
						{
							m_eStreamState = EStreamState::NetworkStream_Record;
							{
								H_WND hBtn_Record = WWidgetManager::getInstance()->GetWindowQ(ID_BUTTON_RECORD);
								SendMessageQ(hBtn_Record, BM__DISABLE, NULL, NULL);
								H_WND hBtn_Stop = WWidgetManager::getInstance()->GetWindowQ(ID_BUTTON_STOP);
								SendMessageQ(hBtn_Stop, BM__ENABLE, NULL, NULL);
							
								m_bShowUI = false;
								{
									H_WND hWnd_Record = WWidgetManager::getInstance()->GetWindowQ(ID_TABBEDPANE_OPTIONS);
									SendMessageQ(hWnd_Record, WM__HIDE, NULL, NULL);
								}
							}
						}
						break;
						case ID_BUTTON_STOP:
						{
							m_eStreamState = EStreamState::FlushToDisk;
						}
						break;
						case ID_BUTTON_FREEZE_ROTATION:
						{
							sendMagic(MAGIC_FREEZE_ROTATION_EVENT);
						}
						break;
						case ID_BUTTON_THAW_ROTATION:
						{
							sendMagic(MAGIC_THAW_ROTATION_EVENT);
						}
						break;
						case ID_BUTTON_DIRECTORY_LISTING:
						{
							sendMagic(MAGIC_DIRECTORY_LISTING);
						}
						break;
					}
					
				}
				break;
			}

			return WM__OKAY;
		}

		void renderQRCode(std::string sQRCodeString, int32_t iModuleSize, Rect canvasRect)
		{
			QrCode qr0 = QrCode::encodeText(sQRCodeString.c_str(), QrCode::Ecc::MEDIUM);

			int32_t iQRCodeSize = qr0.getSize();
			int32_t iTotalQRWH = iQRCodeSize * iModuleSize;
			int32_t iHalfQRWH = iTotalQRWH >> 1;

			int32_t iStartX = canvasRect.X + (canvasRect.Width >> 1) - iHalfQRWH;
			int32_t iStartY = canvasRect.Y + (canvasRect.Height >> 1) - iHalfQRWH;
			Rect qrRect(iStartX, iStartY, iModuleSize, iModuleSize);
			
			for (int y = 0; y < qr0.getSize(); y++)
			{
				for (int x = 0; x < qr0.getSize(); x++)
				{
					bool bQRModule = qr0.getModule(x, y);

					if (bQRModule)
					{
						// BLACK
						FillRect(&qrRect, 0, 0, 0, 255);
					}
					else
					{
						// WHITE
						FillRect(&qrRect, 255, 255, 255, 255);
					}

					qrRect.X += iModuleSize;
				}

				qrRect.X = iStartX;
				qrRect.Y += iModuleSize;
			}
		}

		void addUIElements(H_WND hRootWnd)
		{
			H_WND hWnd_TabbedPane;
			{
				int32_t iXPos = 20;
				int32_t iYPos = 50;
				ADD_UI_TABBEDPANE(hRootWnd, hWnd_TabbedPane, "Options", WM_ANCHOR_TOPLEFT, iXPos, iYPos, m_iWidth - DROID_UI_WND_GUTTER_2W, m_iHeight - DROID_UI_WND_GUTTER_2H, ID_TABBEDPANE_OPTIONS);
				{
					addRecordTab(hWnd_TabbedPane);
				}
				{
					addQRCodeTab(hWnd_TabbedPane);
				}
				{
					addDirectoryTab(hWnd_TabbedPane);
				}
				SendMessageQ(hWnd_TabbedPane, WM__HIDE, NULL, NULL);
			}
		}

		void addRecordTab(H_WND hRoot_TabbedPane)
		{
			if (hRoot_TabbedPane != nullptr)
			{
				H_WND hWnd, hRecordTab;
				hRecordTab = ((WTabbedPane*)hRoot_TabbedPane)->addTab("Record");
				{
					int32_t iXStartPos = 20;
					int32_t iXPos = iXStartPos;
					int32_t iYPos = 50;
					int32_t iTextField_Width = 250;
					int32_t iRowHeight = DROID_UI_COMPONENT_H + 10;

					// Recording Options
					{
						iXPos = iXStartPos;
						ADD_UI_BUTTON(hRecordTab, hWnd, "Record", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXPos + 20, iYPos, DROID_UI_BUTTON_W, DROID_UI_COMPONENT_H, ID_BUTTON_RECORD, "Button")
						SendMessageQ(hWnd, BM__ENABLE, NULL, NULL);
					
						iXPos += DROID_UI_BUTTON_W;
						ADD_UI_BUTTON(hRecordTab, hWnd, "Stop", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXPos + 20, iYPos, DROID_UI_BUTTON_W, DROID_UI_COMPONENT_H, ID_BUTTON_STOP, "Button")
						SendMessageQ(hWnd, BM__DISABLE, NULL, NULL);

						iYPos += iRowHeight;
						ADD_UI_LABEL(hRecordTab, hWnd, "Filename :", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXStartPos, iYPos, 75, DROID_UI_COMPONENT_H, ID_LABEL_FILENAME)
						ADD_UI_TEXTFIELD(hRecordTab, hWnd, "Recorded", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXStartPos + 75, iYPos, iTextField_Width, DROID_UI_COMPONENT_H, ID_TEXTFIELD_FILENAME)
						
						iYPos += iRowHeight;
						ADD_UI_LABEL(hRecordTab, hWnd, "Directory :", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXStartPos, iYPos, 75, DROID_UI_COMPONENT_H, ID_LABEL_DIRECTORY)
						ADD_UI_TEXTFIELD(hRecordTab, hWnd, "..\\res", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXStartPos + 75, iYPos, iTextField_Width, DROID_UI_COMPONENT_H, ID_TEXTFILED_DIRECTORY)
					}

					// Rotation Options
					{
						iXPos = iXStartPos;
						iYPos += iRowHeight;
						ADD_UI_BUTTON(hRecordTab, hWnd, "Freeze Rot", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXPos + 20, iYPos, DROID_UI_BUTTON_W, DROID_UI_COMPONENT_H, ID_BUTTON_FREEZE_ROTATION, "Button")
						SendMessageQ(hWnd, BM__ENABLE, NULL, NULL);
						
						iXPos += DROID_UI_BUTTON_W;
						ADD_UI_BUTTON(hRecordTab, hWnd, "Thaw Rot", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXPos + 20, iYPos, DROID_UI_BUTTON_W, DROID_UI_COMPONENT_H, ID_BUTTON_THAW_ROTATION, "Button")
						SendMessageQ(hWnd, BM__ENABLE, NULL, NULL);

						iYPos += iRowHeight;
						iXPos = iXStartPos;
						ADD_UI_BUTTON(hRecordTab, hWnd, "Directory Listing", WM_ALIGN_WRT_PARENT_TOPCENTER | WM_ANCHOR_TOPCENTER | WM_ANCHOR_HSTRETCH, 0, iYPos, 90, DROID_UI_COMPONENT_H, ID_BUTTON_DIRECTORY_LISTING, "Button")
						SendMessageQ(hWnd, BM__ENABLE, NULL, NULL);
					}

					// Screenshot - Image export Options
					{
						iYPos += iRowHeight;
						ADD_UI_LABEL(hRecordTab, hWnd, "Screenshot Image Type(Press 'Ctrl-P/p' on main window):", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXStartPos, iYPos, 75, DROID_UI_COMPONENT_H, ID_LABEL_IMAGE_EXPORT_TYPE)
						iYPos += (iRowHeight >> 1);
						ADD_UI_COMBOBOX(hRecordTab, hWnd, "", WM_ALIGN_WRT_PARENT_TOPCENTER | WM_ANCHOR_TOPCENTER | WM_ANCHOR_HSTRETCH, 0, iYPos, 90, DROID_UI_COMPONENT_H * (int32_t)STBImgType::Type::MAX, ID_CB_IMAGE_EXPORT_TYPE)
						{
							for(int i = 0; i < (int32_t)STBImgType::Type::MAX; i++)
							{
								((WComboBox*)hWnd)->addItem(STBImgType::ToString((STBImgType::Type)i).c_str());
							}

							((WComboBox*)hWnd)->setSelectedIndex((int32_t)STBImgType::Type::BMP);
						}
					}
				}
			}
		}

		void addQRCodeTab(H_WND hRoot_TabbedPane)
		{
			if (hRoot_TabbedPane != nullptr)
			{
				H_WND hWnd, hQRCodeTab;
				hQRCodeTab = ((WTabbedPane*)hRoot_TabbedPane)->addTab("QR Code");
				{
					int32_t iXStartPos = 20;
					int32_t iXPos = iXStartPos;
					int32_t iYPos = 20;
					int32_t iRowHeight = DROID_UI_COMPONENT_H + 10;

					ADD_UI_LABEL(hQRCodeTab, hWnd, "QR Code:", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXStartPos, iYPos, 75, DROID_UI_COMPONENT_H, 0)
					iYPos += (iRowHeight >> 1);

					// QR Code - IP Address
					{
						ADD_UI_CANVAS(hQRCodeTab, hWnd, "WCanvas window", WM_ALIGN_WRT_PARENT_TOPCENTER | WM_ANCHOR_TOPCENTER, iXStartPos, iYPos, 250, 250, ID_CANVAS_QRCODE)
					}
				}

std::string sText = "New Delhi, July 26 (IANS) Hours after saying he did not wish to implicate Prime Minister Manmohan Singh or anyone else in the 2G spectrum allotment case, former telecom minister A. Raja Tuesday asked why the matter had not been referred to a ministerial panel and also wanted Home Minister P. Chidambaram to take the witness stand.\n\
		Main kisi ko phasana nahi chahta tha (I had no intention of framing anybody),' Raja's lawyer Sushil Kumar said on his behalf when the names of Manmohan Singh and Chidambaram cropped up in a special Central Bureau of Investigation (CBI) court.\n\
		I am just defending myself -- not accusing anything or anybody,' he said, a day after stroking a political storm by dragging the prime minister into the controversy. 'They (the media) cannot put words into my mouth. Ask them to report truthfully, or go out of this court,' he added.\n\
		But the home minister must come in the court from either of the sides and be a witness in the case. When all decisions were known to the home minister, he should appear as a witness in the case,' Kumar told the special court presided over by Judge O.P. Saini.\n\
		Just a few hours later, after recess, he stepped up his attack on the prime minister and wondered why a group of ministers (GoM) was not set up if any inconsistency was found on the way the spectrum allocation matter was handled.\n\
		A lawyer by traininRaja himself took over from his counsel at one point.\n\
		The prime minister is superior to me. He could have constituted a GoM. But he ignored a GoM. Is this a conspiracy?' Raja's counsel asked, wanting the then solicitor general and now attorney general Goolam. E. Vahanvati, too, in the witness box, while terming the judicial custody of his client since Feb 2 illegal.\n\
		The counsel was continuing with the arguments the previous day that as finance minister in 2008 Chidambaram had taken the decision to permit the promoters of two telecom firms to sell stakes with full knowledge of the prime minister.\n\
		While this was not denied subsequently by Chidambaram or present Communications Minister Kapil Sibal, both sought to say that the equity sale was by way of issuing fresh shares and not divestment by promoters, permitted under the policy that existed then.\n\
		The Congress even launched a counter-attack Tuesday and said Raja had also dragged former prime minister Atal Bihari Vajpayee's name in the case and that the government of the Bharatiya Janata Party (BJP)-led coalition at that time was equally culpable.\n\
		If the BJP decides to make a song and dance about one part of Raja's statement, then the other part of his statement squarely indicts Atal Bihari Vajpayee also,' Congress spokesperson Manish Tewari said.\n\
		The official probe agency has said that Raja's decision as telecom minister in 2008 to issue radio spectrum to companies at a mere Rs.1,659 crore ($350 million) for a pan-India operation had caused the exchequer losses worth thousands of crores of rupees.\n\
		Nine new telecom companies were issued the radio frequency airwaves, a scarce national resource, to operate second generation (2G) mobile phone services in the country. As many as 122 circle-wise licences were issued.\n\
		The probe agency questioned the manner in which allocations were made that even resulted in a windfall for some.\n\
		A new player Swan Telecom had bought licences for 13 circles with the necessary spectrum for $340 million but managed to sell a 45 percent stake in the company to UAE's Etisalat for $900 million. This swelled its valuation to $2 billion without a single subscriber.\n\
		Similarly, another new player, Unitech, paid $365 million as licence fee but sold a 60 percent stake to Norway's Telenor for $1.36 billion, taking its valuation to nearly $2 billion, again without a single subscriber.\n\
		\n\
		The MBR can only represent four partitions. A technique called \"extended\" partitioning is used to allow more than four, and often times it is used when there are more than two partitions. All we're going to say about extended partitions is that they appear in this table just like a normal partition, and their first sector has another partition table that describes the partitions within its space. But for the sake of simply getting some code to work, we're going to not worry about extended partitions (and repartition and reformat any drive that has them....) The most common scenario is only one partition using the whole drive, with partitions 2, 3 and 4 blank.";	
					
				// Dummy Window
				{
					//ADD_UI_DUMMY(hQRCodeTab, hWnd, sText.c_str(), WM_ALIGN_WRT_PARENT_TOPCENTER | WM_ANCHOR_TOPCENTER | WM_ANCHOR_HSTRETCH, 0, iYPos, -1, 250, ID_CANVAS_QRCODE + 1)
				}

				// Inspector
				{
					/*
					ADD_UI_INSPECTOR(hQRCodeTab, hWnd, "Title1", WM_ANCHOR_TOPLEFT, 10, 40, 350, 250, HMENU(1013), true)
					{
						((WInspector*)hWnd)->addTab();
						((WInspector*)hWnd)->addTab();
						((WInspector*)hWnd)->addTab();
					}
					*/
				}

				// Table
				{
					/*
					ADD_UI_TABLE(hQRCodeTab, hWnd, "Table", WM_ANCHOR_TOPLEFT, 50, 140, 450, 450, HMENU(1004), NULL)
					{
						((WTable*)hWnd)->addColumn("Name");
						((WTable*)hWnd)->addColumn("Date Modified");
						((WTable*)hWnd)->addColumn("Type");
						((WTable*)hWnd)->addColumn("Size");
						((WTable*)hWnd)->addColumn("Date Created");
						((WTable*)hWnd)->addColumn("Authors");
						((WTable*)hWnd)->addColumn("Tags");

						TableRowData* trd = NULL;
						TableCellData* tcd = NULL;
						for (int ii = 0; ii < 15; ii++) {
							trd = new TableRowData();
							{
								tcd = new TableCellData("1.txt");	trd->addCellData(tcd);
								tcd = new TableCellData("10 June 2012");	trd->addCellData(tcd);
								tcd = new TableCellData("Type 0");	trd->addCellData(tcd);
								tcd = new TableCellData("12345");	trd->addCellData(tcd);
								tcd = new TableCellData("10 June 2012");	trd->addCellData(tcd);
								tcd = new TableCellData("Aalekh Maldikar");	trd->addCellData(tcd);
								tcd = new TableCellData("RW");	trd->addCellData(tcd);
							}
							((WTable*)hWnd)->addRow(trd);
						}

						for (int ii = 0; ii < 15; ii++) {
							trd = new TableRowData();
							{
								tcd = new TableCellData("2.txt");	trd->addCellData(tcd);
								tcd = new TableCellData("12 June 2012");	trd->addCellData(tcd);
								tcd = new TableCellData("Type 1");	trd->addCellData(tcd);
								tcd = new TableCellData("54321");	trd->addCellData(tcd);
								tcd = new TableCellData("12 June 2012");	trd->addCellData(tcd);
								tcd = new TableCellData("Rashmi Maldikar");	trd->addCellData(tcd);
								tcd = new TableCellData("RW");	trd->addCellData(tcd);
							}
							((WTable*)hWnd)->addRow(trd);
						}
					}
					*/
				}

				// TextBox
				{
					//ADD_UI_TEXTBOX(hQRCodeTab, hWnd, sText.c_str(), WM_ANCHOR_TOPLEFT, 10, 10, 350, 350, HMENU(197), NULL)
				}

				// Window
				{
					ADD_UI_WINDOW(hQRCodeTab, hWnd, sText.c_str(), WM_ANCHOR_TOPLEFT, 10, 10, 350, 350, HMENU(197), (LPVOID)ID_TYPE_WND_CSMX)
				}
			}
		}

		void addDirectoryTab(H_WND hRoot_TabbedPane)
		{
			if (hRoot_TabbedPane != nullptr)
			{
				H_WND hWnd, hDirectoryTab;
				hDirectoryTab = ((WTabbedPane*)hRoot_TabbedPane)->addTab("Directory");
				{
					int32_t iXStartPos = 20;
					int32_t iXPos = iXStartPos;
					int32_t iYPos = 20;
					int32_t iRowHeight = DROID_UI_COMPONENT_H + 10;
					int32_t iTextField_Width = 250;
					int32_t iTabWidth = 0, iTabHeight = 0;

					DWORD dwDirectoryFramSize = 0;
					SendMessageQ(hRoot_TabbedPane, WM__GETSIZE, (WPARAM)0, (LPARAM)&dwDirectoryFramSize);
					{
						iTabWidth = (dwDirectoryFramSize & 0xffff);
						iTabHeight = ((dwDirectoryFramSize >> 16) & 0xffff);
					}

					std::string sPathLabel = "Path:";
					ADD_UI_LABEL(hDirectoryTab, hWnd, sPathLabel.c_str(), WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXStartPos, iYPos, 75, DROID_UI_COMPONENT_H, 0)
					ADD_UI_TEXTFIELD(hDirectoryTab, hWnd, "/sdcard", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_CENTERLEFT, iXStartPos + WWidgetManager::getStringWidthTillPos(sPathLabel.c_str(), strlen(sPathLabel.c_str())) + 10, iYPos, iTextField_Width, DROID_UI_COMPONENT_H, ID_TEXTFILED_DIRECTORY)

					iYPos += iRowHeight;
					ADD_UI_TREE(hDirectoryTab, hWnd, "Directory", WM_ALIGN_WRT_PARENT_TOPCENTER | WM_ANCHOR_TOPCENTER | WM_ANCHOR_HSTRETCH | WM_ANCHOR_VSTRETCH, 0, iYPos, 90, 70, ID_BUTTON_DIRECTORY_TREE)
				}
			}
		}
	protected:
	private:
		FFmpegNetworkState						m_pFFmpegNetworkState;

		FFmpegState								m_pFFmpegState_StreamToDesktop;
		FFmpegState								m_pFFmpegState_StreamToDisk;

		EFFmpegState							m_ePrevState;
		EFFmpegState							m_eState;
		EStreamState							m_eStreamState;

		std::string								m_sInputVideoFile;

		MOUSE_FUNC_CALLBACK						m_fMouseFuncCallback;
		KEY_FUNC_CALLBACK						m_fKeyboardFuncCallback;
		ANDROID_KEY_FUNC_CALLBACK				m_fAndroidKeyboardFuncCallback;
		UNICODE_CHAR_FUNC_CALLBACK				m_fUnicodeCharFuncCallback;

		std::unique_ptr<std::thread>			m_pNetworkThread;
		std::function<void(void)>				m_fNetworkThreadFunc;

		int32_t									m_bRenderRecordVisuals;

		bool									m_bShowUI;
		bool									m_bExitNetworkThread;
		WWidgetManager*							m_pWidgetManager = nullptr;
		UI_FUNC_CALLBACK						m_fUIFuncCallback;

		std::string								IPV4_IPADDRESS_S;
		ULONG									IPV4_IPADDRESS_UL;

		BufferData								m_pSliceHeader;

		int32_t									m_iRecordFileCount;

		ObjectQueue<AVPacket>					m_PacketQueue;
		AVPacket								m_AVPacketReusable;
};

int main(int argc, char** argv)
{
	ULONG ulIPAddress = 0;
	std::cout << NetBase::getDynamicIPAddress(ulIPAddress);

	FFmpegYAGuiDroidCapture uiDroidCapture("../res/Recorded", NetBase::getDynamicIPAddress(ulIPAddress), ulIPAddress);

	exit(EXIT_SUCCESS);
}