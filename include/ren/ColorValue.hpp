#pragma once
#include <cstdint>
#include <type_traits>

namespace ren
{
	template <typename WorkingType>
	inline constexpr WorkingType resizeColorChannel(WorkingType value, size_t inWidth, size_t outWidth)
	{
		static_assert(std::is_unsigned_v<WorkingType>, "Working type must be unsigned.");
		if (outWidth > inWidth) {
			WorkingType result = 0;
			int difference = outWidth - inWidth;
			value <<= difference;
			while (value) {
				result |= value;
				value >>= inWidth;
			}
			return result;
		}
		else {
			return value >> inWidth - outWidth;
		}
	}

	template <size_t blue_width, size_t green_width, size_t red_width, size_t alpha_width, typename WorkingType>
	class ColorValueBGRA;

	template <size_t red_width, size_t green_width, size_t blue_width, size_t alpha_width, typename WorkingType = uint32_t>
	class ColorValueRGBA
	{
	public:
		static_assert(std::is_unsigned_v<WorkingType>, "Working type must be unsigned.");
		static constexpr WorkingType bitmaskAll = -1;
		static constexpr size_t wType_width = sizeof(WorkingType) * 8;
		static constexpr size_t dataWidth = red_width + green_width + blue_width + alpha_width;
		static_assert(dataWidth <= wType_width, "Data width cannot exceed the working type width.");
		static constexpr size_t redPos = 0;
		static constexpr size_t greenPos = redPos + red_width;
		static constexpr size_t bluePos = greenPos + green_width;
		static constexpr size_t alphaPos = bluePos + blue_width;
		static constexpr WorkingType bitmaskRed = bitmaskAll >> wType_width - red_width;
		static constexpr WorkingType bitmaskGreen = bitmaskAll >> wType_width - green_width;
		static constexpr WorkingType bitmaskBlue = bitmaskAll >> wType_width - blue_width;
		static constexpr WorkingType bitmaskAlpha = bitmaskAll >> wType_width - alpha_width;

		constexpr ColorValueRGBA() : p_data(0) {}
		constexpr ColorValueRGBA(const ColorValueRGBA& _other) : p_data(_other.p_data) {}

		constexpr ColorValueRGBA(WorkingType value) : p_data(value) {}
		constexpr ColorValueRGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 0xFF) :
			p_data(compose(red, green, blue, alpha)) {}

		template <size_t red_width_other, size_t green_width_other, size_t blue_width_other, size_t alpha_width_other, typename WorkingType_other>
		constexpr ColorValueRGBA(const ColorValueRGBA<red_width_other, green_width_other, blue_width_other, alpha_width_other, WorkingType_other>& _otherDifferent) {
			dataRed(resizeColorChannel(_otherDifferent.dataRed(), red_width_other, red_width));
			dataGreen(resizeColorChannel(_otherDifferent.dataGreen(), green_width_other, green_width));
			dataBlue(resizeColorChannel(_otherDifferent.dataBlue(), blue_width_other, blue_width));
			dataAlpha(resizeColorChannel(_otherDifferent.dataAlpha(), alpha_width_other, alpha_width));
		}

		constexpr ColorValueRGBA(const ColorValueBGRA<blue_width, green_width, red_width, alpha_width, WorkingType>& _otherDifferent) {
			dataRed(_otherDifferent.dataRed());
			dataGreen(_otherDifferent.dataGreen());
			dataBlue(_otherDifferent.dataBlue());
			dataAlpha(_otherDifferent.dataAlpha());
		}

		template <size_t blue_width_other, size_t green_width_other, size_t red_width_other, size_t alpha_width_other, typename WorkingType_other>
		constexpr ColorValueRGBA(const ColorValueBGRA<blue_width_other, green_width_other, red_width_other, alpha_width_other, WorkingType_other>& _otherDifferent) {
			dataRed(resizeColorChannel(_otherDifferent.dataRed(), red_width_other, red_width));
			dataGreen(resizeColorChannel(_otherDifferent.dataGreen(), green_width_other, green_width));
			dataBlue(resizeColorChannel(_otherDifferent.dataBlue(), blue_width_other, blue_width));
			dataAlpha(resizeColorChannel(_otherDifferent.dataAlpha(), alpha_width_other, alpha_width));
		}
		
		WorkingType data() const {
			return p_data;
		}

		void data(WorkingType value) {
			p_data = value;
		}

		WorkingType dataRed() const {
			return (p_data >> redPos) & bitmaskRed;
		}

		void dataRed(WorkingType value) {
			p_data &= ~(bitmaskRed << redPos);
			p_data |= (value & bitmaskRed) << redPos;
		}

		uint8_t red() const {
			return static_cast<uint8_t>(resizeColorChannel(dataRed(), red_width, 8));
		}

		void red(uint8_t value) {
			dataRed(resizeColorChannel(value, 8, red_width));
		}

		WorkingType dataGreen() const {
			return (p_data >> greenPos) & bitmaskGreen;
		}

		void dataGreen(WorkingType value) {
			p_data &= ~(bitmaskGreen << greenPos);
			p_data |= (value & bitmaskGreen) << greenPos;
		}

		uint8_t green() const {
			return static_cast<uint8_t>(resizeColorChannel(dataGreen(), green_width, 8));
		}

		void green(uint8_t value) {
			dataGreen(resizeColorChannel(value, 8, green_width));
		}

		WorkingType dataBlue() const {
			return (p_data >> bluePos) & bitmaskBlue;
		}

		void dataBlue(WorkingType value) {
			p_data &= ~(bitmaskBlue << bluePos);
			p_data |= (value & bitmaskBlue) << bluePos;
		}

		uint8_t blue() const {
			return static_cast<uint8_t>(resizeColorChannel(dataBlue(), blue_width, 8));
		}

		void blue(uint8_t value) {
			dataBlue(resizeColorChannel(value, 8, blue_width));
		}

		WorkingType dataAlpha() const {
			return (p_data >> alphaPos) & bitmaskAlpha;
		}

		void dataAlpha(WorkingType value) {
			p_data &= ~(bitmaskAlpha << alphaPos);
			p_data |= (value & bitmaskAlpha) << alphaPos;
		}

		uint8_t alpha() const {
			if (alpha_width) {
				return static_cast<uint8_t>(resizeColorChannel(dataAlpha(), alpha_width, 8));
			}
			else {
				return 0xFF;
			}
		}

		void alpha(uint8_t value) {
			if (alpha_width) {
				dataAlpha(resizeColorChannel(value, 8, alpha_width));
			}
		}

		static constexpr WorkingType compose(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
		{
			WorkingType dataRed = resizeColorChannel<WorkingType>(red, 8, red_width);
			WorkingType dataGreen = resizeColorChannel<WorkingType>(green, 8, green_width);
			WorkingType dataBlue = resizeColorChannel<WorkingType>(blue, 8, blue_width);
			WorkingType dataAlpha = resizeColorChannel<WorkingType>(alpha, 8, alpha_width);
			return (dataAlpha << alphaPos) | (dataBlue << bluePos) | (dataGreen << greenPos) | (dataRed << redPos);
		}

	private:
		WorkingType p_data = 0;
	};

	template <size_t blue_width, size_t green_width, size_t red_width, size_t alpha_width, typename WorkingType = uint32_t>
	class ColorValueBGRA
	{
	public:
		static_assert(std::is_unsigned_v<WorkingType>, "Working type must be unsigned.");
		static constexpr WorkingType bitmaskAll = -1;
		static constexpr size_t wType_width = sizeof(WorkingType) * 8;
		static constexpr size_t dataWidth = blue_width + green_width + red_width + alpha_width;
		static_assert(dataWidth <= wType_width, "Data width can not exceed the working type width.");
		static constexpr size_t bluePos = 0;
		static constexpr size_t greenPos = bluePos + blue_width;
		static constexpr size_t redPos = greenPos + green_width;
		static constexpr size_t alphaPos = redPos + red_width;
		static constexpr WorkingType bitmaskBlue = bitmaskAll >> wType_width - blue_width;
		static constexpr WorkingType bitmaskGreen = bitmaskAll >> wType_width - green_width;
		static constexpr WorkingType bitmaskRed = bitmaskAll >> wType_width - red_width;
		static constexpr WorkingType bitmaskAlpha = bitmaskAll >> wType_width - alpha_width;

		constexpr ColorValueBGRA() : p_data(0) {}
		constexpr ColorValueBGRA(const ColorValueBGRA& _other) : p_data(_other.p_data) {}

		constexpr ColorValueBGRA(WorkingType value) : p_data(value) {}
		constexpr ColorValueBGRA(uint8_t blue, uint8_t green, uint8_t red, uint8_t alpha = 0xFF) :
			p_data(compose(blue, green, red, alpha)) {}

		template <size_t blue_width_other, size_t green_width_other, size_t red_width_other, size_t alpha_width_other, typename WorkingType_other>
		constexpr ColorValueBGRA(const ColorValueBGRA<blue_width_other, green_width_other, red_width_other, alpha_width_other, WorkingType_other>& _otherDifferent) {
			dataRed(resizeColorChannel(_otherDifferent.dataRed(), red_width_other, red_width));
			dataGreen(resizeColorChannel(_otherDifferent.dataGreen(), green_width_other, green_width));
			dataBlue(resizeColorChannel(_otherDifferent.dataBlue(), blue_width_other, blue_width));
			dataAlpha(resizeColorChannel(_otherDifferent.dataAlpha(), alpha_width_other, alpha_width));
		}

		constexpr ColorValueBGRA(const ColorValueRGBA<red_width, green_width, blue_width, alpha_width, WorkingType>& _otherDifferent) {
			dataRed(_otherDifferent.dataRed());
			dataGreen(_otherDifferent.dataGreen());
			dataBlue(_otherDifferent.dataBlue());
			dataAlpha(_otherDifferent.dataAlpha());
		}

		template <size_t red_width_other, size_t green_width_other, size_t blue_width_other, size_t alpha_width_other, typename WorkingType_other>
		constexpr ColorValueBGRA(const ColorValueRGBA<red_width_other, green_width_other, blue_width_other, alpha_width_other, WorkingType_other>& _otherDifferent) {
			dataRed(resizeColorChannel(_otherDifferent.dataRed(), red_width_other, red_width));
			dataGreen(resizeColorChannel(_otherDifferent.dataGreen(), green_width_other, green_width));
			dataBlue(resizeColorChannel(_otherDifferent.dataBlue(), blue_width_other, blue_width));
			dataAlpha(resizeColorChannel(_otherDifferent.dataAlpha(), alpha_width_other, alpha_width));
		}

		WorkingType data() const {
			return p_data;
		}

		void data(WorkingType value) {
			p_data = value;
		}

		WorkingType dataRed() const {
			return (p_data >> redPos) & bitmaskRed;
		}

		void dataRed(WorkingType value) {
			p_data &= ~(bitmaskRed << redPos);
			p_data |= (value & bitmaskRed) << redPos;
		}

		uint8_t red() const {
			return static_cast<uint8_t>(resizeColorChannel(dataRed(), red_width, 8));
		}

		void red(uint8_t value) {
			dataRed(resizeColorChannel(value, 8, red_width));
		}

		WorkingType dataGreen() const {
			return (p_data >> greenPos) & bitmaskGreen;
		}

		void dataGreen(WorkingType value) {
			p_data &= ~(bitmaskGreen << greenPos);
			p_data |= (value & bitmaskGreen) << greenPos;
		}

		uint8_t green() const {
			return static_cast<uint8_t>(resizeColorChannel(dataGreen(), green_width, 8));
		}

		void green(uint8_t value) {
			dataGreen(resizeColorChannel(value, 8, green_width));
		}

		WorkingType dataBlue() const {
			return (p_data >> bluePos) & bitmaskBlue;
		}

		void dataBlue(WorkingType value) {
			p_data &= ~(bitmaskBlue << bluePos);
			p_data |= (value & bitmaskBlue) << bluePos;
		}

		uint8_t blue() const {
			return static_cast<uint8_t>(resizeColorChannel(dataBlue(), blue_width, 8));
		}

		void blue(uint8_t value) {
			dataBlue(resizeColorChannel(value, 8, blue_width));
		}

		WorkingType dataAlpha() const {
			return (p_data >> alphaPos) & bitmaskAlpha;
		}

		void dataAlpha(WorkingType value) {
			p_data &= ~(bitmaskAlpha << alphaPos);
			p_data |= (value & bitmaskAlpha) << alphaPos;
		}

		uint8_t alpha() const {
			if (alpha_width) {
				return static_cast<uint8_t>(resizeColorChannel(dataAlpha(), alpha_width, 8));
			}
			else {
				return 0xFF;
			}
		}

		void alpha(uint8_t value) {
			if (alpha_width) {
				dataAlpha(resizeColorChannel(value, 8, alpha_width));
			}
		}

		static constexpr WorkingType compose(uint8_t blue, uint8_t green, uint8_t red, uint8_t alpha)
		{
			WorkingType dataBlue = resizeColorChannel<WorkingType>(blue, 8, blue_width);
			WorkingType dataGreen = resizeColorChannel<WorkingType>(green, 8, green_width);
			WorkingType dataRed = resizeColorChannel<WorkingType>(red, 8, red_width);
			WorkingType dataAlpha = resizeColorChannel<WorkingType>(alpha, 8, alpha_width);
			return (dataAlpha << alphaPos) | (dataBlue << bluePos) | (dataGreen << greenPos) | (dataRed << redPos);
		}

	private:
		WorkingType p_data = 0;
	};

	typedef ColorValueRGBA<8, 8, 8, 8, uint32_t> ColorRGBA;
	typedef ColorValueBGRA<8, 8, 8, 8, uint32_t> ColorBGRA;
}