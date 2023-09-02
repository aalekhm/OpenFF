 #include "FFmpeg/FFmpeg.h"

bool FFmpeg::closeReader(FFmpegState& pState)
{
	// Cleanup
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

	if (pState.m_pAVFormatContext != nullptr)
	{
		avformat_free_context(pState.m_pAVFormatContext);
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

	return true;
}
