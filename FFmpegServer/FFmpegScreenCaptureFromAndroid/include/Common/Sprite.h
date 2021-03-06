#pragma once
#include <stdint.h>
#include "Pixel.h"
#include <memory>

class Sprite
{
	private:
		struct _cookie {};
	public:
											Sprite(_cookie);
		virtual								~Sprite();

		static std::unique_ptr<Sprite>		create(uint32_t iWidth, uint32_t iHeight);
		static std::unique_ptr<Sprite>		create(const char* sFileName);

		void								clear(Pixel p);
		void								clear(Pixel* pColourData);
		Pixel								getPixel(int32_t x, int32_t y);
		bool								setPixel(int32_t x, int32_t y, Pixel& p);

		Pixel*								getData();

		uint32_t							m_iWidth;
		uint32_t							m_iHeight;
		Pixel*								m_pColourData = nullptr;
	protected:
	private:
};
