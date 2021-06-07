#include "Network/NetDefines.h"
#include <stdlib.h>
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "FFmpeg/FFmpeg.h"
#include "Common/Defines.h"
#include "Network/NetServer.h"
#include "Buffer/BufferedStreamWriter.h"
#include <sstream>
#include "Buffer/BufferData.hpp"
#include <thread>
#include <future>
#include "Engine/UI/WWidgetManager.h"
#include "Engine/UI/WComponentFactory.h"

#define NOT !

#define FRAMERATE						60
#define DEST_SCALE						0.40
#define SERVER_CONNECT_DELAY			1500

#define DEVICE_SERVER_PATH				"/data/local/tmp/"
#define DEVICE_SOCKET_NAME				"AndroidRemoteControl"

#define ANDROID_SERVER_BINARY			"remote-server"
#define ANDROID_CLIENT_BINARY			"remote-client"
#define ANDROID_SERVER_PACKAGE_NAME		"com.ea.remotecontrol.RemoteServer"
#define ANDROID_CLIENT_PACKAGE_NAME		"com.ea.remotecontrol.RemoteClient"

#define RESOURCES						"../res/"

#define ID_WINDOW_RECORD				2000
#define ID_BUTTON_RECORD				2001
#define ID_BUTTON_STOP					2002
#define ID_LABEL_FILENAME				2003
#define ID_TEXTFIELD_FILENAME			2004
#define ID_LABEL_DIRECTORY				2005
#define ID_TEXTFILED_DIRECTORY			2006

HANDLE	m_HProc;
const float_t ONE_OVER_DEST_SCALE		= 1 / DEST_SCALE;

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

		FFmpegYAGuiDroidCapture(const char* sInputVideoFile)
		: m_sInputVideoFile(sInputVideoFile)
		, m_bExitNetworkThread(false)
		{
			handleState(EFFmpegState::Android_PushClient);

			// Create a window of the size of our loaded video.
			createWindow(	m_pFFmpegNetworkState.m_AndroidHeader.m_iWidth * DEST_SCALE, 
							m_pFFmpegNetworkState.m_AndroidHeader.m_iHeight * DEST_SCALE,
							"FFmpeg Android Server!");
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

		virtual void onDestroy()
		{
			cleanUp();
		}

		void cleanUp()
		{
			sendQuit();

			adbRemoveBinary(ANDROID_SERVER_BINARY, DEVICE_SERVER_PATH);
			adbRemoveBinary(ANDROID_CLIENT_BINARY, DEVICE_SERVER_PATH);

			NetBase& netBase = getNetworkState();
			netBase.netClose();
			
			FFmpeg::closeReader(m_pFFmpegState_StreamToDesktop);
			FFmpeg::closeReader(m_pFFmpegState_StreamToDisk);
			
			CloseHandle(m_HProc);
		}

		void streamToDesktop(Graphics* pGraphics)
		{
			pGraphics->clear(m_pFFmpegState_StreamToDesktop.m_pRawFrameData);
		}

		void streamToNetwork(	FFmpegNetworkState& pFFmpegNetworkState, 
								FFmpegState& pIN_FFmpegState, 
								FFmpegState& pOUT_FFmpegState)
		{
			auto& sDataHeader	= pFFmpegNetworkState.m_DataHeader;
			auto& pNetServer	= pFFmpegNetworkState.m_NetServer;

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
			//LOG_CONSOLE(pAVFrame->pts << " " << pAVCodecContext->time_base.num << " " << pAVCodecContext->time_base.den << " " << iFrameCounter);

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
											sizeof(DataHeader));

					pNetServer.netSendAll(	pAVPacketOut.data,
											sDataHeader.m_iDataSize);

					av_packet_unref(&pAVPacketOut);
				}
			}
		}

		void streamFromNetwork(	FFmpegNetworkState& pFFmpegNetworkState,
								FFmpegState& pFFmpegState)
		{
			BufferData pBufferHeader;
			BufferData pBufferData;
			auto& sDataHeader = pFFmpegNetworkState.m_DataHeader;
			NetBase& netBase = getNetworkState();

			// Receive 'AVPacket'.
			{
				// 'AVPacket' size.	
				pBufferHeader.setSize(sizeof(DataHeader));

				netBase.netRecvAll(	&sDataHeader,
									sizeof(DataHeader));

				// 'AVPacket' contents.
				pBufferData.setSize(sDataHeader.m_iDataSize);
				netBase.netRecvAll(	pBufferData.m_pBuffer,
									sDataHeader.m_iDataSize);
			}

			// We might be waiting for a packet, but the main thread must have tried to exit.
			{
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
							//// Optional
							//// Check 'AVPacket' data validity
							//{
							//	uint8_t* pDataIn = pAVPacket.data;
							//	int iSizeIn = pAVPacket.size;
							//	uint8_t* pDataOut = NULL;
							//	int iSizeOut = 0;
							//
							//	iRet = av_parser_parse2(pAVCodecParserContext,
							//		pAVCodecContext,
							//		&pDataOut, &iSizeOut,
							//		pDataIn, iSizeIn,
							//		AV_NOPTS_VALUE, AV_NOPTS_VALUE, -1);
							//	assert(iRet == iSizeIn);
							//	(void)iRet;
							//	assert(iSizeOut == iSizeIn);
							//}

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
			//LOG_CONSOLE(pAVFrame->pts << " " << pAVCodecContext->time_base.num << " " << pAVCodecContext->time_base.den << " " << iFrameCounter);

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
					// Receive 'Handshake' header.
					{
						pNetServer.netRecvAll(&sAndroidHeader, sizeof(AndroidHeader));
						bInitSuccessful = true;
					}
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
						pNetClient.netRecvAll(&sAndroidHeader, sizeof(AndroidHeader));
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

		HANDLE adbExecuteServer(const char* pServerName, const char* pDeviceDestPath, const char* pFullPackageName)
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
						<< pFullPackageName;

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
			m_eState = eState;
			switch (m_eState)
			{
				case Android_PushServer:
				{
					bool bPushSuccessful = adbPushBinary(ANDROID_SERVER_BINARY, DEVICE_SERVER_PATH);
					if (bPushSuccessful)
					{
						adbForward(PORT, DEVICE_SOCKET_NAME);
						m_HProc = adbExecuteServer(ANDROID_SERVER_BINARY, DEVICE_SERVER_PATH, ANDROID_SERVER_PACKAGE_NAME);

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
						m_HProc = adbExecuteServer(ANDROID_CLIENT_BINARY, DEVICE_SERVER_PATH, ANDROID_CLIENT_PACKAGE_NAME);

						handleState(EFFmpegState::FFmpeg_PCServer_Init);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_PCServer_Init:
				{
					bool bInitSuccessful = initPCServer(	m_pFFmpegNetworkState,
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
						
								sInputVideoFile << sDirectory << "\\" << sFileName << iFileCount++ << ".mp4";
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
						setState(FFmpeg_StreamFromNetwork_Continous);
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

								if (m_bExitNetworkThread)
								{
									setState(EFFmpegState::FFmpeg_Exit);
									break;
								}
							}
						};

						m_pNetworkThread = std::make_unique<std::thread>(m_fNetworkThreadFunc);
						m_pNetworkThread->detach();
					}

					// Empty as 'streamFromNetwork()' is called on 'm_pNetworkThread'.
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
					H_WND hWnd_Record = WWidgetManager::getInstance()->GetWindowQ(ID_WINDOW_RECORD);
					SendMessageQ(hWnd_Record, m_bShowUI ? WM__SHOW : WM__HIDE, NULL, NULL);
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
				//LOG_CONSOLE("iAction = " << iAction << ", iKey = " << iKey << ", XPos = " << dXPos * ONE_OVER_DEST_SCALE << ", dYPos = " << dYPos * ONE_OVER_DEST_SCALE);

				NetBase& netBase = getNetworkState();
				SWriteInfo pSWriteInfo;

				pSWriteInfo.MAGIC = 'M';
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
				//LOG_CONSOLE("iAction = " << iAction << ", iAndroidKeyCode = " << iAndroidKeyCode);

				NetBase& netBase = getNetworkState();
				SWriteInfo pSWriteInfo;
				
				pSWriteInfo.MAGIC = 'K';
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
			if(m_bShowUI)
			{
				//std::cout << "iKeyCode = " << iKeyCode << ", iScanCode = " << iScanCode << ", iModifierKeys = " << iModifierKeys << "\n";

				WWidgetManager* pWidgetManager = WWidgetManager::getInstance();
				bool CONTROL_ON = pWidgetManager->isModifierKeyDown(GLFW_MOD_CONTROL);
				bool SHIFT_ON = pWidgetManager->isModifierKeyDown(GLFW_MOD_SHIFT);

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
		}

		void onUnicodeCharCallback(uint32_t iUnicodeChar)
		{
			if (m_bShowUI)
			{
				m_pWidgetManager->keyPressed(0, iUnicodeChar);
			}
		}

		void sendQuit()
		{
			NetBase& netBase = getNetworkState();
			SWriteInfo pSWriteInfo;
			{
				pSWriteInfo.MAGIC = 'Q';
				pSWriteInfo.m_iSize = 0;
				netBase.netSendAll(	(uint8_t*)&pSWriteInfo,
									sizeof(SWriteInfo));
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
				m_pWidgetManager->init(m_iWidth, m_iHeight);				
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
					if (wParam == ROOT_WINDOW_ID)
					{
						WComponent* pComp = (WComponent*)hWnd;
						pComp->m_RenderColor = Color(1.0, 1.0f, 1.0f, 0.0f);
						addUIElements(hWnd);
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
					if (wParam == -1 && lParam == -1) // WWidgetManager
					{
						if (m_eStreamState == EStreamState::NetworkStream_Record)
						{
							renderRecordVisuals();
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
									H_WND hWnd_Record = WWidgetManager::getInstance()->GetWindowQ(ID_WINDOW_RECORD);
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
					}
					
				}
				break;
			}

			return WM__OKAY;
		}

		void addRecordUI(H_WND hRootWnd)
		{
			H_WND hWnd, hRecordFrame;
			int32_t iXStartPos = 20;
			int32_t iXPos = iXStartPos;
			int32_t iYPos = 50;
			ADD_UI_FRAME(hRootWnd, hRecordFrame, "Record Options", WM_ALIGN_WRT_PARENT_TOPCENTER | WM_ANCHOR_TOPCENTER, iXPos, iYPos, m_iWidth - 40, 200, ID_WINDOW_RECORD, ID_TYPE_WND_C)
			((WFrame*)hRecordFrame)->setVisible(false);
			{
				ADD_UI_BUTTON(hRecordFrame, hWnd, "Record", WM_ALIGN_WRT_PARENT_TOPCENTER | WM_ANCHOR_CENTER, iXPos, iYPos, 150, 25, ID_BUTTON_RECORD)
				SendMessageQ(hWnd, BM__ENABLE, NULL, NULL);
				
				iYPos += 30;
				ADD_UI_BUTTON(hRecordFrame, hWnd, "Stop", WM_ALIGN_WRT_PARENT_TOPCENTER | WM_ANCHOR_CENTER, iXPos, iYPos, 150, 25, ID_BUTTON_STOP)
				SendMessageQ(hWnd, BM__DISABLE, NULL, NULL);
						
				iYPos += 30;
				ADD_UI_LABEL(hRecordFrame, hWnd, "Filename :", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_TOPLEFT, iXPos, iYPos, 150, 25, ID_LABEL_FILENAME)
				
				iXPos += 100;
				ADD_UI_TEXTFIELD(hRecordFrame, hWnd, "Recorded", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_TOPLEFT, iXPos, iYPos, 250, 25, ID_TEXTFIELD_FILENAME)
				
				iXPos = iXStartPos;
				iYPos += 30;
				ADD_UI_LABEL(hRecordFrame, hWnd, "Directory :", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_TOPLEFT, iXPos, iYPos, 150, 25, ID_LABEL_DIRECTORY)
				
				iXPos += 100;
				ADD_UI_TEXTFIELD(hRecordFrame, hWnd, "..\\res", WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_TOPLEFT, iXPos, iYPos, 250, 25, ID_TEXTFILED_DIRECTORY)
			}
		}

		void addUIElements(H_WND hRootWnd)
		{
			H_WND hWnd;
			addRecordUI(hRootWnd);
		}
	protected:
	private:
		FFmpegNetworkState						m_pFFmpegNetworkState;

		FFmpegState								m_pFFmpegState_StreamToDesktop;
		FFmpegState								m_pFFmpegState_StreamToDisk;

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
};

int main(int argc, char** argv)
{
	FFmpegYAGuiDroidCapture uiDroidCapture("../res/Recorded");

	exit(EXIT_SUCCESS);
}