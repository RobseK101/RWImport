#ifndef REN_UTF_HPP
#define REN_UTF_HPP

#include <cstring>
#include <cstdint>

namespace ren
{
    typedef int32_t uChar;

    // These markers are experimental and of academic interest at this point. They are UNUSED in the current code. 
    // However, since they have no meaning in the UTF-8 system, they could be used otherwise:
    constexpr char marker0 = 0xF8;
    constexpr char marker1 = 0xF9;
    constexpr char marker2 = 0xFA;
    constexpr char marker3 = 0xFB;
    constexpr char marker4 = 0xFC;
    constexpr char marker5 = 0xFD;
    constexpr char marker6 = 0xFE;
    constexpr char marker7 = 0xFF;
    
    constexpr char markerPrefixMask = marker0;
    constexpr char markerValueMask = ~markerPrefixMask;

    // Note that these Unicode encodings follow a big-endian logic. This means the most significant bytes come first.
    namespace utf8
    {
        // The integrity of each encoded character is checked, since the 
        // decoder cannot function otherwise. This means every valid encoded character is composed 
        // of one length determining byte and up to three continuation bytes. 
        // If a problem is encountered during the decoding of this pattern only one byte 
        // is marked as read and an invalidCharacter constant is put. This means the process restarts 
        // at the very next byte, preventing single byte corruptions from invalidating the whole stream. 

        // NEW: Illegal (oversized) encodings are decoded as an invalidMarker (-1)

        // The character encoder encodes every value not exceeding 21 bits. All bits higher than this 
        // are simply discarded. The result are encoded characters with up to 4 bytes. 

        // The string encoder and decoder always terminate once a zero character or byte are encountered. 

        constexpr char oneBytePrefixMask = 0x80;
        constexpr char oneBytePrefixValue = 0x0;
        constexpr char oneByteValueMask = ~oneBytePrefixMask;
        constexpr uChar oneByteAllowableMask = 0x7F;
        constexpr uChar oneByteOverflowMask = ~oneByteAllowableMask;
        constexpr uChar oneByteMinimumValue = 0;

        constexpr char twoBytePrefixMask = 0x80 | 0x40 | 0x20;
        constexpr char twoBytePrefixValue = 0x80 | 0x40;
        constexpr char twoByteValueMask = ~twoBytePrefixMask;
        constexpr int twoByteShiftAmount = 5;
        constexpr uChar twoByteAllowableMask = 0x7FF;
        constexpr uChar twoByteOverflowMask = ~twoByteAllowableMask;
        constexpr uChar twoByteMinimumValue = oneByteAllowableMask + 1;

        constexpr char threeBytePrefixMask = 0x80 | 0x40 | 0x20 | 0x10;
        constexpr char threeBytePrefixValue = 0x80 | 0x40 | 0x20;
        constexpr char threeByteValueMask = ~threeBytePrefixMask;
        constexpr int threeByteShiftAmount = 4;
        constexpr uChar threeByteAllowableMask = 0xFFFF;
        constexpr uChar threeByteOverflowMask = ~threeByteAllowableMask;
        constexpr uChar threeByteMinimumValue = twoByteAllowableMask + 1;

        constexpr char fourBytePrefixMask = 0x80 | 0x40 | 0x20 | 0x10 | 0x08;
        constexpr char fourBytePrefixValue = 0x80 | 0x40 | 0x20 | 0x10;
        constexpr char fourByteValueMask = ~fourBytePrefixMask;
        constexpr int fourByteShiftAmount = 3;
        constexpr uChar fourByteAllowableMask = 0x10FFFF;
        constexpr uChar fourByteOverflowMask = ~fourByteAllowableMask;
        constexpr uChar fourByteMinimumValue = threeByteAllowableMask + 1;

        constexpr char continueBytePrefixMask = 0x80 | 0x40;
        constexpr char continueBytePrefixValue = 0x80;
        constexpr char continueByteValueMask = ~continueBytePrefixMask;
        constexpr int continueByteShiftAmount = 6;

        constexpr char bytewiseValueMasks[4] = {oneByteValueMask, twoByteValueMask, threeByteValueMask, fourByteValueMask};
        constexpr uChar lengthwiseMinimumValues[4] = {oneByteMinimumValue, twoByteMinimumValue, threeByteMinimumValue, fourByteMinimumValue};

        constexpr uChar invalidCharacter = -1;

        // Can only return 0, 1, 2, 3 or -1 (invalid).
        inline constexpr int getExtraByteCount(char firstbyte)
        {
            if ((firstbyte & oneBytePrefixMask) == oneBytePrefixValue) return 0;
            if ((firstbyte & twoBytePrefixMask) == twoBytePrefixValue) return 1;
            if ((firstbyte & threeBytePrefixMask) == threeBytePrefixValue) return 2;
            if ((firstbyte & fourBytePrefixMask) == fourBytePrefixValue) return 3;
            return -1;
        }

        inline constexpr bool isContinueByte(char byte)
        {
            if ((byte & continueBytePrefixMask) == continueBytePrefixValue) return true;
            else return false;
        }

        // Returns the count of consumed bytes.
        inline int decodeCharacter(uChar *destination, const char *source)
        {   
            if (destination == nullptr || source == nullptr) {
                return 0;
            }

            int extraByteCount = getExtraByteCount(*source);
            if (extraByteCount >= 0) {
                uChar result = 0;
                result |= source[0] & bytewiseValueMasks[extraByteCount];
                for (int byteIndex = 1; byteIndex <= extraByteCount; byteIndex++) {
                    if (isContinueByte(source[byteIndex])) {
                        result <<= continueByteShiftAmount;
                        result |= source[byteIndex] & continueByteValueMask;
                    }
                    else {
                        *destination = invalidCharacter;
                        return byteIndex;
                    }
                }
                if (result >= lengthwiseMinimumValues[extraByteCount]) {
                    *destination = result;
                    return extraByteCount + 1;
                }
                else {
                    *destination = -1;
                    return extraByteCount + 1;
                }                
            }
            else {
                *destination = invalidCharacter;
                return 1;
            }
        }

        // Returns the count of consumed bytes.
        inline int decode(uChar *destination, size_t destSize, const char* source) 
        {
            if (destination == nullptr || source == nullptr) {
                return 0;
            }

            size_t maxCharCount = destSize - 1;
            size_t readBytes = 0;
            size_t writtenChars = 0;
            while (source[readBytes] != 0 && writtenChars < maxCharCount) {
                readBytes += decodeCharacter(destination + writtenChars, source + readBytes);
                writtenChars++;
            }
            destination[writtenChars] = 0;
            return readBytes;
        }

        // Returns the count of written bytes.
        inline int encodeCharacter(char *destination, const uChar *source)
        {
            if (destination == nullptr || source == nullptr) {
                return 0;
            }

            if ((*source & oneByteOverflowMask) == 0) {
                *destination = *source;
                return 1;
            }
            if ((*source & twoByteOverflowMask) == 0) {
                char resultBuffer[2] {};
                uChar workBuffer = *source;
                resultBuffer[1] = continueBytePrefixValue | (workBuffer & continueByteValueMask);
                workBuffer >>= continueByteShiftAmount;
                resultBuffer[0] = twoBytePrefixValue | (workBuffer & twoByteValueMask);
                std::memcpy(destination, resultBuffer, sizeof(resultBuffer));
                return sizeof(resultBuffer);
            }
            if ((*source & threeByteOverflowMask) == 0) {
                char resultBuffer[3] {};
                uChar workBuffer = *source;
                resultBuffer[2] = continueBytePrefixValue | (workBuffer & continueByteValueMask);
                workBuffer >>= continueByteShiftAmount;
                resultBuffer[1] = continueBytePrefixValue | (workBuffer & continueByteValueMask);
                workBuffer >>= continueByteShiftAmount;
                resultBuffer[0] = threeBytePrefixValue | (workBuffer & threeByteValueMask);
                std::memcpy(destination, resultBuffer, sizeof(resultBuffer));
                return sizeof(resultBuffer);
            }
            // Stuff out of bounds gets truncated now:
            char resultBuffer[4] {};
            uChar workBuffer = *source;
            resultBuffer[3] = continueBytePrefixValue | (workBuffer & continueByteValueMask);
            workBuffer >>= continueByteShiftAmount;
            resultBuffer[2] = continueBytePrefixValue | (workBuffer & continueByteValueMask);
            workBuffer >>= continueByteShiftAmount;
            resultBuffer[1] = continueBytePrefixValue | (workBuffer & continueByteValueMask);
            workBuffer >>= continueByteShiftAmount;
            resultBuffer[0] = fourBytePrefixValue | (workBuffer & fourByteValueMask);
            std::memcpy(destination, resultBuffer, sizeof(resultBuffer));
            return sizeof(resultBuffer);
        }

        // Returns the count of consumed characters.
        inline int encode(char *destination, size_t destSize, const uChar *source)
        {
            if (destination == nullptr || source == nullptr) {
                return 0;
            }

            size_t maxByteCount = destSize - 1;
            size_t readChars = 0;
            size_t writtenBytes = 0;
            while (source[readChars] != 0 && writtenBytes <= maxByteCount - 4) { // In each iteration up to 4 bytes can be written.
                writtenBytes += encodeCharacter(destination + writtenBytes, source + readChars);
                readChars++;
            }
            destination[writtenBytes] = 0;
            return readChars;
        }
    }

    namespace utf16
    {
        typedef char16_t Char16;

        constexpr Char16 surrogatePrefixMask = 0xFC00;
        constexpr Char16 highSurrogatePrefixValue = 0xD800;
        constexpr Char16 lowSurrogatePrefixValue = 0xDC00;
        constexpr Char16 surrogateValueMask = ~surrogatePrefixMask;
        constexpr int surrogateShiftAmount = 10;

        // -> leaves 20 bits for encoding a 20 bit number.

        constexpr int32_t invalidCharacter = -1;

        // Returns the count of put Char16s.
        inline int encodeCharacter(Char16 *destination, const int32_t *source)
        {
            if (destination == nullptr || source == nullptr) {
                return 0;
            }

            int32_t value = *source;

            if (value < 0x10000) {
                *destination = static_cast<Char16>(value);
                return 1;
            }

            value -= 0x10000;
            
            Char16 result[2] {};
            result[1] = lowSurrogatePrefixValue | (value & surrogateValueMask);
            result[0] = highSurrogatePrefixValue | ((value >> surrogateShiftAmount) & surrogateValueMask);
            std::memcpy(destination, result, sizeof(result));
            return 2;
        }

        // Returns the count of consumed Char16s.
        inline int decodeCharacter(int32_t *destination, const Char16* source)
        {
            if (destination == nullptr || source == nullptr) {
                return 0;
            }

            Char16 prefix0 = source[0] & surrogatePrefixMask;
            if (prefix0 != highSurrogatePrefixValue && prefix0 != lowSurrogatePrefixValue) {
                *destination = source[0];
                return 1;
            }

            Char16 prefix1 = source[1] & surrogatePrefixMask;
            if (prefix0 == highSurrogatePrefixValue && prefix1 == lowSurrogatePrefixValue) {
                int32_t result = ((source[0] & surrogateValueMask) << surrogateShiftAmount) | (source[1] & surrogateValueMask);
                *destination = result + 0x10000;
                return 2;
            }

            *destination = invalidCharacter;
            return 1;
        }

        // Returns the count of consumed Char16s.
        inline int decode(int32_t *destination, size_t destSize, const Char16* source) 
        {
            if (destination == nullptr || source == nullptr) {
                return 0;
            }

            size_t maxCharCount = destSize - 1;
            size_t readChar16s = 0;
            size_t writtenChars = 0;
            while (source[readChar16s] != 0 && writtenChars < maxCharCount) {
                readChar16s += decodeCharacter(destination + writtenChars, source + readChar16s);
                writtenChars++;
            }
            destination[writtenChars] = 0;
            return readChar16s;
        }

        // Returns the count of consumed characters.
        inline int encode(Char16 *destination, size_t destSize, const int32_t *source)
        {
            if (destination == nullptr || source == nullptr) {
                return 0;
            }

            size_t maxByteCount = destSize - 1;
            size_t readChars = 0;
            size_t writtenChar16s = 0;
            while (source[readChars] != 0 && writtenChar16s <= maxByteCount - 2) { // In each iteration up to 2 Char16s can be written.
                writtenChar16s += encodeCharacter(destination + writtenChar16s, source + readChars);
                readChars++;
            }
            destination[writtenChar16s] = 0;
            return readChars;
        }
    }
}

#endif