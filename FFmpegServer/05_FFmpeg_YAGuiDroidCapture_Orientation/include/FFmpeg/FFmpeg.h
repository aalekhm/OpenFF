#pragma once
#include "Common/Defines.h"
#include <functional>
#include "Network/NetClient.h"
#include "Network/NetServer.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavFormat/avformat.h>
#include <libswscale/swscale.h>
#include "libavdevice/avdevice.h"
}

#define INPUT_READER_FORMAT		"gdigrab"
#define INPUT_URL				"desktop"
#define DUMP_TYPE_INPUT			0
#define DUMP_TYPE_OUTPUT		1

#pragma pack(1)
struct SMagicHeader
{
	char		MAGIC[8];
};

#pragma pack(1)
struct SDroidHeader
{
	// { 'D', 'R', 'O', 'I', 'D' };
	char		m_iModelName[64];
	uint32_t	m_iWidth;
	uint32_t	m_iHeight;
};

#pragma pack(1)
struct SDisplayHeader
{
	char		MAGIC[4] = { 'D', 'I', 'S', 'P' };
	uint32_t	m_iWidth;
	uint32_t	m_iHeight;
	uint32_t	m_iExtraDataSize;
};

#pragma pack(1)
struct SDataHeader
{
	// { 'D', 'A', 'T', 'A' };
	int64_t		m_iPresentationTimeStamp;
	uint32_t	m_iDataSize;
};

struct FFmpegNetworkState
{
	FFmpegNetworkState()
	: m_pExtraData(nullptr)
	{
	}

	SDisplayHeader	m_DisplayHeader;
	SDataHeader		m_DataHeader;
	SDroidHeader	m_AndroidHeader;
	uint8_t*		m_pExtraData;

	NetClient		m_NetClient;
	NetServer		m_NetServer;
};

struct FFmpegState
{
	FFmpegState()
	: m_iSrcWidth(0)
	, m_iSrcHeight(0)
	, m_iDstWidth(0)
	, m_iDstHeight(0)
	, m_pAVFormatContext(nullptr)
	, m_pAVCodecContext(nullptr)
	, m_pAVCodec(nullptr)
	, m_pAVCodecParserContext(nullptr)
	, m_pAVPacket(nullptr)
	, m_pAVFrame(nullptr)
	, m_pSwsContext(nullptr)
	, m_pAVStream(nullptr)
	, m_pAVOutputFormat(nullptr)
	, m_iVideoStreamIndex(-1)
	, m_pRawFrameData(nullptr)
	, m_iFrameCounter(-1)
	{
	}

	uint32_t				m_iSrcWidth;
	uint32_t				m_iSrcHeight;
	uint32_t				m_iDstWidth;
	uint32_t				m_iDstHeight;

	AVFormatContext*		m_pAVFormatContext;
	AVCodecContext*			m_pAVCodecContext;
	AVCodec*				m_pAVCodec;
	AVCodecParserContext*	m_pAVCodecParserContext;
	AVPacket*				m_pAVPacket;
	AVFrame*				m_pAVFrame;
	SwsContext*				m_pSwsContext;
	AVStream*				m_pAVStream;
	AVOutputFormat*			m_pAVOutputFormat;
	int32_t					m_iVideoStreamIndex;

	uint8_t*				m_pRawFrameData;

	AVRational				m_TimeBase;
	int64_t					m_lPresentationTimeStamp;

	int32_t					m_iFrameCounter;
};

class FFmpeg
{
	public:
		static bool closeReader(FFmpegState& pState);
	protected:
	private:
};