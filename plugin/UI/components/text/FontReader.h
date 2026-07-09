//
// Created by Erik Jourgensen on 7/8/26.
//

#ifndef ANIMATEDNOISE_TEXT_H
#define ANIMATEDNOISE_TEXT_H

#include <fstream>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <unordered_map>

//================================================================================================
// FontReader — raw byte access. Knows nothing about fonts.
//================================================================================================
class FontReader
{
public:
    // From memory (for JUCE BinaryData later)
    FontReader(const uint8_t* fontData, size_t fontSize)
        : buffer(fontData, fontData + fontSize) {}

    // From file
    explicit FontReader(const std::string& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
            throw std::runtime_error("Failed to open font: " + path);
        buffer.assign(std::istreambuf_iterator<char>(stream),
                      std::istreambuf_iterator<char>());
    }

    size_t tell() const        { return pos; }
    void   seek(size_t newPos) { checkBounds(newPos, 0); pos = newPos; }
    void   skip(size_t n)      { checkBounds(pos, n); pos += n; }

    uint8_t readUInt8()
    {
        checkBounds(pos, 1);
        return buffer[pos++];
    }

    uint16_t readUInt16BE()
    {
        checkBounds(pos, 2);
        uint16_t v = static_cast<uint16_t>(
            (static_cast<int>(buffer[pos]) << 8) | static_cast<int>(buffer[pos + 1]));
        pos += 2;
        return v;
    }

    int16_t readInt16BE()
    {
        return static_cast<int16_t>(readUInt16BE());
    }

    uint32_t readUInt32BE()
    {
        checkBounds(pos, 4);
        uint32_t v = (static_cast<uint32_t>(buffer[pos])     << 24)
                   | (static_cast<uint32_t>(buffer[pos + 1]) << 16)
                   | (static_cast<uint32_t>(buffer[pos + 2]) << 8)
                   |  static_cast<uint32_t>(buffer[pos + 3]);
        pos += 4;
        return v;
    }

    std::string readTag()
    {
        checkBounds(pos, 4);
        std::string tag(reinterpret_cast<const char*>(buffer.data() + pos), 4);
        pos += 4;
        return tag;
    }

private:
    void checkBounds(size_t at, size_t n) const
    {
        if (at + n > buffer.size())
            throw std::runtime_error("FontReader: read past end of font data");
    }

    std::vector<uint8_t> buffer;
    size_t pos = 0;
};

//================================================================================================
// GlyphData — one parsed glyph outline, in font units.
//================================================================================================
struct GlyphData
{
    std::vector<int>        xCoords;
    std::vector<int>        yCoords;
    std::vector<bool>       onCurve;
    std::vector<uint16_t>   contourEndIndices;
    int                     advanceWidth = 0;   // font units; set even for empty glyphs (space)

    bool isEmpty() const { return xCoords.empty(); }


};


#endif //ANIMATEDNOISE_TEXT_H