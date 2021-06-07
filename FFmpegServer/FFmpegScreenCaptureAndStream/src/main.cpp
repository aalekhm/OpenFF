#include "Network/NetDefines.h"
#include <stdlib.h>
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "FFmpeg/FFmpeg.h"
#include "Common/Defines.h"
#include "Network/NetServer.h"
#include "Buffer/BufferedStreamWriter.h"
#include <sstream>

#define NOT !

#define SRC_INPUT_WIDTH			960
#define SRC_INPUT_HEIGHT		640

#define DEST_OUTPUT_WIDTH		480
#define DEST_OUTPUT_HEIGHT		320

#define FRAMERATE				60

// Bartholomew ==> C++ Real-time video processing
// https://www.youtube.com/playlist?list=PLKucWgGjAuTbobNC28EaR9lbGQTVyD9IS
//
// ffmpeg - libav - tutorial - https://github.com/leandromoreira/ffmpeg-libav-tutorial
class FFmpegScreeCaptureAndStream : public GameEngine
{
	public:
		enum EFFmpegState
		{
			FFmpeg_Invalid = -1,
			FFmpeg_StreamFomDesktop_Init,
			FFmpeg_StreamToNetwork_Init,
			FFmpeg_NetworkServer_Init,
			FFmpeg_StreamToDisk_Init,
			FFmpeg_StreamToNetwork_Continous,
			FFmpeg_Exit,
		};

		FFmpegScreeCaptureAndStream(const char* sInputVideoFile)
		: m_sInputVideoFile(sInputVideoFile)
		{
			setState(EFFmpegState::FFmpeg_StreamFomDesktop_Init);

			// Create a window of the size of our loaded video.
			createWindow(DEST_OUTPUT_WIDTH, DEST_OUTPUT_HEIGHT, "FFmpeg Video Server!");
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
			flushToFile(m_pFFmpegState_StreamToDisk);

			FFmpeg::closeReader(m_pFFmpegState_StreamFromDesktop);
			FFmpeg::closeReader(m_pFFmpegState_StreamToNetwork);
			FFmpeg::closeReader(m_pFFmpegState_StreamToDisk);
		}

		void syncVideo()
		{
			double fPresentationTimeInSeconds = m_pFFmpegState_StreamFromDesktop.m_lPresentationTimeStamp * (double)(m_pFFmpegState_StreamFromDesktop.m_TimeBase.num) / (double)(double)(m_pFFmpegState_StreamFromDesktop.m_TimeBase.den);
			while (fPresentationTimeInSeconds > glfwGetTime())
			{
				glfwWaitEventsTimeout(fPresentationTimeInSeconds - glfwGetTime());
			}
		}

		void streamToDesktop(Graphics* pGraphics)
		{
			pGraphics->clear(m_pFFmpegState_StreamFromDesktop.m_pRawFrameData);
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
			LOG_CONSOLE(pAVFrame->pts << " " << pAVCodecContext->time_base.num << " " << pAVCodecContext->time_base.den << " " << iFrameCounter);

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

				LOG_CONSOLE("pkt key: " << (packetOut.flags & AV_PKT_FLAG_KEY) << " " << packetOut.size << " " << (counter++));
				uint8_t* size = ((uint8_t*)packetOut.data);
				LOG_CONSOLE("first: " << (int)size[0] << " " << (int)size[1] << " " << (int)size[2] << " " << (int)size[3] << " " << (int)size[4] << " " << (int)size[5] << " " << (int)size[6] << " " << (int)size[7]);

				av_interleaved_write_frame(pAVFormatContext, &packetOut);
				av_packet_unref(&packetOut);
			}
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
			LOG_CONSOLE(pAVFrame->pts << " " << pAVCodecContext->time_base.num << " " << pAVCodecContext->time_base.den << " " << iFrameCounter);

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

		void setState(EFFmpegState eState)
		{
			m_eState = eState;
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

		bool initStreamToNetwork(	FFmpegState& pFFmpegState,
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

			pAVFormatContext = avformat_alloc_context();
			if (pAVFormatContext != nullptr)
			{
				AVCodec* pAVCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
				if (pAVCodec)
				{
					AVStream* pAVStream = avformat_new_stream(pAVFormatContext, pAVCodec);
					if (pAVStream)
					{
						pAVStream->codecpar->codec_id	= AV_CODEC_ID_H264;
						pAVStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
						pAVStream->codecpar->width		= iDestW;
						pAVStream->codecpar->height		= iDestH;
						pAVStream->codecpar->format		= OUT_AVPixelFormat;
						//pAVStream->codecpar->bit_rate	= 2000 * 1000;

						pAVCodecContext = avcodec_alloc_context3(pAVCodec);
						if (pAVCodecContext)
						{
							avcodec_parameters_to_context(pAVCodecContext, pAVStream->codecpar);
							pAVCodecContext->time_base.num = 1;
							pAVCodecContext->time_base.den = 1;
							pAVCodecContext->max_b_frames = 2;
							pAVCodecContext->framerate.num = iFramerate;
							pAVCodecContext->framerate.den = 1;
							//pAVCodecContext->gop_size = 12;

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
								pAVFrame = av_frame_alloc();
								pAVFrame->format = OUT_AVPixelFormat;
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

									if (pSwsContext != nullptr)
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
			bool bInitSuccessful = false;

			auto& pAVFormatContext	= pFFmpegState.m_pAVFormatContext;
			auto& pAVCodecContext	= pFFmpegState.m_pAVCodecContext;
			auto& pAVOutputFormat	= pFFmpegState.m_pAVOutputFormat;
			auto& pAVStream			= pFFmpegState.m_pAVStream;
			auto& pAVFrame			= pFFmpegState.m_pAVFrame;
			auto& pSwsContext		= pFFmpegState.m_pSwsContext;

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

		bool initNetworkServer(	FFmpegNetworkState& pFFmpegNetworkState,
								FFmpegState& pFFmpegState, 
								int64_t iNetworkAddress, 
								int32_t iNetworkPort,
								bool bHasExtraData = false)
		{
			bool bInitSuccessful = false;

			auto& pNetServer		= pFFmpegNetworkState.m_NetServer;
			auto& iIPAddress		= pNetServer.m_iIPAddress;
			auto& iPort				= pNetServer.m_iPort;
			auto& sDisplayHeader	= pFFmpegNetworkState.m_DisplayHeader;
			
			iIPAddress = iNetworkAddress;
			iPort = iNetworkPort;

			if (pNetServer.netInitialize())
			{
				if (pNetServer.netListen(iIPAddress, iPort, 1) > 0)
				{	
					sDisplayHeader.m_iWidth = pFFmpegState.m_iDstWidth;
					sDisplayHeader.m_iHeight = pFFmpegState.m_iDstHeight;
					sDisplayHeader.m_iExtraDataSize = bHasExtraData ? pFFmpegState.m_pAVStream->codecpar->extradata_size : 0;

					pNetServer.netSendAll(	(uint8_t*)&sDisplayHeader,
											sizeof(DisplayHeader));

					if (bHasExtraData)
					{
						pNetServer.netSendAll(	pFFmpegState.m_pAVStream->codecpar->extradata,
												sDisplayHeader.m_iExtraDataSize);
					}

					bInitSuccessful = true;
				}
			}

			return bInitSuccessful;
		}

		void handleState(EFFmpegState eState)
		{
			m_eState = eState;
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
						setState(EFFmpegState::FFmpeg_StreamToNetwork_Init);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_StreamToNetwork_Init:
				{
					bool bInitSuccessful = initStreamToNetwork(	m_pFFmpegState_StreamToNetwork, 
																DEST_OUTPUT_WIDTH, 
																DEST_OUTPUT_HEIGHT, 
																DEST_OUTPUT_WIDTH, 
																DEST_OUTPUT_HEIGHT, 
																FRAMERATE,
																AV_PIX_FMT_RGBA,
																AV_PIX_FMT_YUV420P);
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
					bool bInitSuccessful = initStreamToDisk(	m_pFFmpegState_StreamToDisk,
																m_sInputVideoFile.c_str(),
																DEST_OUTPUT_WIDTH,
																DEST_OUTPUT_HEIGHT,
																DEST_OUTPUT_WIDTH,
																DEST_OUTPUT_HEIGHT,
																FRAMERATE,
																AV_PIX_FMT_RGBA,
																AV_PIX_FMT_YUV420P);

					if (bInitSuccessful)
					{
						setState(EFFmpegState::FFmpeg_NetworkServer_Init);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_NetworkServer_Init:
				{
					bool bInitSuccessful = initNetworkServer(	m_pFFmpegNetworkState_Server,
																m_pFFmpegState_StreamFromDesktop,
																IPV4_LOCALHOST, 
																PORT, 
																false);
					if (bInitSuccessful)
					{
						setState(EFFmpegState::FFmpeg_StreamToNetwork_Continous);
					}
					else
					{
						setState(EFFmpegState::FFmpeg_Exit);
					}
				}
				break;
				case EFFmpegState::FFmpeg_StreamToNetwork_Continous:
				{
					if (NOT streamFromDesktop(m_pFFmpegState_StreamFromDesktop))
					{
						break;
					}

					streamToDisk(m_pFFmpegState_StreamFromDesktop, m_pFFmpegState_StreamToDisk);
					streamToNetwork(	m_pFFmpegNetworkState_Server,
										m_pFFmpegState_StreamFromDesktop, 
										m_pFFmpegState_StreamToNetwork);
				}
				break;
				case EFFmpegState::FFmpeg_Exit:
				{

				}
				break;
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
	protected:
	private:
		std::string					m_sInputVideoFile;

		FFmpegNetworkState			m_pFFmpegNetworkState_Server;

		FFmpegState					m_pFFmpegState_StreamFromDesktop;
		FFmpegState					m_pFFmpegState_StreamToNetwork;
		FFmpegState					m_pFFmpegState_StreamToDisk;

		EFFmpegState				m_eState;
};

int main(int argc, char** argv)
{
	FFmpegScreeCaptureAndStream videoPlayer("Recorded.mp4");

	exit(EXIT_SUCCESS);
}