#include "FFmpeg/FFmpeg.h"

bool FFmpeg::loadFrame(const char* sFileName, uint32_t& iWidthOut, uint32_t& iHeightOut, uint8_t** pFrameDataOut)
{
	// 1. Create a empty 'AVFormatContext' object.
	// 'AVFormatContext' basically is an handle to the Audio-Video properties of the input media.
	AVFormatContext* pAVFormatContext = avformat_alloc_context();
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
	int32_t iVideoStreamIndex = -1;
	AVCodecParameters* pAVCodecParameters = nullptr;
	for (int i = 0; i < pAVFormatContext->nb_streams; i++)
	{
		AVStream* pAVStream = pAVFormatContext->streams[i];
		pAVCodecParameters = pAVStream->codecpar;
		if (pAVCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			iVideoStreamIndex = i;
			break;
		}
	}

	// 4. If we found a Video stream
	if (iVideoStreamIndex == -1)
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
	AVCodecContext* pAVCodecContext = avcodec_alloc_context3(pAVCodec);
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
	AVPacket* pAVPacket = nullptr;
	pAVPacket = av_packet_alloc();
	if (pAVPacket == nullptr)
	{
		std::cout << "Couldn't allocate 'AVPacket'.\n";
		return false;
	}

	AVFrame* pAVFrame = nullptr;
	pAVFrame = av_frame_alloc();
	if (pAVFrame == nullptr)
	{
		std::cout << "Couldn't allocate 'AVFrame'.\n";
		return false;
	}

	// Keep reading the packets...
	int iResponse = -1;
	while (av_read_frame(pAVFormatContext, pAVPacket) >= 0)
	{
		// Ignore all other streams, we are only looking for a video stream.
		if (pAVPacket->stream_index != iVideoStreamIndex)
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
		// AVFrame::data is basically a list if channels.
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

	uint8_t* pFrameData = new uint8_t[pAVFrame->width * pAVFrame->height * 4]; // RGBA
	for (int x = 0; x < pAVFrame->width; ++x)
	{
		for (int y = 0; y < pAVFrame->height; ++y)
		{
			pFrameData[y * pAVFrame->width * 4 + x * 4 + 0] = pAVFrame->data[0][y * pAVFrame->linesize[0] + x]; // Gray scale data
			pFrameData[y * pAVFrame->width * 4 + x * 4 + 1] = pAVFrame->data[0][y * pAVFrame->linesize[0] + x]; // Gray scale data
			pFrameData[y * pAVFrame->width * 4 + x * 4 + 2] = pAVFrame->data[0][y * pAVFrame->linesize[0] + x]; // Gray scale data
			pFrameData[y * pAVFrame->width * 4 + x * 4 + 3] = 0xff;
		}
	}

	iWidthOut = pAVFrame->width;
	iHeightOut = pAVFrame->height;
	*pFrameDataOut = pFrameData;

	// Cleanup
	avformat_close_input(&pAVFormatContext);
	avformat_free_context(pAVFormatContext);
	av_packet_free(&pAVPacket);
	av_frame_free(&pAVFrame);
	avcodec_free_context(&pAVCodecContext);

	return true;
}