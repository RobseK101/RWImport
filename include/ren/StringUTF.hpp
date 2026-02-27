#ifndef REN_STRINGUTF_HPP
#define REN_STRINGUTF_HPP

#include <ren/utf.hpp>
#include <string>

namespace ren
{
#ifdef _WIN32
    // Specific for Windows; Linux etc. use 32 bit wchar_t
    inline std::wstring utf8ToWStr(const char *source, wchar_t invalidChar = L'�')
    {
        static_assert(sizeof(wchar_t) == 2, "This implementation is for Windows.");

        constexpr size_t ucharBufferCount = 256;
        constexpr size_t wcharBufferCount = 512;

        uChar ucharBuffer[ucharBufferCount];
        wchar_t wcharBuffer[wcharBufferCount];
        std::wstring output;

        if (source == nullptr) {
            return output;
        }

        size_t consumedBytes = 0;
        while (source[consumedBytes] != 0) {
            consumedBytes += utf8::decode(ucharBuffer, ucharBufferCount, source + consumedBytes);
            // Check the result in the buffer for "invalid" marks and convert them to something readable.
            for (size_t charIndex = 0; ucharBuffer[charIndex] != 0 && charIndex < ucharBufferCount; charIndex++) {
                if (ucharBuffer[charIndex] == utf8::invalidCharacter) {
                    ucharBuffer[charIndex] = invalidChar;
                }
            }
            utf16::encode(reinterpret_cast<utf16::Char16*>(wcharBuffer), wcharBufferCount, ucharBuffer);
            output += wcharBuffer;
        }
        return output;
    }

#elif defined(__GNUG__)

    inline std::wstring utf8ToWstr(const char *source, wchar_t invalidChar = L'�')
    {
        static_assert(sizeof(wchar_t) == 4, "This implementation is for a non-Windows OS.");

        constexpr size_t ucharBufferCount = 256;
        uChar ucharBuffer[ucharBufferCount];
        std::wstring output;

        if (source == nullptr) {
            return output;
        }

        size_t consumedBytes = 0;
        while (source[consumedBytes] != 0) {
            consumedBytes += utf8::decode(ucharBuffer, ucharBufferCount, source + consumedBytes);
            // Check the result in the buffer for "invalid" marks and convert them into something readable.
            for (size_t charIndex = 0; ucharBuffer[charIndex] != 0 && charIndex < ucharBufferCount; charIndex++) {
                if (ucharBuffer[charIndex] == utf8::invalidCharacter) {
                    ucharBuffer[charIndex] = invalidChar;
                }
            }
            output += reinterpret_cast<wchar_t*>(ucharBuffer);
        }
        return output;
    }
#endif // _WIN32

#ifdef _WIN32
    // Specific for Windows; Linux etc. use 32 bit wchar_t
    inline std::string wstrToUtf8(const wchar_t* source, wchar_t invalidChar = L'�')
    {
        static_assert(sizeof(wchar_t) == 2, "This implementation is for Windows.");

        constexpr size_t ucharBufferCount = 256;
        constexpr size_t byteBufferCount = 1024;

        uChar ucharBuffer[ucharBufferCount];
        char byteBuffer[byteBufferCount];
        std::string output;

        if (source == nullptr) {
            return output;
        }

        size_t consumedWChars = 0;
        while (source[consumedWChars] != 0) {
            consumedWChars += utf16::decode(ucharBuffer, ucharBufferCount, reinterpret_cast<const utf16::Char16*>(source + consumedWChars));
            // Check the result in the buffer for "invalid" marks and convert them to something readable.
            for (size_t charIndex = 0; ucharBuffer[charIndex] != 0 && charIndex < ucharBufferCount; charIndex++) {
                if (ucharBuffer[charIndex] == utf16::invalidCharacter) {
                    ucharBuffer[charIndex] = invalidChar;
                }
            }
            utf8::encode(byteBuffer, byteBufferCount, ucharBuffer);
            output += byteBuffer;
        }
        return output;
    }

#elif defined(__GNUG__)

    inline std::string wstrToUtf8(const wchar_t* source, wchar_t invalidChar = L'�')
    {
        static_assert(sizeof(wchar_t) == 4, "This implementation is for a non-Windows OS.");

        constexpr size_t ucharBufferCount = 256;
        constexpr size_t writeableUChars = ucharBufferCount - 1;
        constexpr size_t byteBufferCount = 1024;

        uChar ucharBuffer[ucharBufferCount];
        char byteBuffer[byteBufferCount];
        std::string output;

        if (source == nullptr) {
            return output;
        }

        size_t consumedWChars = 0;
        while (source[consumedWChars] != 0) {
            // copy source material replacing invalid characters:
            for (size_t ucharIndex = 0; ucharIndex < writeableUChars; ucharIndex++) {
                if (source[consumedWChars] & utf8::fourByteOverflowMask) {
                    ucharBuffer[ucharIndex] = invalidChar;
                }
                else if (source[consumedWChars] == 0) {
                    ucharBuffer[ucharIndex] = 0;
                    consumedWChars++;
                    break;
                }                
                else {
                    ucharBuffer[ucharIndex] = source[consumedWChars];
                }
                consumedWChars++;
            }
            ucharBuffer[writeableUChars] = 0;
            utf8::encode(byteBuffer, byteBufferCount, ucharBuffer);
            output += byteBuffer;
        }
        return output;
    }

#endif // _WIN32


    // This just replaces every non-ASCII character in the string.
    inline std::string utf8ToAStr(const char *source, char replacement = '?')
    {
        constexpr size_t bufferCount = 256;

        uChar ucharBuffer[bufferCount];
        char charBuffer[bufferCount];

        std::string output;

        if (source == nullptr) {
            return output;
        }

        size_t consumedChars = 0;
        while (source[consumedChars] != 0) {
            consumedChars += utf8::decode(ucharBuffer, bufferCount, source + consumedChars);
            // Copy everything in the uchar buffer into the char buffer if it's small enough. Replace everything else.
            for (size_t charIndex = 0; ucharBuffer[charIndex] != 0 && charIndex < bufferCount; charIndex++) {
                if (ucharBuffer[charIndex] < 128 && ucharBuffer[charIndex] != utf8::invalidCharacter) {
                    charBuffer[charIndex] = static_cast<char>(ucharBuffer[charIndex]);
                }
                else {
                    charBuffer[charIndex] = replacement;
                }
            }
            output += charBuffer;
        }
        return output;
    }
}

#endif // REN_STRINGUTF_HPP
