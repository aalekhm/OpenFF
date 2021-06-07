 #include "FFmpeg/FFmpeg.h"

bool FFmpeg::openReader(VideoReaderState& pState, const char* sFileName)
{
	auto&		pAVFormatContext = pState.m_pAVFormatContext;
	auto&		pAVCodecContext = pState.m_pAVCodecContext;
	auto&		pAVPacket = pState.m_pAVPacket;
	auto&		pAVFrame = pState.m_pAVFrame;
	auto*		pSwsContext = pState.m_pSwsContext;
	pState.m_iVideoStreamIndex = -1;

	// 1. Create a empty 'AVFormatContext' object.
	// 'AVFormatContext' basically is an handle to the Audio-Video properties of the input media.
	pAVFormatContext = avformat_alloc_context();
	if (pAVFormatContext == nullptr)
	{
		std::cout << "Couldn't create 'AVFormatContext'.\n";
		return false;
	}

	// 2. Query the input media & try to load the 'AVFormatContext'.
	if (avformat_open_input(&pAVFormatContext, sFileName, nullptr, nullptr) != 0)
	{
		std::cout << "Couldn't open video file.\n";
		return false;
	}

	// 3. Iterate through the number of available streams(Audio or Video) & find the 1st valid 'Video' stream.
	AVCodecParameters* pAVCodecParameters = nullptr;
	for (int i = 0; i < pAVFormatContext->nb_streams; i++)
	{
		AVStream* pAVStream = pAVFormatContext->streams[i];
		pAVCodecParameters = pAVStream->codecpar;
		if (pAVCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			pState.m_iVideoStreamIndex = i;

			pState.m_iWidth = pAVCodecParameters->width;
			pState.m_iHeight = pAVCodecParameters->height;

			break;
		}
	}

	// 4. If we found a Video stream
	if (pState.m_iVideoStreamIndex == -1)
	{
		std::cout << "Couldn't find a video stream.\n";
		return false;
	}

	// 4. Check if we have a valid decoder to decode the 'video codec'
	AVCodec* pAVCodec = avcodec_find_decoder(pAVCodecParameters->codec_id);
	if (pAVCodec == nullptr)
	{
		std::cout << "Couldn't find a codec to decode the video stream format.\n";
		return false;
	}

	// 5. Setup a codec context for the decoder.
	// 'AVCodecContext' is the internal data structure that holds the decoder data.
	pAVCodecContext = avcodec_alloc_context3(pAVCodec);
	if (pAVCodecContext == nullptr)
	{
		std::cout << "Couldn't create 'AVCodecContext'.\n";
		return false;
	}

	if (avcodec_parameters_to_context(pAVCodecContext, pAVCodecParameters))
	{
		std::cout << "Couldn't initialize 'AVCodecContext'.\n";
		return false;
	}

	if (avcodec_open2(pAVCodecContext, pAVCodec, NULL) < 0)
	{
		std::cout << "Couldn't open codec.\n";
		return false;
	}

	// A video file internally consists of a series of packets('AVPacket') lined one after the other.
	// These packets contain frame data(s) from an Audio, Video or any other data stream.
	// So, we retrieve a packet one by one, give it to the decoder.
	// The decoder then decodes the packet & gives us a raw frame('AVFrame')
	// This frame can contain Audio, Video or any other Frame related data.
	pAVPacket = av_packet_alloc();
	if (pAVPacket == nullptr)
	{
		std::cout << "Couldn't allocate 'AVPacket'.\n";
		return false;
	}

	pAVFrame = av_frame_alloc();
	if (pAVFrame == nullptr)
	{
		std::cout << "Couldn't allocate 'AVFrame'.\n";
		return false;
	}

	pState.m_pRawFrameData = (uint8_t*)std::malloc(pState.m_iWidth * pState.m_iHeight * 4);
	return true;
}

bool FFmpeg::readFrame(VideoReaderState& pState)
{
	auto&		pAVFormatContext = pState.m_pAVFormatContext;
	auto&		pAVCodecContext = pState.m_pAVCodecContext;
	auto&		pAVPacket = pState.m_pAVPacket;
	auto&		pAVFrame = pState.m_pAVFrame;
	auto&		pSwsContext = pState.m_pSwsContext;

	// Keep reading the packets...
	int iResponse = -1;
	while (av_read_frame(pAVFormatContext, pAVPacket) >= 0)
	{
		// Ignore all other streams, we are only looking for a video stream.
		if (pAVPacket->stream_index != pState.m_iVideoStreamIndex)
		{
			continue;
		}

		// We then send the packet to our decoder for processing,
		// which will in turn return us a frame.
		iResponse = avcodec_send_packet(pAVCodecContext, pAVPacket);
		if (iResponse < 0)
		{
			//char* sErrorResponse = av_err2str(iResponse);
			//std::cout << "Couldn't decode packet: " << sErrorResponse << "\n";
			std::cout << "Couldn't decode 'AVpacket'.\n";
			return false;
		}

		iResponse = avcodec_receive_frame(pAVCodecContext, pAVFrame);
		if (iResponse == AVERROR(EAGAIN) || iResponse == AVERROR_EOF)
		{
			continue;
		}
		else
		if (iResponse < 0)
		{
			std::cout << "Couldn't decode 'AVpacket'.\n";
			return false;
		}

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

		// So we just break out of this & process the raw data

		av_packet_unref(pAVPacket);
		break;
	}

	// The 'Software Scaler'.
	if(pSwsContext == nullptr)
	{
		pSwsContext = sws_getContext(	pState.m_iWidth,
										pState.m_iHeight,
										pAVCodecContext->pix_fmt,
										pState.m_iWidth,
										pState.m_iHeight,
										AV_PIX_FMT_RGB0,
										SWS_BILINEAR,
										NULL,
										NULL,
										NULL);
	}

	if (NOT pSwsContext)
	{
		std::cout << "Couldn't initialize sw scaler.\n";
		return false;

	}

	uint8_t* pDest[4] = { pState.m_pRawFrameData, NULL, NULL, NULL };
	int iDestLineSize[4] = { pAVFrame->width * 4, 0, 0, 0 };

	// For a normal reader, which will read all the packets, we need to do this in a loop.
	sws_scale(pSwsContext, pAVFrame->data, pAVFrame->linesize, 0, pAVFrame->height, pDest, iDestLineSize);

	return true;
}

bool FFmpeg::closeReader(VideoReaderState& pState)
{
	// Cleanup
	sws_freeContext(pState.m_pSwsContext);
	avformat_close_input(&pState.m_pAVFormatContext);
	avformat_free_context(pState.m_pAVFormatContext);
	av_packet_free(&pState.m_pAVPacket);
	av_frame_free(&pState.m_pAVFrame);
	avcodec_free_context(&pState.m_pAVCodecContext);

	if (pState.m_pRawFrameData != nullptr)
	{
		std::free(pState.m_pRawFrameData);
	}

	return true;
}
