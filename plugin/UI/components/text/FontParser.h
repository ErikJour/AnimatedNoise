//
// Created by Erik Jourgensen on 7/8/26.
//

#ifndef ANIMATEDNOISE_FONTPARSER_H
#define ANIMATEDNOISE_FONTPARSER_H
#include <fstream>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "FontReader.h"

class FontParser
{
public:
    explicit FontParser(const std::string& fontPath)
        : reader(fontPath)
    {
        initialize();
    }

    // For JUCE BinaryData — same parse path, different byte source
    FontParser(const uint8_t* data, size_t size)
        : reader(data, size)
    {
        initialize();
    }

    uint16_t glyphCount() const { return mNumGlyphs; }
    uint16_t unitsPerEm() const { return mUnitsPerEm; }

    // Character (Unicode code point) -> glyph index. Returns 0 (.notdef) if unmapped.
    uint16_t glyphIndexForChar(uint32_t codepoint) const
    {
        const auto it = mCmap.find(codepoint);
        return (it != mCmap.end()) ? it->second : 0;
    }

    GlyphData getGlyphByIndex(uint16_t index)
    {
        if (index >= mNumGlyphs)
            throw std::runtime_error("Glyph index out of range: " + std::to_string(index));

        GlyphData glyph;
        if (mLoca[index] != mLoca[index + 1])       // has an outline (else space etc.)
        {
            reader.seek(mGlyfOffset + mLoca[index]);
            glyph = readGlyph(reader);
        }

        glyph.advanceWidth = advanceWidthForGlyph(index);
        return glyph;
    }

private:
    //============================================================================================
    // One-time setup
    //============================================================================================
    void initialize()
    {
        parseTableDirectory();
        parseHead();
        parseMaxp();
        parseLoca();
        parseHhea();
        parseCmap();
    }

    void parseTableDirectory()
    {
        reader.skip(4);                             // sfnt version
        uint16_t numTables = reader.readUInt16BE();
        reader.skip(6);                             // searchRange, entrySelector, rangeShift

        for (uint16_t i = 0; i < numTables; i++)
        {
            std::string tag = reader.readTag();
            reader.skip(4);                         // checksum
            uint32_t offset = reader.readUInt32BE();
            reader.skip(4);                         // length

            mTables[tag] = offset;
        }

        mGlyfOffset = mTables.at("glyf");
    }

    void parseHead()
    {
        const uint32_t head = mTables.at("head");

        reader.seek(head + 18);
        mUnitsPerEm = reader.readUInt16BE();

        reader.seek(head + 50);
        mIndexToLocFormat = reader.readInt16BE();   // 0 = short, 1 = long
    }

    void parseMaxp()
    {
        reader.seek(mTables.at("maxp") + 4);        // skip version
        mNumGlyphs = reader.readUInt16BE();
    }

    void parseLoca()
    {
        reader.seek(mTables.at("loca"));
        mLoca.resize(static_cast<size_t>(mNumGlyphs) + 1);

        if (mIndexToLocFormat == 0)
        {
            // short format: uint16 offsets, stored divided by 2
            for (auto& entry : mLoca)
                entry = static_cast<uint32_t>(reader.readUInt16BE()) * 2;
        }
        else
        {
            // long format: uint32 offsets, stored as-is
            for (auto& entry : mLoca)
                entry = reader.readUInt32BE();
        }
    }

    //============================================================================================
    // Horizontal metrics (hhea + hmtx)
    //============================================================================================
    void parseHhea()
    {
        reader.seek(mTables.at("hhea") + 34);       // numberOfHMetrics is the last field
        mNumHMetrics = reader.readUInt16BE();
        mHmtxOffset  = mTables.at("hmtx");
    }

    uint16_t advanceWidthForGlyph(uint16_t index)
    {
        if (mNumHMetrics == 0)
            return 0;

        // hmtx has an advance width per glyph up to numberOfHMetrics; trailing glyphs
        // (typically monospaced) all reuse the last entry's advance.
        const uint16_t i = (index < mNumHMetrics) ? index : (mNumHMetrics - 1);
        reader.seek(mHmtxOffset + static_cast<uint32_t>(i) * 4);
        return reader.readUInt16BE();
    }

    //============================================================================================
    // Character map (cmap) — builds a code point -> glyph index table
    //============================================================================================
    void parseCmap()
    {
        const uint32_t cmap = mTables.at("cmap");
        reader.seek(cmap + 2);                      // skip version
        const uint16_t numSubtables = reader.readUInt16BE();

        // Prefer Windows Unicode BMP (3,1), then any Unicode (0,*), then Windows symbol (3,0).
        uint32_t bestOffset = 0;
        int      bestScore  = -1;
        for (uint16_t i = 0; i < numSubtables; i++)
        {
            const uint16_t platformID = reader.readUInt16BE();
            const uint16_t encodingID = reader.readUInt16BE();
            const uint32_t subOffset  = reader.readUInt32BE();

            int score = -1;
            if      (platformID == 3 && encodingID == 1) score = 3;
            else if (platformID == 0)                    score = 2;
            else if (platformID == 3 && encodingID == 0) score = 1;

            if (score > bestScore)
            {
                bestScore  = score;
                bestOffset = cmap + subOffset;
            }
        }

        if (bestScore < 0)
            return;                                 // no usable subtable; lookups return 0

        reader.seek(bestOffset);
        if (reader.readUInt16BE() == 4)             // format 4 covers the BMP
            parseCmapFormat4(bestOffset);
    }

    void parseCmapFormat4(uint32_t subtableOffset)
    {
        reader.seek(subtableOffset + 6);            // skip format, length, language
        const uint16_t segCount = static_cast<uint16_t>(reader.readUInt16BE() / 2);
        reader.skip(6);                             // searchRange, entrySelector, rangeShift

        std::vector<uint16_t> endCode(segCount), startCode(segCount), idRangeOffset(segCount);
        std::vector<int16_t>  idDelta(segCount);

        for (auto& v : endCode)   v = reader.readUInt16BE();
        reader.skip(2);                             // reservedPad
        for (auto& v : startCode) v = reader.readUInt16BE();
        for (auto& v : idDelta)   v = reader.readInt16BE();

        // idRangeOffset entries encode a byte offset relative to their own position, so
        // remember where the array starts before reading it.
        const uint32_t idRangeOffsetBase = static_cast<uint32_t>(reader.tell());
        for (auto& v : idRangeOffset) v = reader.readUInt16BE();

        for (uint16_t s = 0; s < segCount; s++)
        {
            if (startCode[s] == 0xFFFF)             // terminator segment
                continue;

            // idDelta wraps mod 65536; reinterpret its bits as unsigned so the add is
            // unsigned throughout and free of signed/unsigned mixing.
            const uint32_t delta = static_cast<uint16_t>(idDelta[s]);
            const uint32_t start = startCode[s];
            const uint32_t end   = endCode[s];

            for (uint32_t c = start; c <= end; c++)
            {
                uint16_t glyphId = 0;
                if (idRangeOffset[s] == 0)
                {
                    glyphId = static_cast<uint16_t>((c + delta) & 0xFFFFu);
                }
                else
                {
                    const uint32_t addr = idRangeOffsetBase + static_cast<uint32_t>(s) * 2u
                                        + idRangeOffset[s] + (c - start) * 2u;
                    reader.seek(addr);
                    const uint32_t raw = reader.readUInt16BE();
                    if (raw != 0)
                        glyphId = static_cast<uint16_t>((raw + delta) & 0xFFFFu);
                }

                if (glyphId != 0)
                    mCmap[c] = glyphId;
            }
        }
    }

    //============================================================================================
    // Glyph parsing
    //============================================================================================
    static bool flagBitIsSet(uint8_t flag, int bitIndex)
    {
        return ((static_cast<int>(flag) >> bitIndex) & 1) == 1;
    }

    static std::vector<int> readCoordinates(FontReader& reader,
                                            const std::vector<uint8_t>& allFlags,
                                            bool readingX)
    {
        int offsetSizeFlagBit   = readingX ? 1 : 2;   // X_SHORT / Y_SHORT
        int offsetSignOrSkipBit = readingX ? 4 : 5;   // sign (if short) or same-as-prev

        std::vector<int> coordinates(allFlags.size());

        for (size_t i = 0; i < coordinates.size(); i++)
        {
            // start from the previous coordinate (0 for the first point)
            coordinates[i] = (i > 0) ? coordinates[i - 1] : 0;

            uint8_t flag = allFlags[i];

            if (flagBitIsSet(flag, offsetSizeFlagBit))
            {
                // short form: 1 unsigned byte, bit 4/5 gives the sign
                int offset = static_cast<int>(reader.readUInt8());
                int sign   = flagBitIsSet(flag, offsetSignOrSkipBit) ? 1 : -1;
                coordinates[i] += offset * sign;
            }
            else if (!flagBitIsSet(flag, offsetSignOrSkipBit))
            {
                // long form: signed 16-bit delta (big-endian)
                coordinates[i] += static_cast<int>(reader.readInt16BE());
            }
            // else: bit set + not short = coordinate same as previous, read nothing
        }

        return coordinates;
    }

    static GlyphData readGlyph(FontReader& reader)
    {
        int16_t numContours = reader.readInt16BE();

        if (numContours < 0)
        {
            // Compound glyph — built from other glyphs + transforms. Later.
            return {};
        }

        if (numContours == 0)
            return {};                              // empty glyph

        return readSimpleGlyph(reader, numContours);
    }

    static GlyphData readSimpleGlyph(FontReader& reader, int16_t numContours)
    {
        std::vector<uint16_t> contourEndIndices(static_cast<size_t>(numContours));
        reader.skip(8);                             // xMin, yMin, xMax, yMax

        for (size_t i = 0; i < contourEndIndices.size(); i++)
            contourEndIndices[i] = reader.readUInt16BE();

        int numPoints = static_cast<int>(contourEndIndices.back()) + 1;

        std::vector<uint8_t> allFlags(static_cast<size_t>(numPoints));
        reader.skip(static_cast<size_t>(reader.readUInt16BE()));   // instructions

        for (int i = 0; i < numPoints; i++)
        {
            uint8_t flag = reader.readUInt8();
            allFlags[static_cast<size_t>(i)] = flag;

            if (flagBitIsSet(flag, 3))              // REPEAT
            {
                int repeatCount = static_cast<int>(reader.readUInt8());
                for (int r = 0; r < repeatCount; r++)
                    allFlags[static_cast<size_t>(++i)] = flag;
            }
        }

        std::vector<int> coordsX = readCoordinates(reader, allFlags, /*readingX=*/true);
        std::vector<int> coordsY = readCoordinates(reader, allFlags, /*readingX=*/false);

        return GlyphData{ std::move(coordsX), std::move(coordsY),
                          std::move(contourEndIndices) };
    }

    //============================================================================================
    // State
    //============================================================================================
    FontReader reader;
    std::unordered_map<std::string, uint32_t> mTables;
    std::unordered_map<uint32_t, uint16_t>    mCmap;    // code point -> glyph index
    std::vector<uint32_t> mLoca;
    uint32_t mGlyfOffset       = 0;
    uint32_t mHmtxOffset       = 0;
    uint16_t mNumHMetrics      = 0;
    uint16_t mNumGlyphs        = 0;
    uint16_t mUnitsPerEm       = 1000;
    int16_t  mIndexToLocFormat = 0;
};

#endif //ANIMATEDNOISE_FONTPARSER_H