 #include "FFmpeg/FFmpeg.h"

bool FFmpeg::closeReader(FFmpegState& pState)
{
	// Cleanup
	if (pState.m_pAVFormatContext != nullptr)
	{
		avformat_free_context(pState.m_pAVFormatContext);
		pState.m_pAVFormatContext = nullptr;
	}

	if (pState.m_pSwsContext != nullptr)
	{
		sws_freeContext(pState.m_pSwsContext);
	}

	if (pState.m_pAVStream != nullptr)
	{
		avcodec_close(pState.m_pAVCodecContext);
	}

	if (pState.m_pAVCodecContext != nullptr)
	{
		avcodec_free_context(&pState.m_pAVCodecContext);
	}

	if (pState.m_pAVPacket != nullptr)
	{
		av_packet_free(&pState.m_pAVPacket);
	}

	if (pState.m_pAVFrame != nullptr)
	{
		av_frame_free(&pState.m_pAVFrame);
	}

	if (pState.m_pRawFrameData != nullptr)
	{
		std::free(pState.m_pRawFrameData);
	}

	{
		pState.m_iSrcWidth = 0;
		pState.m_iSrcHeight = 0;
		pState.m_iDstWidth = 0;
		pState.m_iDstHeight = 0;
		pState.m_pAVFormatContext = nullptr;
		pState.m_pAVCodecContext = nullptr;
		pState.m_pAVCodec = nullptr;
		pState.m_pAVCodecParserContext = nullptr;
		pState.m_pAVPacket = nullptr;
		pState.m_pAVFrame = nullptr;
		pState.m_pSwsContext = nullptr;
		pState.m_pAVStream = nullptr;
		pState.m_pAVOutputFormat = nullptr;
		pState.m_iVideoStreamIndex = -1;
		pState.m_pRawFrameData = nullptr;
		pState.m_iFrameCounter = -1;
	}

	return true;
}
