#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <ren/FileHandle.hpp>
#include <ren/Exceptions.hpp>
#include <ren/ColorValue.hpp>
#include <algorithm>

namespace ren
{

	template <size_t _width, size_t _height>
	struct RasterBlock
	{
		static constexpr size_t width = _width;
		static constexpr size_t height = _height;

		void putPixel(size_t element, size_t row, uint32_t value) {
			pixels[row * width + element] = value;
		}

		uint32_t getPixel(size_t element, size_t row) const {
			return pixels[row * width + element];
		}

		uint32_t pixels[_width * _height];
	};

	inline void expand8BitIndexed(void* _buffer, size_t _bufferSize, const void* _src, size_t _srcSize, const void* _palette, size_t _entryCount, size_t _entrySize) {
		size_t targetCount = std::min(_bufferSize / _entrySize, _srcSize);
		char* bufferPtr = reinterpret_cast<char*>(_buffer);
		const char* sourcePtr = reinterpret_cast<const char*>(_src);
		const uint8_t* palettePtr = reinterpret_cast<const uint8_t*>(_palette);
		const uint32_t missingColor = 0xFF00FF00;

		for (size_t pixelIndex = 0; pixelIndex < targetCount; pixelIndex++) {
			uint8_t colorIndex = sourcePtr[pixelIndex];
			if (colorIndex < _entryCount) {
				std::memcpy(
					bufferPtr + pixelIndex * _entrySize,
					palettePtr + _entrySize * colorIndex,
					_entrySize
				);
			}
			else {
				std::memcpy(
					bufferPtr + pixelIndex * _entrySize,
					&missingColor,
					_entrySize
				);	
			}
		}
	}

	inline void expand4BitIndexed(void* _buffer, size_t _bufferSize, const void* _src, size_t _srcSize, const void* _palette, size_t _entryCount, size_t _entrySize) {
		size_t targetCount = std::min((_bufferSize / _entrySize) / 2, _srcSize);
		char* bufferPtr = reinterpret_cast<char*>(_buffer);
		const char* sourcePtr = reinterpret_cast<const char*>(_src);
		const uint8_t* palettePtr = reinterpret_cast<const uint8_t*>(_palette);
		const uint32_t missingColor = 0xFF00FF00;

		for (size_t doublePixelIndex = 0; doublePixelIndex < targetCount; doublePixelIndex++) {
			uint8_t colorIndex0 = sourcePtr[doublePixelIndex] & 0xF;
			uint8_t colorIndex1 = (sourcePtr[doublePixelIndex] >> 4) & 0xF;
			if (colorIndex0 < _entryCount) {
				std::memcpy(
					bufferPtr + (doublePixelIndex * 2) * _entrySize,
					palettePtr + _entrySize * colorIndex0,
					_entrySize
				);
			}
			else {
				std::memcpy(
					bufferPtr + (doublePixelIndex * 2) * _entrySize,
					&missingColor,
					_entrySize
				);
			}
			if (colorIndex1 < _entryCount) {
				std::memcpy(
					bufferPtr + (doublePixelIndex * 2 + 1) * _entrySize,
					palettePtr + _entrySize * colorIndex1,
					_entrySize
				);
			}
			else {
				std::memcpy(
					bufferPtr + (doublePixelIndex * 2 + 1) * _entrySize,
					&missingColor,
					_entrySize
				);
			}
		}
	}

	struct ImageData
	{
		static constexpr int ORDERING_BLUEFIRST = 0;
		static constexpr int ORDERING_REDFIRST = 1;
		static constexpr int ORDERING_BOTTOMFIRST = 0;
		static constexpr int ORDERING_TOPFIRST = 1;

		ImageData(int _width, int _height, int _channels, int _orderingColor = ORDERING_BLUEFIRST, int _orderingRows = ORDERING_BOTTOMFIRST) : 
			width(_width), height(_height), channels(_channels), 
			pixels(_width*_height*_channels), orderingColor(_orderingColor), orderingRows(_orderingRows) {}

		void putPixel(size_t element, size_t row, uint32_t value);
		void putPixelR(size_t element, size_t row, ColorRGBA value);
		void putPixelB(size_t element, size_t row, ColorBGRA value);
		uint32_t getPixel(size_t element, size_t row) const;
		ColorRGBA getPixelR(size_t element, size_t row) const;
		ColorBGRA getPixelB(size_t element, size_t row) const;
		char* getRow(size_t row);
		const char* getRow(size_t row) const;
		int rowSize() const;

		static ImageData invertRowOrdering(const ImageData& _original);
		static ImageData keyToAlpha(const ImageData &_original, uint32_t _key);
		static ImageData invertColorOrderding(const ImageData& _original);

		std::vector<char> pixels;
		int width, height;
		int channels;
		int orderingColor;
		int orderingRows;
	};

	inline void ImageData::putPixel(size_t element, size_t row, uint32_t value)
	{
		size_t index = row * width + element;
		std::memcpy(&pixels[index*channels], &value, channels);
	}

	inline void ImageData::putPixelR(size_t element, size_t row, ColorRGBA value) 
	{
		uint32_t orderedValue = (ORDERING_REDFIRST) ? value.data() : ColorBGRA(value).data();
		putPixel(element, row, orderedValue);
	}

	inline void ImageData::putPixelB(size_t element, size_t row, ColorBGRA value)
	{
		uint32_t orderedValue = (ORDERING_BLUEFIRST) ? value.data() : ColorRGBA(value).data();
		putPixel(element, row, orderedValue);
	}

	inline uint32_t ImageData::getPixel(size_t element, size_t row) const
	{
		size_t index = row * width + element;
		uint32_t value = 0;
		std::memcpy(&value, &pixels[index*channels], channels);
		return value;
	}

	inline ColorRGBA ImageData::getPixelR(size_t element, size_t row) const
	{
		ColorRGBA result;
		if (ORDERING_REDFIRST) {
			result = ColorRGBA(getPixel(element, row));
		}
		else {
			result = ColorBGRA(getPixel(element, row));
		}
		if (channels < 4) {
			result.dataAlpha(0xFF);
		}
		return result;
	}

	inline ColorBGRA ImageData::getPixelB(size_t element, size_t row) const
	{
		ColorBGRA result;
		if (ORDERING_BLUEFIRST) {
			result = ColorBGRA(getPixel(element, row));
		}
		else {
			result = ColorRGBA(getPixel(element, row));
		}
		if (channels < 4) {
			result.dataAlpha(0xFF);
		}
		return result;
	}

	inline char* ImageData::getRow(size_t row)
	{
		return &pixels[row*rowSize()];
	}

	inline const char* ImageData::getRow(size_t row) const
	{
		return &pixels[row*rowSize()];
	}

	inline int ImageData::rowSize() const
	{
		return width * channels;
	}

	inline ImageData ImageData::invertRowOrdering(const ImageData& _original)
	{
		ImageData result = {_original.width, _original.height, _original.channels, !_original.orderingColor};
		for (int row = 0; row < result.height; row++) {
			int lastRow = result.height - 1;
			std::memcpy(result.getRow(row), _original.getRow(lastRow-row), result.rowSize());
		}
		return result;
	}

	inline ImageData ImageData::keyToAlpha(const ImageData &_original, uint32_t _key)
	{
		if (_original.channels != 3 && _original.channels != 4) {
			return _original;
		}
		_key &= 0xFFFFFF;
		ImageData result = {_original.width, _original.height, 4, _original.orderingColor};
		for (int row = 0; row < result.height; row++) {
			for (int element = 0; element < result.width; element++) {
				uint32_t pixel = _original.getPixel(element, row);
				if ((pixel & 0xFFFFFF) == _key) {
					result.putPixel(element, row, pixel & 0xFFFFFF);
				}
				else {
					result.putPixel(element, row, 0xFF000000 | pixel);
				}
			}
		}
		return result;
	}

	inline ImageData ImageData::invertColorOrderding(const ImageData& _original)
	{
		ImageData result = {_original.width, _original.height, _original.channels, (_original.orderingColor == ORDERING_REDFIRST) ? ORDERING_BLUEFIRST : ORDERING_REDFIRST};
		for (int row = 0; row < result.height; row++) {
			for (int element = 0; element < result.width; element++) {
				uint32_t color = _original.getPixel(element,row);
				uint32_t newUpper = (color << 16) & 0xFF0000;
				uint32_t newLower = (color >> 16) & 0xFF;
				color &= 0xFF00FF00;
				result.putPixel(element,row, color | newUpper | newLower);
			}
		}
		return result;
	}

	class ImageDecoder
	{
	public:
		ImageDecoder();
		ImageDecoder(const FileInputHandle *handle);

		ImageData decode();
		void setHandle(const FileInputHandle* _handle);

		// target format will always be 8 bit per channel

		typedef bool(*GetDecoderFunction)(FileInputHandle*, ImageDecoder**);
		typedef bool(*GetNameFunction)(const char**);

	protected:
		virtual ImageData p_decode() = 0;
		const FileInputHandle *p_handle;
	};

	inline ImageDecoder::ImageDecoder() : p_handle(nullptr) {}

	inline ImageDecoder::ImageDecoder(const FileInputHandle *handle) : p_handle(handle) {}

	inline ImageData ImageDecoder::decode() 
	{
		if (p_handle) {
			return p_decode();
		}
		else {
			throwException<std::logic_error>("%s() Decoder object has no file handle assigned!", __FUNCTION__);
		}
	}

	inline void ImageDecoder::setHandle(const FileInputHandle* _handle)
	{
		p_handle = _handle;
	}

	class ImageEncoder
	{
	public:
		ImageEncoder();
		ImageEncoder(FileOutputHandle *_handle);

		void encode(const ImageData& _image);
		void setHandle(FileOutputHandle* _handle);

		typedef bool(*GetEncoderFunction)(FileOutputHandle*, ImageEncoder**);
		typedef bool(*GetNameFunction)(const char**);

	protected:
		virtual void p_encode(const ImageData& _image) = 0;
		FileOutputHandle* p_handle;
	};

	inline ImageEncoder::ImageEncoder() : p_handle(nullptr) {}

	inline ImageEncoder::ImageEncoder(FileOutputHandle* _handle) : p_handle(_handle) {}

	inline void ImageEncoder::encode(const ImageData& _image)
	{
		if (p_handle) {
			p_encode(_image);
		}
		else {
			throwException<std::logic_error>("%s() Encoder object has no file handle assigned!", __FUNCTION__);
		}
	}

	inline void ImageEncoder::setHandle(FileOutputHandle* _handle)
	{
		p_handle = _handle;
	}

	namespace TGA
	{
		constexpr uint8_t ITYPE_NODATA = 0;
		constexpr uint8_t ITYPE_COLORMAPPED = 1;
		constexpr uint8_t ITYPE_TRUECOLOR = 2;
		constexpr uint8_t ITYPE_GREYSCALE = 3;
		constexpr uint8_t ITYPE_RLE_FALSE = 0;
		constexpr uint8_t ITYPE_RLE_TRUE = 1;
		constexpr uint8_t IDESC_LEFT_TO_RIGHT = 0;
		constexpr uint8_t IDESC_RIGHT_TO_LEFT = 1;
		constexpr uint8_t IDESC_BOTTOM_TO_TOP = 0;
		constexpr uint8_t IDESC_TOP_TO_BOTTOM = 1;

		struct Header
		{
			uint8_t imageIDSize;
			uint8_t colorMapType;
			struct
			{
				uint8_t colorModel : 3;
				uint8_t rle : 1;
			} imageType;
			uint8_t colorMapSpec[5];
			struct
			{
				uint16_t originX;
				uint16_t originY;
				uint16_t width;
				uint16_t height;
				uint8_t bpp;
				struct
				{
					uint8_t alphaChannelDepth : 4;
					uint8_t pixelOrderingHorizontal : 1;
					uint8_t pixelOrderingVertical : 1;
				} imageDesc;
			} imageSpec;
		};

		struct ColorMapSpec
		{
			uint16_t firstEntryIndex;
			uint16_t colorMapSize;
			uint8_t singleEntrySize;
		};

		class Decoder : public ImageDecoder
		{
		public:
			Decoder() = delete;
			Decoder(const FileInputHandle *handle);

		protected:
			virtual ImageData p_decode() override;
		};

		inline Decoder::Decoder(const FileInputHandle* handle) : 
			ImageDecoder(handle) {}

		inline ImageData Decoder::p_decode() 
		{
			Header header;
			if (p_handle->filesize() < sizeof(header)) {
				throwException<std::runtime_error>("%s(): Invalid TGA file! (Filesize too small: %d)", __FUNCTION__, (int)p_handle->filesize());
			}
			p_handle->read(0, reinterpret_cast<char*>(&header), sizeof(header));
			ColorMapSpec cmapSpec = {};
			std::memcpy(&cmapSpec, &header.colorMapSpec, sizeof(header.colorMapSpec));
			if (header.imageSpec.bpp != 24 && header.imageSpec.bpp != 32) {
				throwException<std::runtime_error>("%s(): Unsupported bpp value found! (%d)", __FUNCTION__, (int)header.imageSpec.bpp);
			}
			if ((header.imageSpec.bpp == 32 && header.imageSpec.imageDesc.alphaChannelDepth != 8) || 
					(header.imageSpec.bpp == 24 && header.imageSpec.imageDesc.alphaChannelDepth != 0)) {
				throwException<std::runtime_error>("%s(): Invalid alpha channel depth! (%d)", __FUNCTION__, (int)header.imageSpec.imageDesc.alphaChannelDepth);
			}
			if (header.imageType.colorModel != ITYPE_TRUECOLOR) {
				throwException<std::runtime_error>("%s(): Invalid color model! (%d)", __FUNCTION__, (int)header.imageType.colorModel);
			}
			if (header.imageType.rle == ITYPE_RLE_TRUE) {
				throwException<std::runtime_error>("%s(): RLE encoding is unsupported!", __FUNCTION__);
			}

			int dataOffset = sizeof(Header) + header.imageIDSize + cmapSpec.colorMapSize;
			int calculatedDataSize = header.imageSpec.width * header.imageSpec.height * (header.imageSpec.bpp / 8);
			if (p_handle->filesize() - dataOffset < calculatedDataSize) {
				throwException<std::runtime_error>("%s(): Invalid image data! (file too short)", __FUNCTION__);
			}

			ImageData imageData = {header.imageSpec.width, header.imageSpec.height, header.imageSpec.bpp / 8};
			p_handle->read(dataOffset, imageData.pixels.data(), imageData.pixels.size());
			return imageData;
		}

		class Encoder : public ImageEncoder
		{
		public:
			Encoder() = delete;
			Encoder(FileOutputHandle* _handle, size_t _channels = 4);

			

		protected:
			void p_encode(const ImageData& _image) override;
			size_t p_channels;
		};

		inline Encoder::Encoder(FileOutputHandle* _handle, size_t _channels) : 
			ImageEncoder(_handle)
		{
			if (_channels == 3 || _channels == 4) {
				p_channels = _channels;
			}
			else {
				throwException<std::logic_error>("%s() Invalid number of channels!", __FUNCTION__);
			}
		}

		inline void Encoder::p_encode(const ImageData& _image)
		{
			Header header = {};
			header.imageIDSize = 0;
			header.colorMapType = 0;
			header.imageType.colorModel = ITYPE_TRUECOLOR;
			header.imageType.rle = ITYPE_RLE_FALSE;
			header.imageSpec.originX = 0;
			header.imageSpec.originY = 0;
			header.imageSpec.width = _image.width;
			header.imageSpec.height = _image.height;
			header.imageSpec.bpp = p_channels * 8;
			if (p_channels == 4) {
				header.imageSpec.imageDesc.alphaChannelDepth = 8;
			}
			header.imageSpec.imageDesc.pixelOrderingHorizontal = IDESC_LEFT_TO_RIGHT;
			header.imageSpec.imageDesc.pixelOrderingVertical = IDESC_BOTTOM_TO_TOP;

			p_handle->write(0, reinterpret_cast<char*>(&header), sizeof(header));
			if (_image.channels == p_channels && _image.orderingColor == _image.ORDERING_BLUEFIRST) {
				p_handle->write(_image.pixels.data(), _image.pixels.size());
			}
			else {
				ImageData output = {_image.width, _image.height, (int)p_channels, _image.ORDERING_BLUEFIRST};
				for (size_t rowIndex = 0; rowIndex < _image.height; rowIndex++) {
					for (size_t elementIndex = 0; elementIndex < _image.width; elementIndex++) {
						output.putPixelB(elementIndex, rowIndex, _image.getPixelB(elementIndex, rowIndex));
					}
				}
				p_handle->write(output.pixels.data(), output.pixels.size());
			}
		}
	}

	namespace BMP
	{
		constexpr uint16_t signature = 0x4D42;

		struct FileHeader 
		{
			uint32_t filesize;
			uint16_t reserved1;
			uint16_t reserved2;
			uint32_t dataOffset;
		};

		constexpr uint32_t BM_RGB = 0;
		constexpr uint32_t BM_RLE8 = 1;
		constexpr uint32_t BM_RLE4 = 2;

		struct BitmapInfoHeader
		{
			uint32_t headerSize;
			int32_t width;
			int32_t height;
			uint16_t colorPlaneCount; // always 1
			uint16_t bpp;
			uint32_t compression;
			uint32_t imageSize; // can be 0 for uncompressed images
			int32_t resHorizontal; // px/m
			int32_t resVertical; // px/m
			uint32_t paletteEntryCount; // can be 0
			uint32_t importantColorCount; // mostly 0, generally ignored
		};

		// no palette support for now!

		// each row of pixels is padded to full 4 bytes

		class Decoder : public ImageDecoder
		{
		public:
			Decoder() = delete;
			Decoder(const FileInputHandle *handle);
		
		protected:
			virtual ImageData p_decode() override;
		};

		inline Decoder::Decoder(const FileInputHandle* handle) : 
			ImageDecoder(handle) {}

		inline ImageData Decoder::p_decode() 
		{
			uint16_t l_sig;
			if (p_handle->filesize() < (sizeof(l_sig) + sizeof(FileHeader) + sizeof(BitmapInfoHeader))) {
				throwException<std::runtime_error>("%s(): Invalid BMP file! (Filesize too small: %d)", __FUNCTION__, (int)p_handle->filesize());
			}
			p_handle->read(0, reinterpret_cast<char*>(&l_sig), sizeof(l_sig));
			if (l_sig != signature) {
				throwException<std::runtime_error>("%s(): No Windows bitmap file! (wrong signature)", __FUNCTION__);
			}
			FileHeader fileHeader;
			p_handle->read(sizeof(signature), reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
			if (fileHeader.filesize != p_handle->filesize()) {
				throwException<std::runtime_error>("%s(): Invalid BMP file! (invalid filesize)", __FUNCTION__);
			}
			BitmapInfoHeader infoHeader;
			p_handle->read(sizeof(signature) + sizeof(FileHeader), reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
			if (infoHeader.bpp != 24) {
				throwException<std::runtime_error>("%s(): Unsupported color depth! (%d)", __FUNCTION__, (int)infoHeader.bpp);
			}
			if (infoHeader.compression != BM_RGB) {
				throwException<std::runtime_error>("%s(): Compressed images are unsupported!", __FUNCTION__);
			}
			int channelCount = infoHeader.bpp / 8;
			int rowSizePacked = infoHeader.width * channelCount;
			int rowSizeFull = (rowSizePacked % sizeof(uint32_t)) ? (rowSizePacked / sizeof(uint32_t) + 1) * sizeof(uint32_t) : rowSizePacked;
			if (p_handle->filesize() - fileHeader.dataOffset < rowSizeFull*infoHeader.height) {
				throwException<std::runtime_error>("%s(): Pixel array too short!", __FUNCTION__);
			}
			std::vector<char> rawData(rowSizeFull*infoHeader.height);
			ImageData imageData = {infoHeader.width, infoHeader.height, channelCount};
			p_handle->read(fileHeader.dataOffset, rawData.data(), rawData.size());
			for (int row = 0; row < infoHeader.height; row++) {
				std::memcpy(imageData.getRow(row), rawData.data() + rowSizeFull*row, rowSizePacked);
			}
			return imageData;
		}

		class Encoder : public ImageEncoder
		{
		public:
			Encoder() = delete;
			Encoder(FileOutputHandle* _handle);

		protected:
			void p_encode(const ImageData& _image) override;
		};

		inline Encoder::Encoder(FileOutputHandle* _handle) : 
			ImageEncoder(_handle) {}

		inline void Encoder::p_encode(const ImageData& _image)
		{
			constexpr char padding[2] = {0, 0};
			constexpr size_t dataOffset = sizeof(signature) + sizeof(FileHeader) + sizeof(BitmapInfoHeader) + sizeof(padding);
			size_t rowSize = ((_image.width * 3) % 4) ? ((_image.width * 3) / 4 + 1) * 4 : _image.width * 3;
			std::vector<char> rowBuffer(rowSize, 0);

			FileHeader fHeader = {};
			fHeader.filesize = dataOffset + rowSize * _image.height;
			fHeader.dataOffset = dataOffset;

			BitmapInfoHeader infoHeader = {};
			infoHeader.headerSize = sizeof(infoHeader);
			infoHeader.width = _image.width;
			infoHeader.height = _image.height;
			infoHeader.colorPlaneCount = 1;
			infoHeader.bpp = 24;
			infoHeader.compression = BM_RGB;
			infoHeader.imageSize = rowSize * infoHeader.height;
			infoHeader.resHorizontal = 2835;
			infoHeader.resVertical = 2835;
			infoHeader.paletteEntryCount = 0;
			infoHeader.importantColorCount = 0;

			p_handle->write(reinterpret_cast<const char*>(&signature), sizeof(signature));
			p_handle->write(reinterpret_cast<const char*>(&fHeader), sizeof(fHeader));
			p_handle->write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
			p_handle->write(padding, sizeof(padding));

			if (_image.channels == 3 && _image.orderingColor == ImageData::ORDERING_BLUEFIRST) {
				for (size_t rowIndex = 0; rowIndex < _image.height; rowIndex++) {
					std::memcpy(rowBuffer.data(), _image.getRow(rowIndex), _image.rowSize());
					p_handle->write(rowBuffer.data(), rowBuffer.size());
				}
			}
			else {
				for (size_t rowIndex = 0; rowIndex < _image.height; rowIndex++) {
					for (size_t elementIndex = 0; elementIndex < _image.width; elementIndex++) {
						ColorBGRA colorBuffer = _image.getPixelB(elementIndex, rowIndex);
						std::memcpy(rowBuffer.data() + elementIndex * 3, &colorBuffer, 3);
					}
					p_handle->write(rowBuffer.data(), rowBuffer.size());
				}
			}
		}
	}

	namespace PCX
	{
		constexpr uint8_t SIGNATURE = 0x0A;
		constexpr uint8_t VERSION_2_5 = 0; // fixed EGA
		constexpr uint8_t VERSION_2_8_PAL = 2; // modifiable EGA
		constexpr uint8_t VERSION_2_8_NOPAL = 3;
		constexpr uint8_t VERSION_WIN = 4;
		constexpr uint8_t VERSION_3_0 = 5; // including 24-bit images
		constexpr uint8_t ENCODING_NONE = 0; // mostly unsupported
		constexpr uint8_t ENCODING_RLE = 1;
		constexpr uint16_t PALMODE_DEFAULT = 0;
		constexpr uint16_t PALMODE_MONO_COLOR = 1;
		constexpr uint16_t PALMODE_GREYSCALE = 2;
		constexpr uint8_t RLE_MARKER = 0x80 | 0x40;
		constexpr size_t VGA_PALETTE_SIZE = 0x300; // 256 colors a 3 bytes
		// image data starts right after the header
		// VGA palette is at the end of the file

		struct PaletteEntry
		{
			uint8_t red;
			uint8_t green;
			uint8_t blue;
		};

		inline bool isRleByte(uint8_t _value) 
		{
			return _value >= RLE_MARKER;
		}

		inline constexpr int runLength(uint8_t _rleByte)
		{
			return _rleByte & ~RLE_MARKER;
		}

		// returns number of bytes written
		inline int scanRleAndPut(uint8_t* _dest, const uint8_t* _src) 
		{
			int l_runLength = runLength(*_src);
			int value = (*_src+1); 						// might be buggy
			std::memset(_dest, value, l_runLength);
			return l_runLength;
		}

		struct Header
		{
			uint8_t identifier; // always 0x0A
			uint8_t version;
			uint8_t encoding;
			uint8_t bitsPerColorplane; // 1, 2, 4 or 8
			uint16_t minX;
			uint16_t minY;
			uint16_t maxX;
			uint16_t maxY;
			uint16_t dpiHorizontal;
			uint16_t dpiVertical;
			uint8_t egaPalette[48]; // 16 colors
			uint8_t userField1;
			uint8_t colorPlaneCount;
			uint16_t bytesPerColorplaneScanline;
			uint16_t paletteMode;
			uint16_t sourceDpiHorizontal;
			uint16_t sourceDpiVertical;
			uint8_t userField2[54];
		};

		class Decoder : public ImageDecoder
		{
		public:
			Decoder() = delete;
			Decoder(const FileInputHandle *handle);
			
		protected:
			virtual ImageData p_decode() override;
		};

		inline Decoder::Decoder(const FileInputHandle* handle) : 
			ImageDecoder(handle) {}

		inline ImageData Decoder::p_decode() 
		{
			Header header;
			if (p_handle->filesize() < sizeof(header)) {
				throwException<std::runtime_error>("%s(): Invalid PCX file! (Filesize too small: %d)", __FUNCTION__, (int)p_handle->filesize());
			}
			p_handle->read(0, reinterpret_cast<char*>(&header), sizeof(header));
			if (header.identifier != SIGNATURE) {
				throwException<std::runtime_error>("%s(): Not a Paintbrush PCX file! (Invalid signature)", __FUNCTION__);
			}
			if (header.version != VERSION_3_0) {
				throwException<std::runtime_error>("%s(): Unsupported Paintbrush PCX version!", __FUNCTION__);
			}
			int width = (header.maxX + 1) - header.minX;
			int height = (header.maxY + 1) - header.minY;
			if (width <= 0 || height <= 0) {
				throwException<std::runtime_error>("%s(): Invalid width and/or height! (%d, %d)", __FUNCTION__, width, height);
			}
			if (header.bitsPerColorplane != 8 || header.colorPlaneCount != 1) {
				throwException<std::runtime_error>("%s(): Invalid BitsPerColorplane and/or ColorPlaneCount setting! (%d, %d)", __FUNCTION__, 
					(int)header.bitsPerColorplane, (int)header.colorPlaneCount);
			}
			
			std::vector<uint8_t> fileRemainder(p_handle->filesize() - sizeof(Header));
			p_handle->read(sizeof(Header), reinterpret_cast<char*>(fileRemainder.data()), fileRemainder.size());
			std::vector<uint8_t> decodedColorplane(width*height);

			int bytesRead = 0;
			int bytesWritten = 0;
			while (bytesWritten < decodedColorplane.size()) {
				if (isRleByte(fileRemainder[bytesRead])) {
					bytesWritten += scanRleAndPut(&decodedColorplane[bytesWritten], &fileRemainder[bytesRead]);
					bytesRead += 2;
				}
				else {
					decodedColorplane[bytesWritten] = fileRemainder[bytesRead];
					bytesWritten++;
					bytesRead++;
				}
			}
			
			int paletteSize = p_handle->filesize() - sizeof(Header) - bytesRead;
			PaletteEntry palette[256];
			if (paletteSize < sizeof(palette)) {
				throwException<std::runtime_error>("%s(): Palette size too small! (%d bytes)", __FUNCTION__, paletteSize);
			}
			p_handle->read(p_handle->filesize() - sizeof(palette), reinterpret_cast<char*>(&palette[0]), sizeof(palette));

			ImageData imageData = {width, height, 3, ImageData::ORDERING_REDFIRST};
			for (int currentPixel = 0; currentPixel < decodedColorplane.size(); currentPixel++) {
				std::memcpy(&imageData.pixels[currentPixel*3], &palette[decodedColorplane[currentPixel]], 3);
			}

			return imageData;
		}
	}

	namespace DXT
	{
		// S3 compression stores 4 x 4 blocks of pixels
		// 16 bit color is 5:6:5

		typedef RasterBlock<4,4> DXTBlock;
		typedef ColorValueRGBA<5,6,5,0,uint16_t> DXTColor;

		struct V1Row
		{
			uint8_t e0 : 2;
			uint8_t e1 : 2;
			uint8_t e2 : 2;
			uint8_t e3 : 2;
		};

		struct V1Block
		{
			DXTColor c[2];
			V1Row rows[4];
		};

		struct V3Block
		{

		};

		inline DXTBlock decodeBlock(const V1Block* _src) 
		{
			ColorValueRGBA<8,8,8,8> colorsDecoded[4];
			colorsDecoded[0] = _src->c[0];
			colorsDecoded[1] = _src->c[1];
			if (_src->c[0].data() > _src->c[1].data()) {
				colorsDecoded[2].dataRed((colorsDecoded[0].dataRed() * 2 + colorsDecoded[1].dataRed()) / 3);
				colorsDecoded[2].dataGreen((colorsDecoded[0].dataGreen() * 2 + colorsDecoded[1].dataGreen()) / 3);
				colorsDecoded[2].dataBlue((colorsDecoded[0].dataBlue() * 2 + colorsDecoded[1].dataBlue()) / 3);
				colorsDecoded[2].alpha(0xFF);

				colorsDecoded[3].dataRed((colorsDecoded[0].dataRed() + colorsDecoded[1].dataRed() * 2) / 3);
				colorsDecoded[3].dataGreen((colorsDecoded[0].dataGreen() + colorsDecoded[1].dataGreen() * 2) / 3);
				colorsDecoded[3].dataBlue((colorsDecoded[0].dataBlue() + colorsDecoded[1].dataBlue() * 2) / 3);
				colorsDecoded[3].alpha(0xFF);
			}
			else {
				colorsDecoded[2].dataRed((colorsDecoded[0].dataRed()+ colorsDecoded[1].dataRed()) / 2);
				colorsDecoded[2].dataGreen((colorsDecoded[0].dataGreen()+ colorsDecoded[1].dataGreen()) / 2);
				colorsDecoded[2].dataBlue((colorsDecoded[0].dataBlue()+ colorsDecoded[1].dataBlue()) / 2);
				colorsDecoded[2].alpha(0xFF);

				colorsDecoded[3] = 0;
			}

			DXTBlock result;
			for (size_t rowIndex = 0; rowIndex < result.height; rowIndex++) {
				result.putPixel(0, rowIndex, colorsDecoded[_src->rows[rowIndex].e0].data());
				result.putPixel(1, rowIndex, colorsDecoded[_src->rows[rowIndex].e1].data());
				result.putPixel(2, rowIndex, colorsDecoded[_src->rows[rowIndex].e2].data());
				result.putPixel(3, rowIndex, colorsDecoded[_src->rows[rowIndex].e3].data());
			}

			return result;
		}
	}
}