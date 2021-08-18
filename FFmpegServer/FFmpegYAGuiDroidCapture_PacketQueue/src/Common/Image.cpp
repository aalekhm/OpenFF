#include "Common/Image.h"
#include "Common/Defines.h"

Image::Image(_cookie)
{
}

std::unique_ptr<Image> Image::createImage(const char* sTexWithPath)
{
	TGAImg tgaImg;

	// Load our Texture
	if (tgaImg.Load((char*)sTexWithPath) != IMG_OK)
	{
		return nullptr;
	}
	
	std::unique_ptr<Image> pImage = std::make_unique<Image>(_cookie{});
	pImage->m_iWidth = tgaImg.GetWidth();
	pImage->m_iHeight = tgaImg.GetHeight();

	unsigned int size = 0;
	switch (tgaImg.GetBPP())
	{
		case 32:
		{
			pImage->m_Format = Image::RGBA;
			size = pImage->m_iWidth * pImage->m_iHeight * 4;
		}
		break;
		case 24:
		{
			pImage->m_Format = Image::RGB;
			size = pImage->m_iWidth * pImage->m_iHeight * 3;
		}
		break;
	}

	pImage->m_pPixelData = (unsigned char*)std::malloc(sizeof(char) * size);
	memcpy(pImage->m_pPixelData, tgaImg.GetImg(), sizeof(unsigned char) * size);

	return pImage;
}

unsigned char* Image::getPixelData() const
{
	return m_pPixelData;
}

Image::Format Image::getFormat() const
{
	return m_Format;
}
unsigned int Image::getWidth() const
{
	return m_iWidth;
}

unsigned int Image::getHeight() const
{
	return m_iHeight;
}

Image::~Image()
{
	if (m_pPixelData != nullptr)
	{
		std::free(m_pPixelData);
	}
}