#include "Network/NetDefines.h"
#include <stdlib.h>
#include "Engine/GameEngine.h"
#include "Engine/InputManager.h"
#include "FFmpeg/FFmpeg.h"
#include "Common/Defines.h"
#include "Network/NetServer.h"
#include "Buffer/BufferedStreamWriter.h"

#define NOT !

// Bartholomew ==> C++ Real-time video processing
// https://www.youtube.com/playlist?list=PLKucWgGjAuTbobNC28EaR9lbGQTVyD9IS
//
// ffmpeg - libav - tutorial - https://github.com/leandromoreira/ffmpeg-libav-tutorial
class FFmpegServer : public GameEngine
{
	public:
		enum EFFmpegState
		{
			FFmpeg_Invalid = -1,
			FFmpeg_StreamFomDisk_Init,
			FFmpeg_StreamToNetwork_Init,
			FFmpeg_StreamToNetwork_Continous,
			FFmpeg_Exit
		};

		FFmpegServer(const char* sInputVideoFile)
		: m_sInputVideoFile(sInputVideoFile)
		{
			handleState(EFFmpegState::FFmpeg_StreamFomDisk_Init);

			// Create a window of the size of our loaded video.
			createWindow(m_pFFmpegState_StreamFromDisk.m_iSrcWidth, m_pFFmpegState_StreamFromDisk.m_iSrcHeight, "FFmpeg Video Server!");
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
			pGraphics->clear(m_pFFmpegState_StreamFromDisk.m_pRawFrameData);
		}

		virtual void onDestroy()
		{
			FFmpeg::closeReader(m_pFFmpegState_StreamFromDisk);
		}

		void syncVideo()
		{
			double fPresentationTimeInSeconds = m_pFFmpegState_StreamFromDisk.m_lPresentationTimeStamp * (double)(m_pFFmpegState_StreamFromDisk.m_TimeBase.num) / (double)(double)(m_pFFmpegState_StreamFromDisk.m_TimeBase.den);
			while (fPresentationTimeInSeconds > glfwGetTime())
			{
				glfwWaitEventsTimeout(fPresentationTimeInSeconds - glfwGetTime());
			}
		}

		void setState(EFFmpegState eState)
		{
			m_eState = eState;
		}

		bool initStreamFromDisk(FFmpegState& pFFmpegState, const char* sInputVideoFile)
		{
			bool bInitSuccessful = false;
			{
				auto& pAVFormatContext		= pFFmpegState.m_pAVFormatContext;
				auto& pAVCodecContext		= pFFmpegState.m_pAVCodecContext;
				auto& pAVStream				= pFFmpegState.m_pAVStream;
				auto& pAVPacket				= pFFmpegState.m_pAVPacket;
				auto& pAVFrame				= pFFmpegState.m_pAVFrame;
				auto& iVideoStreamIndex		= pFFmpegState.m_iVideoStreamIndex;
				auto& iSrcWidth				= pFFmpegState.m_iSrcWidth;
				auto& iSrcHeight			= pFFmpegState.m_iSrcHeight;
				auto& iDstWidth				= pFFmpegState.m_iDstWidth;
				auto& iDstHeight			= pFFmpegState.m_iDstHeight;
				auto& sTimeBase				= pFFmpegState.m_TimeBase;

				iVideoStreamIndex = -1;

				// 1.	Create a empty 'AVFormatContext' object.
				//		'AVFormatContext' basically is an handle to the Audio-Video properties of the input media.
				//		Allocate an AVFormatContext.
				pAVFormatContext = avformat_alloc_context();
				if (pAVFormatContext != nullptr)
				{
					// 2.	Query the input media & try to load the 'AVFormatContext'.
					//		Open an input stream and read the header.The codecs are not opened.
					//		The stream must be closed with avformat_close_input().
					if (avformat_open_input(&pAVFormatContext, sInputVideoFile, nullptr, nullptr) == 0)
					{
						// 3. Iterate through the number of available streams(Audio or Video) & find the 1st valid 'Video' stream.
						AVCodecParameters* pAVCodecParameters = nullptr;
						for (int i = 0; i < pAVFormatContext->nb_streams; i++)
						{
							pAVCodecParameters = pAVFormatContext->streams[i]->codecpar;
							if (pAVCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
							{
								iVideoStreamIndex = i;
								pAVStream = pAVFormatContext->streams[i];

								sTimeBase = pAVStream->time_base;
								iSrcWidth = pAVCodecParameters->width;
								iSrcHeight = pAVCodecParameters->height;
								iDstWidth = iSrcWidth;
								iDstHeight = iSrcHeight;

								break;
							}
						}

						// 4. If we found a Video stream
						if (iVideoStreamIndex >= 0)
						{
							// 5.	Check if we have a valid decoder to decode the 'video codec'
							//		Find a registered decoder with a matching codec ID.
							AVCodec* pAVCodec = avcodec_find_decoder(pAVCodecParameters->codec_id);
							if (pAVCodec != nullptr)
							{
								// 6.	Setup a codec context for the decoder.
								//		'AVCodecContext' is the internal data structure that holds the decoder data.
								//		Allocate an AVCodecContext and set its fields to default values.
								//		The resulting struct should be freed with avcodec_free_context().
								pAVCodecContext = avcodec_alloc_context3(pAVCodec);
								if (pAVCodecContext != nullptr)
								{
									// 7.	Populate the 'AVCodecContext' from the provided parameters.
									//		Fill the codec context based on the values from the supplied codec parameters.
									//		Any allocated fields in codec that have a corresponding field in
									//		par are freed and replaced with duplicates of the corresponding field in par.
									//		Fields in codec that do not have a counterpart in par are not touched.
									if (avcodec_parameters_to_context(pAVCodecContext, pAVCodecParameters) >= 0)
									{
										// 8.	Initialize the AVCodecContext to use the given AVCodec. 
										//		Prior to using this function the context has to be allocated with avcodec_alloc_context3().
										if (avcodec_open2(pAVCodecContext, pAVCodec, NULL) == 0)
										{
											// 9.	A video file internally consists of a series of packets('AVPacket') lined one after the other.
											//		These packets contain frame data(s) from an Audio, Video or any other data stream.
											//		So, we retrieve a packet one by one, give it to the decoder.
											//		The decoder then decodes the packet & gives us a raw frame('AVFrame')
											//		This frame can contain Audio, Video or any other Frame related data.
											//		Allocate an AVPacket and set its fields to default values.
											//		The resulting struct must be freed using av_packet_free().
											pAVPacket = av_packet_alloc();
											if (pAVPacket != nullptr)
											{
												// 10.	Allocate an AVFrame and set its fields to default values.
												//		The resulting struct must be freed using av_frame_free().
												pAVFrame = av_frame_alloc();
												if (pAVFrame != nullptr)
												{	
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
			switch (eState)
			{
				case EFFmpegState::FFmpeg_StreamFomDisk_Init:
				{
					bool bInitSuccessful = initStreamFromDisk(m_pFFmpegState_StreamFromDisk, m_sInputVideoFile.c_str());
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
					bool bInitSuccessful = initNetworkServer(	m_pFFmpegNetworkState_Server,
																m_pFFmpegState_StreamFromDisk,
																IPV4_LOCALHOST, 
																PORT,
																true);
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
					syncVideo();

					static std::function<void(AVPacket*)> m_fSendPacket = [this](AVPacket* pAVPacket)
					{
						// Send AVPAcket->data
						{
							auto& sDataHeader	= m_pFFmpegNetworkState_Server.m_DataHeader;
							auto& pNetServer	= m_pFFmpegNetworkState_Server.m_NetServer;

							sDataHeader.m_iDataSize = pAVPacket->size;
							pNetServer.netSendAll(	(uint8_t*)&sDataHeader,
													sizeof(DataHeader));
							
							pNetServer.netSendAll(	pAVPacket->data,
													sDataHeader.m_iDataSize);
						}
					};

					displayAndStreamVideo(m_pFFmpegState_StreamFromDisk, m_fSendPacket);
				}
				break;
				case EFFmpegState::FFmpeg_Exit:
				{

				}
				break;
			}
		}

		void displayAndStreamVideo(FFmpegState& pState, std::function<void(AVPacket*)>& fSendPacketCallback)
		{
			auto& pAVFormatContext	= pState.m_pAVFormatContext;
			auto& pAVCodecContext	= pState.m_pAVCodecContext;
			auto& pAVPacket			= pState.m_pAVPacket;
			auto& pAVFrame			= pState.m_pAVFrame;
			auto& pSwsContext		= pState.m_pSwsContext;

			// Keep reading the packets...
			int iResponse = -1;

			// 12.	Return the next frame of a stream.
			//		This function returns what is stored in the file, and does not validate
			//		that what is there are valid frames for the decoder.It will split what is
			//		stored in the file into frames and return one for each call.It will not
			//		omit invalid data between valid frames so as to give the decoder the maximum
			//		information possible for decoding.
			while (av_read_frame(pAVFormatContext, pAVPacket) >= 0)
			{
				//	Ignore all other streams, we are only looking for a video stream.
				//	The packet must be freed with av_packet_unref() when it is no longer needed.
				//	For video, the packet contains exactly one frame.
				if (pAVPacket->stream_index != pState.m_iVideoStreamIndex)
				{
					av_packet_unref(pAVPacket);
					continue;
				}

				fSendPacketCallback(pAVPacket);

				// 13.		We then send the packet to our decoder for processing,
				//			which will in turn return us a frame.
				//			Supply raw packet data as input to a decoder.
				//			Internally, this call will copy relevant AVCodecContext fields, which can
				//			influence decoding per - packet, and apply them when the packet is actually
				//			decoded.
				//
				//			For decoding, call avcodec_send_packet() to give the decoder raw compressed data in an AVPacket.
				if (avcodec_send_packet(pAVCodecContext, pAVPacket) == 0)
				{
					// 14.	Return decoded output data('pAVFrame') from a decoder.
					//		It will be set to a reference-counted video or audio
					//		frame(depending on the decoder type) allocated by the
					//		decoder.Note that the function will always call
					//		av_frame_unref(frame) before doing anything else.
					iResponse = avcodec_receive_frame(pAVCodecContext, pAVFrame);
					if (iResponse == AVERROR(EAGAIN) || iResponse == AVERROR_EOF)
					{
						// So if its not a valid 'AVFrame' decrease the reference count.
						av_packet_unref(pAVPacket);
						continue;
					}
					else
					if (iResponse == 0)
					{
						////////////////////////////////////////////////////////////////////////////
						// Since we get the 'pix_fmt' only after we decode the 1st valid Video AVPacket,
						// we need to initialize 'SwsContext' here!
						if(m_pFFmpegState_StreamFromDisk.m_pSwsContext == nullptr)
						{
							// 11.	The 'Software Scaler'.
							//		Allocate and return an SwsContext.You need it to perform
							//		scaling / conversion operations using sws_scale().
							m_pFFmpegState_StreamFromDisk.m_pSwsContext = sws_getContext(	m_pFFmpegState_StreamFromDisk.m_iSrcWidth,		// Source Width of the Video Frame
																							m_pFFmpegState_StreamFromDisk.m_iSrcHeight,		// Source Height of the Video Frame
																							pAVCodecContext->pix_fmt,						// Pixel Format of the Video Frame
																							m_pFFmpegState_StreamFromDisk.m_iDstWidth,		// Destination Width of the Video Frame
																							m_pFFmpegState_StreamFromDisk.m_iDstHeight,		// Destination Height of the Video Frame
																							AV_PIX_FMT_RGBA,								// Destination Image format
																							SWS_BILINEAR,									// Specify the Algorithm use to do the rescaling
																							NULL,
																							NULL,
																							NULL);

							if (m_pFFmpegState_StreamFromDisk.m_pSwsContext != nullptr)
							{
								// 11.	Allocate space for the raw pixel data in RGBA format(1 byte each colour).
								m_pFFmpegState_StreamFromDisk.m_pRawFrameData = (uint8_t*)std::malloc(m_pFFmpegState_StreamFromDisk.m_iSrcWidth * m_pFFmpegState_StreamFromDisk.m_iSrcHeight * 4);
							}
						}
						////////////////////////////////////////////////////////////////////////////

						// At this point we got raw data decode in an 'AVFrame'.
						// AVFrame::data is basically a list if channels in 'YUV' format.
						// In 'YUV', 
						//		‘Y’ represents the brightness, or ‘luma’ value, 
						//		and 
						//		‘UV’ represents the color, or ‘chroma’ values.
						// Each channel will have either a:
						//		Y - 'Luminance'(Dark & bright) component(we will use this to draw our 'Gray Scale' image), 
						//		U - 'Hue'(If its a Red, Green or Blue) component 
						//		or a 
						//		V - 'Saturation' component
						// if this frame is a 'Video' frame.
						uint8_t* pDest[4] = { pState.m_pRawFrameData, NULL, NULL, NULL };
						int iDestLineSize[4] = { pState.m_iDstWidth * 4, 0, 0, 0 };

						// 17.	Read the raw video contents from the 'AVFrame' & populate the Destination buffer with the relevant details.
						//		Scale the image slice in srcSlice and put the resulting scaled slice in the image in dst.
						//		A slice is a sequence of consecutive rows in an image.
						sws_scale(pSwsContext, pAVFrame->data, pAVFrame->linesize, 0, pState.m_iSrcHeight, pDest, iDestLineSize);

						// Capture the pts(Presentation Time Stamp) for the current frame that will be used while rendering the frame.
						pState.m_lPresentationTimeStamp = pAVFrame->pts;

						LOG_CONSOLE("Frame "							<< av_get_picture_type_char(pAVFrame->pict_type) 
									<< " "								<< pAVCodecContext->frame_number 
									<< " pts "							<< pAVFrame->pts									// pts => Presentation Time Stamp
									<< " dts "							<< pAVFrame->pkt_dts								// dts => Decoding Time Stamp
									<< " key_frame  "					<< pAVFrame->key_frame
									<< " [coded_picture_number "		<< pAVFrame->coded_picture_number
									<< "] [display_picture_number "		<< pAVFrame->display_picture_number
									<< "]");

						// 15.	At the end, we simply decrease the reference count for the 'AVPacket' as we no longer need it.
						//		On the next frame, it will be populated with that frames contextual data.
						av_packet_unref(pAVPacket);
						break;
					}
				}
			}
		}

	protected:
	private:
		std::string			m_sInputVideoFile;

		FFmpegNetworkState	m_pFFmpegNetworkState_Server;
		FFmpegState			m_pFFmpegState_StreamFromDisk;
		//NetServer			m_pNetServer;

		//DataHeader			m_sDataHeader;
		EFFmpegState		m_eState;
};

int main(int argc, char** argv)
{
	FFmpegServer videoPlayer("../res/KissOfWar.mp4");

	exit(EXIT_SUCCESS);
}