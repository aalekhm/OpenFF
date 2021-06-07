#pragma once
#include "Common/Defines.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavFormat/avformat.h>
#include <libswscale/swscale.h>
}

struct VideoReaderState
{
	VideoReaderState()
	: m_pAVFormatContext(nullptr)
	, m_pAVCodecContext(nullptr)
	, m_pAVPacket(nullptr)
	, m_pAVFrame(nullptr)
	, m_pSwsContext(nullptr)
	, m_iVideoStreamIndex(-1)
	, m_pRawFrameData(nullptr)
	{
	}

	uint32_t				m_iWidth;
	uint32_t				m_iHeight;
	
	AVFormatContext*		m_pAVFormatContext;
	AVCodecContext*			m_pAVCodecContext;
	AVPacket*				m_pAVPacket;
	AVFrame*				m_pAVFrame;
	SwsContext*				m_pSwsContext;
	int32_t					m_iVideoStreamIndex;

	AVRational				m_TimeBase;
	int64_t					m_lPresentationTimeStamp;

	uint8_t*				m_pRawFrameData;
};

class FFmpeg
{
	public:
		static bool openReader(VideoReaderState& pState, const char* sFileName);
		static bool readFrame(VideoReaderState& pState);
		static bool closeReader(VideoReaderState& pState);
	protected:
	private:
};