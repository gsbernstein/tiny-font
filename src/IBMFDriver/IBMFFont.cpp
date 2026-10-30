#if CONFIG_TINYFONT_IBMF

#include "IBMFFont.hpp"

auto Font::ligKernUTF8Map(const std::string &line, LigKernMappingHandler handler) const -> void {
    if (line.length() != 0) {
        auto iter = UTF8Iterator(line);
        auto glyphCode1 = fontData_->translate(*iter++);
        auto glyphCode2 = (iter == line.end()) ? NO_GLYPH_CODE : fontData_->translate(*iter++);
        FIX16 kern;
        bool firstWordChar = true;
        bool wasEndOfWord = false;
        while (glyphCode1 != NO_GLYPH_CODE) {
            if (wasEndOfWord && (glyphCode1 != SPACE_CODE)) {
                wasEndOfWord = false;
                firstWordChar = true;
            }
            kern = static_cast<FIX16>(0);

            // Ligature loop for glyphCode1
            while (fontData_->getFace(faceIndex_)->ligKern(glyphCode1, &glyphCode2, &kern)) {
                glyphCode1 = glyphCode2;
                glyphCode2 = (iter == line.end()) ? NO_GLYPH_CODE : fontData_->translate(*iter++);
            }

            // Ligature loop for glyphCode2
            auto glyphCode3 = (iter == line.end()) ? NO_GLYPH_CODE : fontData_->translate(*iter);
            if (glyphCode3 != NO_GLYPH_CODE) {
                bool someLig = false;
                FIX16 k;
                while (fontData_->getFace(faceIndex_)->ligKern(glyphCode2, &glyphCode3, &k)) {
                    glyphCode2 = glyphCode3;
                    glyphCode3 =
                        (iter == line.end()) ? NO_GLYPH_CODE : fontData_->translate(*++iter);
                    someLig = true;
                }
                if (someLig) {
                    fontData_->getFace(faceIndex_)->ligKern(glyphCode1, &glyphCode2, &kern);
                }
            }

            bool lastWordChar = (glyphCode2 == SPACE_CODE) || (glyphCode2 == NO_GLYPH_CODE);
            (handler)(glyphCode1, kern, firstWordChar, lastWordChar);
            firstWordChar = false;
            if (lastWordChar) {
                wasEndOfWord = true;
            }
            glyphCode1 = glyphCode2;
            glyphCode2 = (iter == line.end()) ? NO_GLYPH_CODE : fontData_->translate(*iter++);
        }
    }
}

// Synthesizes italics by shearing the upright glyph about the baseline: each row is shifted right
// in proportion to its height above the baseline (descenders shift left).
auto Font::drawSlantedGlyph(ibmf_defs::Bitmap &canvas, GlyphCode glyphCode, ibmf_defs::Pos atPos,
                            bool inverted, ibmf_defs::Glyph &glyphOut) const -> bool {
    IBMFFace *face = fontData_->getFace(faceIndex_);

    ibmf_defs::Glyph metricsOnly{};
    if (!face->getGlyph(glyphCode, metricsOnly, false)) {
        return false;
    }
    glyphOut.metrics = metricsOnly.metrics;

    // With caching enabled the face decodes into its own freshly allocated bitmap. Passing the
    // vertical offset as the position cancels the face's baseline adjustment so the glyph lands
    // at (0, 0) in that bitmap.
    ibmf_defs::Glyph decoded{};
    if (!face->getGlyph(glyphCode, decoded, true, true, ibmf_defs::Pos(0, metricsOnly.metrics.yoff),
                        false)) {
        return false;
    }
    if (decoded.bitmap.pixels == nullptr) {
        return true; // Space or blank glyph, nothing to draw
    }

    const bool oneBit = getDisplayPixelResolution() == PixelResolution::ONE_BIT;
    const int16_t xoff = metricsOnly.metrics.xoff;
    const int16_t yoff = metricsOnly.metrics.yoff;
    const int originX = atPos.x - xoff;
    const int originY = atPos.y - yoff;

    for (int row = 0; row < decoded.bitmap.dim.height; row++) {
        const int destY = originY + row;
        if (destY < 0 || destY >= canvas.dim.height) {
            continue;
        }

        const int shift = slantShift(yoff - row);

        const MemoryPtr srcRow = decoded.bitmap.pixels + row * decoded.bitmap.pitch;
        const MemoryPtr destRow = canvas.pixels + destY * canvas.pitch;

        for (int col = 0; col < decoded.bitmap.dim.width; col++) {
            const int destX = originX + col + shift;
            if (destX < 0 || destX >= canvas.dim.width) {
                continue;
            }

            if (oneBit) {
                const uint8_t srcBit = (srcRow[col >> 3] >> (7 - (col & 7))) & 1;
                if (srcBit != BLACK_ONE_BIT) {
                    continue;
                }
                const uint8_t mask = 0x80 >> (destX & 7);
                if ((inverted ? WHITE_ONE_BIT : BLACK_ONE_BIT) != 0) {
                    destRow[destX >> 3] |= mask;
                } else {
                    destRow[destX >> 3] &= ~mask;
                }
            } else {
                if (srcRow[col] != BLACK_EIGHT_BITS) {
                    continue;
                }
                destRow[destX] = inverted ? WHITE_EIGHT_BITS : BLACK_EIGHT_BITS;
            }
        }
    }

    delete[] decoded.bitmap.pixels;
    return true;
}

// Returns the x position at the end of string
auto Font::drawSingleLineOfText(ibmf_defs::Bitmap &canvas, ibmf_defs::Pos pos,
                                const std::string &line, bool inverted) const -> int {

    ibmf_defs::Pos atPos = pos;

    font_defs::Dim dim = getTextSize("T");

    // Half of the difference between the line height (less subscripts) and the height
    // of the font is how much we need to move up the text to ensure the subscript part
    // of letters that go below the baseline still fit within the bounding box.
    int16_t up = (lineHeight() - dim.height - 1) / 2;

    // We get passed in the upper left coordinate, so we need to shift that down by the
    // height of the line.
    atPos.y += lineHeight() - up;

    if constexpr (IBMF_TRACING) {
        LOGD(FONT_TAG, "drawSingleLineOfText()");
    }

    if (isInitialized()) {
        Glyph glyph{};

        // We set here the canvas pitch value, as there is still some definitions required at the
        // application-level in regard of the upcoming Sol Glasse augmented resolution.
        //
        // The following may require some modification as the next Sol Glasses version
        // may be using a different pitch than the one computed here.

        canvas.pitch = (getDisplayPixelResolution() == PixelResolution::ONE_BIT)
                           ? (canvas.dim.width + 7) >> 3
                           : canvas.dim.width;

        glyph.bitmap = canvas;
        int italicCorrection = 0;

        ligKernUTF8Map(line, [this, &glyph, &atPos, &italicCorrection,
                              inverted](GlyphCode glyphCode, FIX16 kern, bool first, bool last) {
            int8_t hOffset = first ? fontData_->getFace(faceIndex_)->getGlyphHOffset(glyphCode) : 0;
            atPos.x += hOffset;

            // LOGD(FONT_TAG, "Word Markers: %d %d", first, last);

            bool drawn = isItalic_
                             ? drawSlantedGlyph(glyph.bitmap, glyphCode, atPos, inverted, glyph)
                             : fontData_->getFace(faceIndex_)->getGlyph(glyphCode, glyph, true,
                                                                        false, atPos, inverted);
            if (drawn) {
                // As advance is positive and greather than kern, we can shift right
                // to get rid of the fix point decimals
                if (glyphCode == SPACE_CODE) {
                    atPos.x += glyph.metrics.advance >> 6;
                } else {
                    atPos.x += last ? fontData_->getFace(faceIndex_)->getGlyphWidth(glyphCode) -
                                          (kern / 64) - glyph.metrics.xoff
                                    : ((glyph.metrics.advance + kern) >> 6);
                    italicCorrection = isItalic_ ? slantShift(glyph.metrics.yoff) : 0;
                }
            }
        });

        // Keep the leaning top of the last italic glyph from crowding upright text that follows
        // (e.g. punctuation right after an italic word, or the next word after a trailing space).
        atPos.x += italicCorrection;
    }

    return atPos.x;
}

auto Font::getTextSize(const std::string &buffer) const -> ibmf_defs::Dim {
    // LOGD(FONT_TAG, "getTextSize(): %s", buffer.c_str());
    if constexpr (IBMF_TRACING) {
        LOGD(FONT_TAG, "getTextSize()");
    }
    ibmf_defs::Dim dim = ibmf_defs::Dim(0, 0);
    int16_t up = 0;
    int16_t down = 0;
    if (isInitialized()) {
        ligKernUTF8Map(buffer, [this, &dim, &up, &down](GlyphCode glyphCode, FIX16 kern, bool first,
                                                        bool last) {
            // int8_t hOffset = first ? fontData_->getFace(faceIndex_)->getGlyphHOffset(glyphCode) :
            // 0;

            ibmf_defs::Glyph glyph{};
            if (fontData_->getFace(faceIndex_)
                    ->getGlyph(glyphCode, glyph, false)) { // retrieves only the metrics
                // LOGD(FONT_TAG, "Advance: %f, xoff: %d, yoff: %d, descent: %d, kern: %d",
                //      IBMFFace::fromFIX16(glyph.metrics.advance), glyph.metrics.xoff,
                //      glyph.metrics.yoff, glyph.metrics.descent, kern);
                if (glyphCode == SPACE_CODE) {
                    dim.width += glyph.metrics.advance >> 6;
                } else {
                    dim.width += last ? fontData_->getFace(faceIndex_)->getGlyphWidth(glyphCode) -
                                            (kern / 64) - glyph.metrics.xoff
                                      : ((glyph.metrics.advance + kern) >> 6);
                }
                up = (up < glyph.metrics.yoff) ? glyph.metrics.yoff : up;
                down = (down < glyph.metrics.descent) ? glyph.metrics.descent : down;
                // dim.height = (dim.height > glyph.metrics.yoff)
                //                  ? glyph.metrics.yoff
                //                  : dim.height; // yoff is negative...
            } else {
                // LOGW(FONT_TAG, "Unable to retrieve glyph for glyphCode %d", glyphCode);
            }
        });
    }
    dim.height = (up + down);
    // LOGD(FONT_TAG, "IBMFFont: TextBound: width: %d, height: %d", dim.width, dim.height);
    return dim;
}

auto Font::getTextWidth(const std::string &buffer) -> int {
    if constexpr (IBMF_TRACING) {
        LOGD(FONT_TAG, "getTextWidth()");
    }
    int width = 0;
    if (isInitialized()) {
        ligKernUTF8Map(buffer,
                       [this, &width](GlyphCode glyphCode, FIX16 kern, bool first, bool last) {
            ibmf_defs::Glyph glyph{};
            if (fontData_->getFace(faceIndex_)
                    ->getGlyph(glyphCode, glyph, false)) { // retrieves only the metrics

                // LOGD(FONT_TAG, "Advance: %f, xoff: %d, yoff: %d, descent: %d, kern: %d",
                //      IBMFFace::fromFIX16(glyph.metrics.advance), glyph.metrics.xoff,
                //      glyph.metrics.yoff, glyph.metrics.descent, kern);

                if (glyphCode == SPACE_CODE) {
                    width += glyph.metrics.advance >> 6;
                } else {
                    width += last ? fontData_->getFace(faceIndex_)->getGlyphWidth(glyphCode) -
                                        (kern / 64) - glyph.metrics.xoff
                                  : ((glyph.metrics.advance + kern) >> 6); // + glyph.metrics.xoff;
                }

                // LOGD(FONT_TAG, "Advance value: %f, kern: %f",
                //     IBMFFace::fromFIX16(glyph.metrics.advance),
                //     IBMFFace::fromFIX16(kern));
                // width += (glyph.metrics.advance + kern + 32) >> 6;
            } else {
                // LOGW(FONT_TAG, "Unable to retrieve glyph for glyphCode %d", glyphCode);
            }
        });
    }
    // LOGD(FONT_TAG, "IBMFFont: TextWidth: %d", width);
    return width;
}

auto Font::toChar32(const char **str) -> char32_t {
    const auto *s = reinterpret_cast<const uint8_t *>(*str);
    uint8_t c1 = *s++;

    char32_t res;

    if (c1 >= 0xC0) {
        uint8_t c2 = *s++;

        if (c1 >= 0xE0) {
            uint8_t c3 = *s++;

            if (c1 >= 0xF0) {
                uint8_t c4 = *s++;

                res = ((c1 & 0b00000111) << 18) | ((c2 & 0b00111111) << 12) |
                      ((c3 & 0b00111111) << 6) | (c4 & 0b00111111);
            } else {
                res = ((c1 & 0b00001111) << 12) | ((c2 & 0b00111111) << 6) | (c3 & 0b00111111);
            }
        } else {
            res = ((c1 & 0b00011111) << 6) | (c2 & 0b00111111);
        }

    } else {
        res = c1 & 0b01111111;
    }

    *str = reinterpret_cast<const char *>(s);
    return res;
}

auto Font::getTextHeight(const std::string &buffer) const -> int {
    if constexpr (IBMF_TRACING) {
        LOGD(FONT_TAG, "getTextHeight()");
    }
    int height = 0;
    if (isInitialized()) {
        // for (UTF8Iterator chrIter = buffer.begin(); chrIter != buffer.end(); chrIter++) {
        //     ibmf_defs::Glyph glyph;
        //     if (fontData_->getFace(faceIndex_)->getGlyph(*chrIter, glyph, false)) {
        //         // LOGD(FONT_TAG, "yoff value: %d", glyph.metrics.yoff);
        //         if (height > glyph.metrics.yoff) {
        //             height = glyph.metrics.yoff; // yoff is negative
        //         }
        //     } else {
        //         LOGW(FONT_TAG, "Unable to retrieve glyph for char %d(%c)", *chrIter, *chrIter);
        //     }
        // }
        // height = fontData_->getFace(faceIndex_)->getEmHeight();
        Dim dim = getTextSize(buffer);
        height = dim.height;
    }
    // LOGD(FONT_TAG, "TextHeight: %d", height);
    return height;
}

#endif