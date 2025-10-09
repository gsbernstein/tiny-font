#pragma once

#if CONFIG_TINYFONT_IBMF

#include <cstdio>
#include <functional>

#include "../Font.hpp"
#include "../UTF8Iterator.hpp"
#include "IBMFFontData.hpp"

class Font {
private:
    FontData *fontData_;
    int faceIndex_;
    // IBMFFace *face_;

    // Maximum size of an allocated buffer to do vsnprintf formatting
    static constexpr int MAX_SIZE = 100;

    typedef std::function<void(GlyphCode, FIX16, bool, bool)> const &LigKernMappingHandler;

    /// @brief Ligature/Kerning/UTF8 Mapper
    ///
    /// Iterates on each UTF8 character present in **line**, sending to the
    /// **handler** the corresponding GlyphCode and kerning values after
    /// applying the LigKern program to the character.
    ///
    /// @param line In. The UTF8 compliant string of character.
    /// @param handler Call. The callback closure handler.
    ///
    auto ligKernUTF8Map(const std::string &line, LigKernMappingHandler handler) const -> void;

public:
    Font(FontData &ibmfFont, int index) noexcept : fontData_(&ibmfFont), faceIndex_(index) {}

    ~Font() = default;

    [[nodiscard]] inline auto isInitialized() const -> bool {
        if ((fontData_ != nullptr) && fontData_->isInitialized() &&
            fontData_->getFace(faceIndex_)->isInitialized()) {
            return true;
        }

        LOGE(FONT_TAG, "!!!! FONT NOT INITIALIZED !!!!");
        return false;
    }

    [[nodiscard]] inline auto getFace() -> const IBMFFace * {
        return isInitialized() ? fontData_->getFace(faceIndex_) : nullptr;
    }

    inline auto setDisplayPixelResolution(PixelResolution res) -> bool {
#if CONFIG_TINYFONT_PIXEL_RESOLUTION_IS_FIX
        LOGW(FONT_TAG, "The display does not allow to change it's pixel resolution!");
        return false;
#else

        if constexpr (IBMF_TRACING) {
            LOGD(FONT_TAG, "setDisplayPixelResolution_()");
        }
        if (isInitialized()) {
            fontData_->getFace(faceIndex_)->setDisplayPixelResolution(res);
            return true;
        }
        return false;
#endif
    }

    [[nodiscard]] inline auto getDisplayPixelResolution() const -> PixelResolution {
        if constexpr (IBMF_TRACING) {
            LOGD(FONT_TAG, "getDisplayPixelResolution_()");
        }
        return (isInitialized()) ? fontData_->getFace(faceIndex_)->getDisplayPixelResolution()
                                 : DEFAULT_DISPLAY_PIXEL_RESOLUTION;
    }

    [[nodiscard]] inline auto lineHeight() const -> int {
        if constexpr (IBMF_TRACING) {
            LOGD(FONT_TAG, "lineHeight()");
            if (!isInitialized()) {
                LOGE(FONT_TAG, "Not initialized!!!");
            }
        }
        return isInitialized() ? static_cast<int>(fontData_->getFace(faceIndex_)->getLineHeight())
                               : 0;
    }

    auto drawSingleLineOfText(ibmf_defs::Bitmap &canvas, ibmf_defs::Pos pos,
                              const std::string &line, bool inverted) const -> int;
    [[nodiscard]] auto getTextSize(const std::string &buffer) const -> ibmf_defs::Dim;
    auto getTextWidth(const std::string &buffer) -> int;

    [[nodiscard]] auto getTextHeight(const std::string &buffer) const -> int;

    // Non-validating algorithm
    auto toChar32(const char **str) -> char32_t;

    // inline auto getTextWidthQuick(const char *buffer) -> int { return getTextWidth(buffer); }
    inline auto getTextWidthQuick(const char *buffer) -> int16_t {
        if constexpr (IBMF_TRACING) {
            LOGD(FONT_TAG, "getTextWidthQuick()");
        }

        int16_t width = 0;
        auto face = fontData_->getFace(faceIndex_);

        // LOGW(FONT_TAG, "word: %s", buffer);

        while (*buffer) {
            int16_t xoff;
            FIX16 advance;
            GlyphCode glyphCode = fontData_->translate(toChar32(&buffer));

            if (face->getGlyphHorizontalMetrics(glyphCode, &xoff, &advance)) {
                width +=
                    (*buffer == '\0') ? (face->getGlyphWidth(glyphCode) - xoff) : (advance >> 6);
            }
        }
        // LOGW(FONT_TAG, " width: %" PRIi16, width);
        return width;
    }

    // Character to superscript code point LUT
    // 128-entry ASCII table; empty entries are nullptr.
    static constexpr auto superscriptCharacters = [] {
        std::array<const char *, 128> a{};

        a['0'] = "\xE2\x81\xB0";
        a['1'] = "\xC2\xB9";
        a['2'] = "\xC2\xB2";
        a['3'] = "\xC2\xB3";
        a['4'] = "\xE2\x81\xB4";
        a['5'] = "\xE2\x81\xB5";
        a['6'] = "\xE2\x81\xB6";
        a['7'] = "\xE2\x81\xB7";
        a['8'] = "\xE2\x81\xB8";
        a['9'] = "\xE2\x81\xB9";

        a['A'] = "\xEE\x81\x81";
        a['B'] = "\xEE\x81\x82";
        a['C'] = "\xEE\x81\x83";
        a['D'] = "\xEE\x81\x84";
        a['E'] = "\xEE\x81\x85";
        a['F'] = "\xEE\x81\x86";
        a['G'] = "\xEE\x81\x87";
        a['H'] = "\xEE\x81\x88";
        a['I'] = "\xEE\x81\x89";
        a['J'] = "\xEE\x81\x8A";
        a['K'] = "\xEE\x81\x8B";
        a['L'] = "\xEE\x81\x8C";
        a['M'] = "\xEE\x81\x8D";
        a['N'] = "\xEE\x81\x8E";
        a['O'] = "\xEE\x81\x8F";
        a['P'] = "\xEE\x81\x90";
        a['Q'] = "\xEE\x81\x91";
        a['R'] = "\xEE\x81\x92";
        a['S'] = "\xEE\x81\x93";
        a['T'] = "\xEE\x81\x94";
        a['U'] = "\xEE\x81\x95";
        a['V'] = "\xEE\x81\x96";
        a['W'] = "\xEE\x81\x97";
        a['X'] = "\xEE\x81\x98";
        a['Y'] = "\xEE\x81\x99";
        a['Z'] = "\xEE\x81\x9A";

        a['a'] = "\xEE\x81\x9F";
        a['b'] = "\xEE\x81\xA0";
        a['c'] = "\xEE\x81\xA1";
        a['d'] = "\xEE\x81\xA2";
        a['e'] = "\xEE\x81\xA3";
        a['f'] = "\xEE\x81\xA4";
        a['g'] = "\xEE\x81\xA5";
        a['h'] = "\xEE\x81\xA6";
        a['i'] = "\xEE\x81\xA7";
        a['j'] = "\xEE\x81\xA8";
        a['k'] = "\xEE\x81\xA9";
        a['l'] = "\xEE\x81\xAA";
        a['m'] = "\xEE\x81\xAB";
        a['n'] = "\xEE\x81\xAC";
        a['o'] = "\xEE\x81\xAD";
        a['p'] = "\xEE\x81\xAE";
        a['q'] = "\xEE\x81\xAF";
        a['r'] = "\xEE\x81\xB0";
        a['s'] = "\xEE\x81\xB1";
        a['t'] = "\xEE\x81\xB2";
        a['u'] = "\xEE\x81\xB3";
        a['v'] = "\xEE\x81\xB4";
        a['w'] = "\xEE\x81\xB5";
        a['x'] = "\xEE\x81\xB6";
        a['y'] = "\xEE\x81\xB7";
        a['z'] = "\xEE\x81\xB8";

        a['*'] = "\xEE\x81\xB9";
        a['+'] = "\xE2\x81\xBA";
        a['-'] = "\xE2\x81\xBB";
        a['='] = "\xE2\x81\xBC";
        a['('] = "\xE2\x81\xBD";
        a[')'] = "\xE2\x81\xBE";
        a['['] = "\xEE\x81\xBB";
        a[']'] = "\xEE\x81\xBC";
        a[','] = "\xEE\x81\x9B";
        a['.'] = "\xEE\x81\x9C";
        a['/'] = "\xEE\x81\x9D";

        return a;
    }();

    inline constexpr auto getSuperscriptCharacter(char c) -> const char * {
        unsigned uc = static_cast<unsigned char>(c);
        return (uc < superscriptCharacters.size()) ? superscriptCharacters[uc] : nullptr;
    }

    // Character to subscript code point LUT
    // 128-entry ASCII table; empty entries are nullptr.
    static constexpr auto subscriptCharacters = [] {
        std::array<const char *, 128> a{};

        a['0'] = "\xE2\x82\x80";
        a['1'] = "\xE2\x82\x81";
        a['2'] = "\xE2\x82\x82";
        a['3'] = "\xE2\x82\x83";
        a['4'] = "\xE2\x82\x84";
        a['5'] = "\xE2\x82\x85";
        a['6'] = "\xE2\x82\x86";
        a['7'] = "\xE2\x82\x87";
        a['8'] = "\xE2\x82\x88";
        a['9'] = "\xE2\x82\x89";

        a['A'] = "\xEE\x80\x85";
        a['B'] = "\xEE\x80\x86";
        a['C'] = "\xEE\x80\x87";
        a['D'] = "\xEE\x80\x88";
        a['E'] = "\xEE\x80\x89";
        a['F'] = "\xEE\x80\x8A";
        a['G'] = "\xEE\x80\x8B";
        a['H'] = "\xEE\x80\x8C";
        a['I'] = "\xEE\x80\x8D";
        a['J'] = "\xEE\x80\x8E";
        a['K'] = "\xEE\x80\x8F";
        a['L'] = "\xEE\x80\x90";
        a['M'] = "\xEE\x80\x91";
        a['N'] = "\xEE\x80\x92";
        a['O'] = "\xEE\x80\x93";
        a['P'] = "\xEE\x80\x94";
        a['Q'] = "\xEE\x80\x95";
        a['R'] = "\xEE\x80\x96";
        a['S'] = "\xEE\x80\x97";
        a['T'] = "\xEE\x80\x98";
        a['U'] = "\xEE\x80\x99";
        a['V'] = "\xEE\x80\x9A";
        a['W'] = "\xEE\x80\x9B";
        a['X'] = "\xEE\x80\x9C";
        a['Y'] = "\xEE\x80\x9D";
        a['Z'] = "\xEE\x80\x9E";

        a['a'] = "\xEE\x80\xA3";
        a['b'] = "\xEE\x80\xA4";
        a['c'] = "\xEE\x80\xA5";
        a['d'] = "\xEE\x80\xA6";
        a['e'] = "\xEE\x80\xA7";
        a['f'] = "\xEE\x80\xA8";
        a['g'] = "\xEE\x80\xA9";
        a['h'] = "\xEE\x80\xAA";
        a['i'] = "\xEE\x80\xAB";
        a['j'] = "\xEE\x80\xAC";
        a['k'] = "\xEE\x80\xAD";
        a['l'] = "\xEE\x80\xAE";
        a['m'] = "\xEE\x80\xAF";
        a['n'] = "\xEE\x80\xB0";
        a['o'] = "\xEE\x80\xB1";
        a['p'] = "\xEE\x80\xB2";
        a['q'] = "\xEE\x80\xB3";
        a['r'] = "\xEE\x80\xB4";
        a['s'] = "\xEE\x80\xB5";
        a['t'] = "\xEE\x80\xB6";
        a['u'] = "\xEE\x80\xB7";
        a['v'] = "\xEE\x80\xB8";
        a['w'] = "\xEE\x80\xB9";
        a['x'] = "\xEE\x80\xBA";
        a['y'] = "\xEE\x80\xBB";
        a['z'] = "\xEE\x80\xBC";

        a['*'] = "\xEE\x80\xBD";
        a['+'] = "\xE2\x82\x8A";
        a['-'] = "\xE2\x82\x8B";
        a['='] = "\xE2\x82\x8C";
        a['('] = "\xE2\x82\x8D";
        a[')'] = "\xE2\x82\x8E";
        a['['] = "\xEE\x80\xBF";
        a[']'] = "\xEE\x81\x80";
        a[','] = "\xEE\x80\x9F";
        a['.'] = "\xEE\x80\xA0";
        a['/'] = "\xEE\x80\xA1";

        return a;
    }();

    inline constexpr auto getSubscriptCharacter(char c) -> const char * {
        unsigned uc = static_cast<unsigned char>(c);
        return (uc < subscriptCharacters.size()) ? subscriptCharacters[uc] : nullptr;
    }
};

#endif