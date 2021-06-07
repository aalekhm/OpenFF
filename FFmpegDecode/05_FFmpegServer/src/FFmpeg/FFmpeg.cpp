 #include "FFmpeg/FFmpeg.h"

bool FFmpeg::openReader(VideoReaderState& pState, const char* sFileName)
{
	auto& pAVFormatContext		= pState.m_pAVFormatContext;
	auto& pAVCodecContext		= pState.m_pAVCodecContext;
	auto& pAVPacket				= pState.m_pAVPacket;
	auto& pAVFrame				= pState.m_pAVFrame;
	auto* pSwsContext			= pState.m_pSwsContext;
	pState.m_iVideoStreamIndex	= -1;

	// 1.	Create a empty 'AVFormatContext' object.
	//		'AVFormatContext' basically is an handle to the Audio-Video properties of the input media.
	//		Allocate an AVFormatContext.
	pAVFormatContext = avformat_alloc_context();
	if (pAVFormatContext == nullptr)
	{
		std::cout << "Couldn't create 'AVFormatContext'.\n";
		return false;
	}

	// 2.	Query the input media & try to load the 'AVFormatContext'.
	//		Open an input stream and read the header.The codecs are not opened.
	//		The stream must be closed with avformat_close_input().
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
			pState.m_pAVStream = pState.m_pAVFormatContext->streams[i];

			pState.m_TimeBase = pAVStream->time_base;
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

	// 5.	Check if we have a valid decoder to decode the 'video codec'
	//		Find a registered decoder with a matching codec ID.
	AVCodec* pAVCodec = avcodec_find_decoder(pAVCodecParameters->codec_id);
	if (pAVCodec == nullptr)
	{
		std::cout << "Couldn't find a codec to decode the video stream format.\n";
		return false;
	}

	// 6.	Setup a codec context for the decoder.
	//		'AVCodecContext' is the internal data structure that holds the decoder data.
	//		Allocate an AVCodecContext and set its fields to default values.
	//		The resulting struct should be freed with avcodec_free_context().
	pAVCodecContext = avcodec_alloc_context3(pAVCodec);
	if (pAVCodecContext == nullptr)
	{
		std::cout << "Couldn't create 'AVCodecContext'.\n";
		return false;
	}

	// 7.	Populate the 'AVCodecContext' from the provided parameters.
	//		Fill the codec context based on the values from the supplied codec parameters.
	//		Any allocated fields in codec that have a corresponding field in
	//		par are freed and replaced with duplicates of the corresponding field in par.
	//		Fields in codec that do not have a counterpart in par are not touched.
	if (avcodec_parameters_to_context(pAVCodecContext, pAVCodecParameters))
	{
		std::cout << "Couldn't initialize 'AVCodecContext'.\n";
		return false;
	}

	// 8.	Initialize the AVCodecContext to use the given AVCodec. 
	//		Prior to using this function the context has to be allocated with avcodec_alloc_context3().
	if (avcodec_open2(pAVCodecContext, pAVCodec, NULL) < 0)
	{
		std::cout << "Couldn't open codec.\n";
		return false;
	}

	// 9.	A video file internally consists of a series of packets('AVPacket') lined one after the other.
	//		These packets contain frame data(s) from an Audio, Video or any other data stream.
	//		So, we retrieve a packet one by one, give it to the decoder.
	//		The decoder then decodes the packet & gives us a raw frame('AVFrame')
	//		This frame can contain Audio, Video or any other Frame related data.
	//		Allocate an AVPacket and set its fields to default values.
	//		The resulting struct must be freed using av_packet_free().
	pAVPacket = av_packet_alloc();
	if (pAVPacket == nullptr)
	{
		std::cout << "Couldn't allocate 'AVPacket'.\n";
		return false;
	}

	// 10.	Allocate an AVFrame and set its fields to default values.
	//		The resulting struct must be freed using av_frame_free().
	pAVFrame = av_frame_alloc();
	if (pAVFrame == nullptr)
	{
		std::cout << "Couldn't allocate 'AVFrame'.\n";
		return false;
	}

	// 11.	Allocate space for the raw pixel data in RGBA format(1 byte each colour).
	pState.m_pRawFrameData = (uint8_t*)std::malloc(pState.m_iWidth * pState.m_iHeight * 4);
	return true;
}

bool FFmpeg::readFrame(VideoReaderState& pState, std::function<void(AVPacket*)>& fSendPacketCallback)
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
		iResponse = avcodec_send_packet(pAVCodecContext, pAVPacket);
		if (iResponse < 0)
		{
			//char* sErrorResponse = av_err2str(iResponse);
			//std::cout << "Couldn't decode packet: " << sErrorResponse << "\n";
			std::cout << "Couldn't decode 'AVpacket'.\n";
			return false;
		}

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

		// 15.	At the end, we simply decrease the reference count for the 'AVPacket' as we no longer need it.
		//		On the next frame, it will be populated with that frames contextual data.
		av_packet_unref(pAVPacket);
		break;
	}

	// 16.	The 'Software Scaler'.
	//		Allocate and return an SwsContext.You need it to perform
	//		scaling / conversion operations using sws_scale().
	if(pSwsContext == nullptr)
	{
		pSwsContext = sws_getContext(	pState.m_iWidth,				// Source Width of the Video Frame
										pState.m_iHeight,				// Source Height of the Video Frame
										pAVCodecContext->pix_fmt,		// Pixel Format of the Video Frame
										pState.m_iWidth,				// Destination Width of the Video Frame
										pState.m_iHeight,				// Destination Height of the Video Frame
										AV_PIX_FMT_RGB0,				// Destination Image format
										SWS_BILINEAR,					// Specify the Algorithm use to do the rescaling
										NULL,
										NULL,
										NULL);

		if (NOT pSwsContext)
		{
			std::cout << "Couldn't initialize sw scaler.\n";
			return false;

		}
	}

	uint8_t* pDest[4] = { pState.m_pRawFrameData, NULL, NULL, NULL };
	int iDestLineSize[4] = { pAVFrame->width * 4, 0, 0, 0 };

	// 17.	Read the raw video contents from the 'AVFrame' & populate the Destination buffer with the relevant details.
	//		Scale the image slice in srcSlice and put the resulting scaled slice in the image in dst.
	//		A slice is a sequence of consecutive rows in an image.
	sws_scale(pSwsContext, pAVFrame->data, pAVFrame->linesize, 0, pAVFrame->height, pDest, iDestLineSize);

	// Capture the pts(Presentation Time Stamp) for the current frame that will be used while rendering the frame.
	pState.m_lPresentationTimeStamp = pAVFrame->pts;

	std::cout	<< "Frame "							<< av_get_picture_type_char(pAVFrame->pict_type) 
				<< " "								<< pAVCodecContext->frame_number 
				<< " pts "							<< pAVFrame->pts									// pts => Presentation Time Stamp
				<< " dts "							<< pAVFrame->pkt_dts								// dts => Decoding Time Stamp
				<< " key_frame  "					<< pAVFrame->key_frame
				<< " [coded_picture_number "		<< pAVFrame->coded_picture_number
				<< "] [display_picture_number "		<< pAVFrame->display_picture_number
				<< "]\n";

	return true;
}

bool FFmpeg::closeReader(VideoReaderState& pState)
{
	// 18. Cleanup
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
