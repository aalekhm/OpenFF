#pragma once
#include "Common/Defines.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavFormat/avformat.h>
#include <libswscale/swscale.h>
}

class FFmpeg
{
	public:
		static bool loadFrame(const char* sFileName, uint32_t& iWidthOut, uint32_t& iHeightOut, uint8_t** pFrameDataOut);
	protected:
	private:
};