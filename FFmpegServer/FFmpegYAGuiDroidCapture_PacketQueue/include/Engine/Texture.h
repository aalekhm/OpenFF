#ifndef TEXTURE_H
#define TEXTURE_H
#include <string>

class Image;

class STBImgType
{
	public:
		enum class Type
		{
			BMP = 0,
			TGA,
			PNG,
			JPG,
			HDR,
			MAX
		};

		static std::string ToString(STBImgType::Type eSTBImgType)
		{
			std::string sImageType;
			switch(eSTBImgType)
			{
				case STBImgType::Type::BMP: sImageType = "bmp"; break;
				case STBImgType::Type::TGA: sImageType = "tga"; break;
				case STBImgType::Type::PNG: sImageType = "png"; break;
				case STBImgType::Type::JPG: sImageType = "jpg"; break;
				case STBImgType::Type::HDR: sImageType = "hdr"; break;
			}

			return sImageType;
		}

		static STBImgType::Type FromString(std::string sSTBImgType)
		{
			STBImgType::Type eSTBImgType = STBImgType::Type::BMP;

			if(sSTBImgType == "bmp") eSTBImgType = STBImgType::Type::BMP;
			else if(sSTBImgType == "tga") eSTBImgType = STBImgType::Type::TGA;
			else if(sSTBImgType == "png") eSTBImgType = STBImgType::Type::PNG;
			else if(sSTBImgType == "jpg") eSTBImgType = STBImgType::Type::JPG;
			else if(sSTBImgType == "hdr") eSTBImgType = STBImgType::Type::HDR;

			return eSTBImgType;
		}
};

class Texture {
	friend class Sampler;

	public:
		/**
		 * Defines the set of supported texture formats.
		 */
		enum Format
		{
			UNKNOWN = 0,
			RGB     = GL_RGB,
			RGBA    = GL_RGBA,
			ALPHA   = GL_ALPHA
		};

		enum STBImgFormat
		{
			RGB_3 = 3,
			RGBA_4 = 4
		};

		/**
		 * Defines the set of supported texture filters.
		 */
		enum Filter
		{
			NEAREST = GL_NEAREST,
			LINEAR = GL_LINEAR,
			NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
			LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
			NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
			LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR
		};

		/**
		 * Defines the set of supported texture wrapping modes.
		 */
		enum Wrap
		{
			REPEAT = GL_REPEAT,
			CLAMP = GL_CLAMP,
			//CLAMP_TO_EGDE = GL_CLAMP_TO_EDGE
		};

		enum Type
		{
			TEXTURE_2D = GL_TEXTURE_2D
		};

		/**
		* Destructor.
		*/
		virtual ~Texture();

		static Texture* create(const char* path, bool generateMipmaps = false);
		static Texture* createEx(const char* path, bool generateMipmaps = false);
		static Texture* create(Image* image, bool generateMipmaps = false);
		static Texture* create(Format format, unsigned int width, unsigned int height, unsigned char* data, bool generateMipmaps = false, Type type = TEXTURE_2D);
		static Texture* create(GLuint handle, int width, int height, Format format = UNKNOWN);

		static Texture* createTexture(const char* path, bool generateMipmaps);
		static Texture*	createTGA(const char* path, bool generateMipmaps);

		static void saveToDisk(std::string sPath, STBImgType::Type eSTBImgType, int32_t iWidth, int32_t iHeight, STBImgFormat eSTBImgFormat, void* pRawData);

		Format getFormat() const;
		unsigned int getWidth() const;
		unsigned int getHeight() const;
		void generateMipmaps();
		bool isMipmapped() const;
		bool isCompressed() const;
		GLuint getHandle() const;
		Type getType() const;

		void setWrapMode(Wrap wrapS, Wrap wrapT);
		void setFilterMode(Filter minificationFilter, Filter magnificationFilter);

		void bind();
		void unbind();
		GLuint			m_hTexture;

		/**
		 * Defines a texture sampler.
		 *
		 * A texture sampler is basically an instance of a texture that can be
		 * used to sample a texture from a material. In addition to the texture
		 * itself, a sampler stores per-instance texture state information, such
		 * as wrap and filter modes.
		 */
		class Sampler {
			friend class Texture;

			public:
				virtual ~Sampler();
				static Sampler* create(Texture* texture);
				static Sampler* create(const char* sPath, bool bGenerateMipmaps = false);
				void			setWrapMode(Wrap wrapS, Wrap wrapT);
				void			setFilterMode(Filter minificationFilter, Filter magnificationFilter);
				Texture*		getTexture() const;
				void			bind();
				void			unbind();
			private:
				Sampler(Texture* texture);

				Texture*	m_pTexture;
				Wrap		m_WrapS;
				Wrap		m_WrapT;
				Filter		m_minFilter;
				Filter		m_magFilter;
		};
	private:
		/**
		* Constructor.
		*/
		Texture();

		/**
		* Copy constructor.
		*/
		//Texture(const &Texture copy);

		/**
		* Hidden copy assignment operator.
		*/
		Texture& operator=(const Texture&);
		
		std::string		m_sPath;
		
		Format			m_Format;
		unsigned int	m_iWidth;
		unsigned int	m_iHeight;
		bool			m_bMipmapped;
		bool			m_bCached;
		bool			m_bCompressed;
		Type			m_eType;
};

#endif