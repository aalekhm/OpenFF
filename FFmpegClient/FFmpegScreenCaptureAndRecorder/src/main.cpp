#include <stdlib.h>
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "FFmpeg/FFmpeg.h"
#include "Common/Defines.h"
#include <sstream>

#define NOT !

#define SRC_INPUT_WIDTH			960
#define SRC_INPUT_HEIGHT		640

#define DEST_OUTPUT_WIDTH		480
#define DEST_OUTPUT_HEIGHT		320

#define FRAMERATE				60

#define STRINGYFY(__value__)		#__value__
#define WxH(__width__, __height__)	STRINGYFY(__width__ ##x ## __height__)

class FFmpegScreenCaptureAndRecorder : public GameEngine
{
	public:
		enum EFFmpegState
		{
			FFmpeg_Invalid = -1,
			FFmpeg_StreamFomDesktop_Init,
			FFmpeg_StreamToDisk_Init,
			FFmpeg_StreamToNetwork_Continous,
			FFmpeg_Exit
		};

		FFmpegScreenCaptureAndRecorder(const char* sFileOut)
		: m_sFileName(sFileOut)
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
			flushToFile(m_pFFmpegState_StreamToDisk);

			FFmpeg::closeReader(m_pFFmpegState_StreamFromDesktop);
			FFmpeg::closeReader(m_pFFmpegState_StreamToDisk);
		}

		void streamToDesktop(Graphics* pGraphics)
		{
			pGraphics->clear(m_pFFmpegState_StreamFromDesktop.m_pRawFrameData);
		}

		bool streamFromDesktop(FFmpegState& pFFmpegState)
		{
			auto& pAVFormatContext	= pFFmpegState.m_pAVFormatContext;
			auto& pAVCodecContext	= pFFmpegState.m_pAVCodecContext;
			auto& pAVFrame			= pFFmpegState.m_pAVFrame;
			auto& pSwsContext		= pFFmpegState.m_pSwsContext;
			auto& pRawFrameData		= pFFmpegState.m_pRawFrameData;

			// Read the incoming 'Packet'
			AVPacket packet;
			if (av_read_frame(pAVFormatContext, &packet))
			{
				setState(EFFmpegState::FFmpeg_Exit);
				return false;
			}

			// Send the 'Packet' to the 'Decoder'
			if (avcodec_send_packet(pAVCodecContext, &packet))
			{
				setState(EFFmpegState::FFmpeg_Exit);
				return false;
			}

			// Receive the 'Decoded' 'Frame'
			if (avcodec_receive_frame(pAVCodecContext, pAVFrame))
			{
				setState(EFFmpegState::FFmpeg_Exit);
				return false;
			}
					
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

				// De-Reference the 'Packet' as we no longer need it.
				av_packet_unref(&packet);
			}

			return true;
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
			std::cout << pAVFrame->pts << " " << pAVCodecContext->time_base.num << " " << pAVCodecContext->time_base.den << " " << iFrameCounter << std::endl;

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

				std::cout << "pkt key: " << (packetOut.flags & AV_PKT_FLAG_KEY) << " " << packetOut.size << " " << (counter++) << std::endl;
				uint8_t* size = ((uint8_t*)packetOut.data);
				std::cout << "first: " << (int)size[0] << " " << (int)size[1] << " " << (int)size[2] << " " << (int)size[3] << " " << (int)size[4] << " " << (int)size[5] << " " << (int)size[6] << " " << (int)size[7] << std::endl;

				av_interleaved_write_frame(pAVFormatContext, &packetOut);
				av_packet_unref(&packetOut);
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
						AVStream* pAVStream = avformat_new_stream(pAVFormatContext, pAVCodec);
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
																m_sFileName.c_str(),
																DEST_OUTPUT_WIDTH,
																DEST_OUTPUT_HEIGHT,
																DEST_OUTPUT_WIDTH,
																DEST_OUTPUT_HEIGHT,
																FRAMERATE,
																AV_PIX_FMT_RGBA,
																AV_PIX_FMT_YUV420P);

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
					std::cout << "Failed to close file" << err << std::endl;
				}
			}
		}
	protected:
	private:
		FFmpegState					m_pFFmpegState_StreamFromDesktop;
		FFmpegState					m_pFFmpegState_StreamToDisk;

		EFFmpegState				m_eState;
		std::string					m_sFileName;
};

int main(int argc, char** argv)
{
	FFmpegScreenCaptureAndRecorder screenRecorder("record.mp4");

	exit(EXIT_SUCCESS);
}