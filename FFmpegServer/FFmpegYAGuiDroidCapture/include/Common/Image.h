#pragma once

#include "TGA.h"
#include <memory>

class Image 
{
	private:
		struct _cookie {};
	public:
		enum Format
		{
			RGB,
			RGBA
		};

										~Image();
										Image(_cookie);
		Image&							operator=(const Image&);
		static std::unique_ptr<Image>	createImage(const char* sTexWithPath);

		unsigned char*					getPixelData() const;
		Format							getFormat() const;
		unsigned int					getWidth() const;
		unsigned int					getHeight() const;
		
		unsigned int					m_iWidth;
		unsigned int					m_iHeight;
		unsigned char*					m_pPixelData;
		Format							m_Format;
	private:
};