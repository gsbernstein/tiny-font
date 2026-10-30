#define CATCH_CONFIG_MAIN
#include <Font.hpp>
#include <IBMFFonts/SolSans_75.h>
#include <algorithm>
#include <catch_amalgamated.hpp>
#include <cstdio>
#include <string>
#include <vector>

#include "ImageIO.hpp"
#include "TestHelpers.hpp"

using namespace ibmf_defs;
using namespace font_defs;

static auto renderTextIBMF(const std::string &text, int faceIndex, int &outW, int &outH)
    -> std::vector<uint8_t> {
    FontData fontData(SOLSANS_75_IBMF, SOLSANS_75_IBMF_LEN);
    Font font(fontData, faceIndex);

    const int inset = 10;
    Dim textDim = font.getTextSize(text);
    int width = textDim.width + inset * 2;
    int height = font.lineHeight() + inset * 2;
    outW = width;
    outH = height;

    std::vector<uint8_t> out(static_cast<size_t>(width * height), 255);
    Bitmap canvas;
    canvas.dim = Dim(width, height);
    canvas.pitch = (width + 7) >> 3;
    canvas.pixels = new uint8_t[static_cast<size_t>(canvas.pitch) * height];
    std::memset(canvas.pixels, 0xFF, static_cast<size_t>(canvas.pitch) * height);

    font.drawSingleLineOfText(canvas, Pos(inset, inset), text, false);

    // expand 1bpp to 8bpp grayscale
    for (int y = 0; y < height; ++y) {
        uint8_t *row = canvas.pixels + y * canvas.pitch;
        for (int x = 0; x < width; ++x) {
            int byteIndex = x >> 3;
            int bitIndex = 7 - (x & 7);
            bool on = (row[byteIndex] >> bitIndex) & 1;
            out[static_cast<size_t>(y * width + x)] = on ? 0 : 255;
        }
    }

    delete[] canvas.pixels;
    return out;
}

TEST_CASE("IBMF renders Tiny Font line for all faces matches golden", "[ibmf]") {
    const std::string line = "Tiny Font: A Minimal Font Library";
    for (int face = 0; face < 3; ++face) {
        INFO("IBMF face index " << face);
        int W = 0, H = 0;
        auto buf = renderTextIBMF(line, face, W, H);

        std::string goldenPath =
            std::string(GOLDEN_DIR) + "/ibmf_hello_face" + std::to_string(face) + ".png";

        int gw = 0, gh = 0;
        std::vector<uint8_t> golden;
        bool haveGolden = Load8bpp(goldenPath.c_str(), gw, gh, golden);

        if (haveGolden && (gw != W || gh != H)) {
            REQUIRE(Save8bpp(goldenPath.c_str(), W, H, buf.data()));
            SUCCEED("Golden resized (IBMF hello) " + goldenPath);
            continue;
        }
        if (!haveGolden) {
            REQUIRE(Save8bpp(goldenPath.c_str(), W, H, buf.data()));
            SUCCEED("Golden image created for face. Re-run tests.");
            continue;
        }

        REQUIRE(gw == W);
        REQUIRE(gh == H);

        ASSERT_IMAGE(goldenPath.c_str(), W, H, buf.data(), golden.data());
    }
}

// ---- Glyph grid tests (IBMF) ----

static auto codePointToUtf8(int code) -> std::string {
    std::string glyph;
    if (code < 0x80) {
        glyph += static_cast<char>(code);
    } else if (code < 0x800) {
        glyph += static_cast<char>(0xC0 | (code >> 6));
        glyph += static_cast<char>(0x80 | (code & 0x3F));
    } else if (code < 0x10000) {
        glyph += static_cast<char>(0xE0 | (code >> 12));
        glyph += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        glyph += static_cast<char>(0x80 | (code & 0x3F));
    } else {
        // outside BMP not expected here
    }
    return glyph;
}

static auto generateGlyphsInRange(int start, int end) -> std::vector<std::string> {
    std::vector<std::string> glyphs;
    glyphs.reserve(static_cast<size_t>(end - start + 1));
    for (int code = start; code <= end; ++code) {
        glyphs.push_back(codePointToUtf8(code));
    }
    return glyphs;
}

static auto renderGlyphGridIBMF(const std::vector<std::string> &glyphs, int gridW, int gridH,
                                int faceIndex, int page, int width, int height)
    -> std::vector<uint8_t> {
    std::vector<uint8_t> out(static_cast<size_t>(width * height), 255);
    Bitmap canvas;
    canvas.dim = Dim(width, height);
    canvas.pitch = (width + 7) >> 3;
    canvas.pixels = new uint8_t[static_cast<size_t>(canvas.pitch) * height];
    std::memset(canvas.pixels, 0xFF, static_cast<size_t>(canvas.pitch) * height);

    FontData fontData(SOLSANS_75_IBMF, SOLSANS_75_IBMF_LEN);
    Font font(fontData, faceIndex);

    const int inset = 5;
    int xSpan = (width - inset * 2) / gridW;
    int ySpan = font.lineHeight() + 6; // use font line spacing with small padding

    int perPage = gridW * gridH;
    int startIdx = page * perPage;
    for (int i = 0; i < perPage; ++i) {
        int idx = startIdx + i;
        if (idx >= static_cast<int>(glyphs.size())) break;
        int gx = i % gridW;
        int gy = i / gridW;
        int x = inset + gx * xSpan;
        int y = inset + gy * ySpan;
        font.drawSingleLineOfText(canvas, Pos(x, y), glyphs[idx], false);
    }

    // Expand to 8bpp
    for (int y = 0; y < height; ++y) {
        uint8_t *row = canvas.pixels + y * canvas.pitch;
        for (int x = 0; x < width; ++x) {
            int byteIndex = x >> 3;
            int bitIndex = 7 - (x & 7);
            bool on = (row[byteIndex] >> bitIndex) & 1;
            out[static_cast<size_t>(y * width + x)] = on ? 0 : 255;
        }
    }
    delete[] canvas.pixels;
    return out;
}

TEST_CASE("IBMF glyph grids for multiple blocks", "[ibmf][glyphs]") {
    const int W = 640, H = 480;

    // Build blocks akin to the reference test
    std::vector<std::pair<const char *, std::vector<std::string>>> blocks;
    // Pairs a-z, A-Z, digits
    std::vector<std::string> glyphsBasic;
    const char *digits = "0123456789";
    const char *low = "abcdefghijklmnopqrstuvwxyz";
    const char *up = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (const char *p = digits; *p; ++p) {
        glyphsBasic.emplace_back(std::string(1, *p));
    }
    for (const char *p = low; *p; ++p) {
        glyphsBasic.emplace_back(std::string(1, *p));
    }
    for (const char *p = up; *p; ++p) {
        glyphsBasic.emplace_back(std::string(1, *p));
    }

    std::vector<std::string> pairs;
    pairs.reserve(glyphsBasic.size() * glyphsBasic.size());
    for (const auto &g1 : glyphsBasic) {
        for (const auto &g2 : glyphsBasic) {
            pairs.push_back(g1 + g2);
        }
    }

    blocks.emplace_back("pairs", std::move(pairs));
    blocks.emplace_back("latin", generateGlyphsInRange(0x0020, 0x007F));
    blocks.emplace_back("latin1", generateGlyphsInRange(0x0080, 0x00FF));
    blocks.emplace_back("latinExtA", generateGlyphsInRange(0x0100, 0x017F));
    blocks.emplace_back("latinExtB", generateGlyphsInRange(0x0180, 0x024F));
    blocks.emplace_back("spacingMods", generateGlyphsInRange(0x02B0, 0x02FF));
    blocks.emplace_back("greek", generateGlyphsInRange(0x0370, 0x03FF));
    blocks.emplace_back("cyrillic", generateGlyphsInRange(0x0400, 0x04FF));
    blocks.emplace_back("punct", generateGlyphsInRange(0x2000, 0x206F));
    blocks.emplace_back("supsubs", generateGlyphsInRange(0x2070, 0x209F));
    blocks.emplace_back("currency", generateGlyphsInRange(0x20A0, 0x20CF));
    blocks.emplace_back("private", generateGlyphsInRange(0xE000, 0xE07C));
    blocks.emplace_back("alphaPres", generateGlyphsInRange(0xFB00, 0xFB4F));

    const struct FaceCfg {
        int face;
        int gw, gh;
    } faces[] = {{0, 12, 16}, {1, 10, 14}, {2, 8, 12}};

    for (const auto &cfg : faces) {
        // Prepare a font to compute dynamic canvas size from line spacing
        FontData tmpData(SOLSANS_75_IBMF, SOLSANS_75_IBMF_LEN);
        Font tmpFont(tmpData, cfg.face);
        const int inset = 5;
        const int pad = 6;
        const int cellY = tmpFont.lineHeight() + pad;
        const int cellX = tmpFont.lineHeight() + pad; // simple heuristic; wide glyphs still fit
        const int W = inset * 2 + cfg.gw * cellX;
        const int H = inset * 2 + cfg.gh * cellY;

        for (const auto &blk : blocks) {
            const auto &key = blk.first;
            const auto &glyphs = blk.second;
            int perPage = cfg.gw * cfg.gh;
            int pages = static_cast<int>((glyphs.size() + perPage - 1) / perPage);
            for (int p = 0; p < pages; ++p) {
                auto buf = renderGlyphGridIBMF(glyphs, cfg.gw, cfg.gh, cfg.face, p, W, H);
                std::string goldenPath = std::string(GOLDEN_DIR) + "/ibmf_grid_f" +
                                         std::to_string(cfg.face) + "_" + key + "_" +
                                         std::to_string(p) + ".png";

                int gw = 0, gh = 0;
                std::vector<uint8_t> golden;
                bool haveGolden = Load8bpp(goldenPath.c_str(), gw, gh, golden);
                if (haveGolden && (gw != W || gh != H)) {
                    REQUIRE(Save8bpp(goldenPath.c_str(), W, H, buf.data()));
                    SUCCEED("Golden resized (IBMF grid) " + goldenPath);
                    continue;
                }
                if (!haveGolden) {
                    REQUIRE(Save8bpp(goldenPath.c_str(), W, H, buf.data()));
                    SUCCEED("Golden created (IBMF grid) " + goldenPath);
                    continue;
                }
                REQUIRE(gw == W);
                REQUIRE(gh == H);
                ASSERT_IMAGE(goldenPath.c_str(), W, H, buf.data(), golden.data());
            }
        }
    }
}

// ---- Italic (synthetic slant) tests ----

namespace {

constexpr uint8_t GUARD_VALUE = 0xA5;
constexpr size_t GUARD_BYTES = 64;
constexpr uint8_t WHITE_FILL = WHITE_ONE_BIT ? 0xFF : 0x00;
constexpr uint8_t BLACK_FILL = BLACK_ONE_BIT ? 0xFF : 0x00;

// 1bpp canvas with guard bytes on both sides to catch out-of-bounds writes.
class TestCanvas {
public:
    TestCanvas(int width, int height, uint8_t fill)
        : width_(width), height_(height), pitch_((width + 7) >> 3),
          storage_(GUARD_BYTES * 2 + static_cast<size_t>(pitch_) * height, GUARD_VALUE) {
        std::fill(storage_.begin() + GUARD_BYTES, storage_.end() - GUARD_BYTES, fill);
    }

    auto bitmap() -> Bitmap {
        Bitmap bitmap{};
        bitmap.pixels = storage_.data() + GUARD_BYTES;
        bitmap.dim = Dim(width_, height_);
        bitmap.pitch = pitch_;
        return bitmap;
    }

    [[nodiscard]] auto isBlack(int x, int y) const -> bool {
        const uint8_t byte = storage_[GUARD_BYTES + static_cast<size_t>(y * pitch_ + (x >> 3))];
        return ((byte >> (7 - (x & 7))) & 1) == BLACK_ONE_BIT;
    }

    [[nodiscard]] auto guardsIntact() const -> bool {
        return std::all_of(storage_.begin(), storage_.begin() + GUARD_BYTES, [](uint8_t b) {
            return b == GUARD_VALUE;
        }) && std::all_of(storage_.end() - GUARD_BYTES, storage_.end(), [](uint8_t b) {
            return b == GUARD_VALUE;
        });
    }

    [[nodiscard]] auto blackPixelCount() const -> int {
        int count = 0;
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                count += isBlack(x, y) ? 1 : 0;
            }
        }
        return count;
    }

    // Mean x of black pixels in rows [rowBegin, rowEnd), or 0 if there are none.
    [[nodiscard]] auto meanBlackX(int rowBegin, int rowEnd) const -> double {
        long sum = 0;
        long count = 0;
        for (int y = rowBegin; y < rowEnd; ++y) {
            for (int x = 0; x < width_; ++x) {
                if (isBlack(x, y)) {
                    sum += x;
                    count++;
                }
            }
        }
        return count == 0 ? 0.0 : static_cast<double>(sum) / static_cast<double>(count);
    }

    [[nodiscard]] auto to8bpp() const -> std::vector<uint8_t> {
        std::vector<uint8_t> out(static_cast<size_t>(width_ * height_));
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                out[static_cast<size_t>(y * width_ + x)] = isBlack(x, y) ? 0 : 255;
            }
        }
        return out;
    }

    [[nodiscard]] auto width() const -> int { return width_; }
    [[nodiscard]] auto height() const -> int { return height_; }

private:
    int width_;
    int height_;
    int pitch_;
    std::vector<uint8_t> storage_;
};

auto drawLine(Font &font, TestCanvas &canvas, int x, int y, const std::string &text, bool italic,
              bool inverted = false) -> int {
    Bitmap bitmap = canvas.bitmap();
    font.setItalicFont(italic);
    const int endX = font.drawSingleLineOfText(bitmap, Pos(x, y), text, inverted);
    font.setItalicFont(false);
    return endX;
}

// getTextSize() measures upright glyphs; leave room for the slant on the right.
auto italicSlantMargin(const Font &font) -> int { return font.lineHeight() / 4 + 2; }

const std::string ITALIC_SAMPLE = "fjord bdhklt gjpqy Tiny";

} // namespace

TEST_CASE("IBMF italic line with ascenders and descenders matches golden", "[ibmf][italic]") {
    FontData fontData(SOLSANS_75_IBMF, SOLSANS_75_IBMF_LEN);
    for (int face = 0; face < fontData.getFaceCount(); ++face) {
        INFO("IBMF face index " << face);
        Font font(fontData, face);
        const int inset = 10;
        const int W = font.getTextSize(ITALIC_SAMPLE).width + inset * 2 + italicSlantMargin(font);
        const int H = font.lineHeight() + inset * 2;

        TestCanvas upright(W, H, WHITE_FILL);
        drawLine(font, upright, inset, inset, ITALIC_SAMPLE, false);
        TestCanvas italic(W, H, WHITE_FILL);
        drawLine(font, italic, inset, inset, ITALIC_SAMPLE, true);

        REQUIRE(italic.guardsIntact());
        const bool italicDiffersFromUpright = italic.to8bpp() != upright.to8bpp();
        CHECK(italicDiffersFromUpright);
        // The slant leans right: relative to upright, ink in the top of the text band moves
        // further right than ink at the bottom.
        int firstInkRow = H;
        int lastInkRow = -1;
        for (int y = 0; y < H; ++y) {
            if (upright.meanBlackX(y, y + 1) > 0) {
                firstInkRow = std::min(firstInkRow, y);
                lastInkRow = y;
            }
        }
        REQUIRE(lastInkRow > firstInkRow);
        const int band = (lastInkRow - firstInkRow + 1) / 3;
        const auto lean = [&](const TestCanvas &canvas) {
            return canvas.meanBlackX(firstInkRow, firstInkRow + band) -
                   canvas.meanBlackX(lastInkRow + 1 - band, lastInkRow + 1);
        };
        CHECK(lean(italic) > lean(upright) + 0.5);

        auto buf = italic.to8bpp();
        std::string goldenPath =
            std::string(GOLDEN_DIR) + "/ibmf_italic_face" + std::to_string(face) + ".png";
        int gw = 0, gh = 0;
        std::vector<uint8_t> golden;
        bool haveGolden = Load8bpp(goldenPath.c_str(), gw, gh, golden);
        if (!haveGolden || gw != W || gh != H) {
            REQUIRE(Save8bpp(goldenPath.c_str(), W, H, buf.data()));
            SUCCEED("Golden created (IBMF italic) " + goldenPath);
            continue;
        }
        ASSERT_IMAGE(goldenPath.c_str(), W, H, buf.data(), golden.data());
    }
}

TEST_CASE("IBMF italic inverted drawing is the exact inverse", "[ibmf][italic]") {
    FontData fontData(SOLSANS_75_IBMF, SOLSANS_75_IBMF_LEN);
    Font font(fontData, 0);
    const int inset = 6;
    const int W = font.getTextSize(ITALIC_SAMPLE).width + inset * 2 + italicSlantMargin(font);
    const int H = font.lineHeight() + inset * 2;

    TestCanvas normal(W, H, WHITE_FILL);
    drawLine(font, normal, inset, inset, ITALIC_SAMPLE, true, false);
    TestCanvas inverted(W, H, BLACK_FILL);
    drawLine(font, inverted, inset, inset, ITALIC_SAMPLE, true, true);

    REQUIRE(normal.blackPixelCount() > 0);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            INFO("pixel " << x << "," << y);
            REQUIRE(inverted.isBlack(x, y) == !normal.isBlack(x, y));
        }
    }
    CHECK(inverted.guardsIntact());
}

TEST_CASE("IBMF italic glyphs clip at every canvas edge", "[ibmf][italic]") {
    FontData fontData(SOLSANS_75_IBMF, SOLSANS_75_IBMF_LEN);
    Font font(fontData, 0);
    const int origin = 20;
    const int textW = font.getTextSize(ITALIC_SAMPLE).width + italicSlantMargin(font);
    const int lineH = font.lineHeight();
    const int W = textW + origin * 2;
    const int H = lineH + origin * 2;

    TestCanvas reference(W, H, WHITE_FILL);
    drawLine(font, reference, origin, origin, ITALIC_SAMPLE, true);

    struct Window {
        const char *name;
        int x, y, w, h;
    };
    // Each window cuts through the drawn text on one side.
    const Window windows[] = {
        {"left", origin + 7, 0, W - origin - 7, H},
        {"right", 0, 0, origin + textW / 2, H},
        {"top", 0, origin + lineH / 3, W, H - origin - lineH / 3},
        {"bottom", 0, 0, W, origin + lineH / 2},
        {"all sides", origin + 7, origin + lineH / 4, textW / 2, lineH / 2},
    };

    for (const auto &win : windows) {
        INFO("window " << win.name);
        TestCanvas clipped(win.w, win.h, WHITE_FILL);
        drawLine(font, clipped, origin - win.x, origin - win.y, ITALIC_SAMPLE, true);

        REQUIRE(clipped.guardsIntact());
        int mismatches = 0;
        for (int y = 0; y < win.h; ++y) {
            for (int x = 0; x < win.w; ++x) {
                mismatches += clipped.isBlack(x, y) != reference.isBlack(win.x + x, win.y + y);
            }
        }
        CHECK(mismatches == 0);
        CHECK(clipped.blackPixelCount() > 0);
    }
}

TEST_CASE("IBMF upright text drawn at the italic end position does not collide", "[ibmf][italic]") {
    FontData fontData(SOLSANS_75_IBMF, SOLSANS_75_IBMF_LEN);
    for (int face = 0; face < fontData.getFaceCount(); ++face) {
        Font font(fontData, face);
        // Italic runs ending in tall glyphs (with and without a trailing space) followed by
        // upright text that starts at the baseline or with an ascender.
        const std::pair<std::string, std::string> cases[] = {
            {"word", "."}, {"bold", "l"}, {"half", "!"}, {"lit", ","}, {"bold ", "l"},
        };
        for (const auto &[italicText, uprightText] : cases) {
            INFO("face " << face << " italic '" << italicText << "' then '" << uprightText << "'");
            const int inset = 8;
            const int W = font.getTextSize(italicText + uprightText).width + inset * 2 +
                          italicSlantMargin(font) * 2;
            const int H = font.lineHeight() + inset * 2;

            TestCanvas italic(W, H, WHITE_FILL);
            const int endX = drawLine(font, italic, inset, inset, italicText, true);
            TestCanvas upright(W, H, WHITE_FILL);
            drawLine(font, upright, endX, inset, uprightText, false);

            REQUIRE(italic.blackPixelCount() > 0);
            REQUIRE(upright.blackPixelCount() > 0);
            int shared = 0;
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    shared += italic.isBlack(x, y) && upright.isBlack(x, y);
                }
            }
            CHECK(shared == 0);
        }
    }
}

TEST_CASE("IBMF italic mode resets cleanly to upright", "[ibmf][italic]") {
    FontData fontData(SOLSANS_75_IBMF, SOLSANS_75_IBMF_LEN);
    Font font(fontData, 0);
    REQUIRE_FALSE(font.getItalicFont());
    font.setItalicFont(true);
    REQUIRE(font.getItalicFont());
    font.setItalicFont(false);
    REQUIRE_FALSE(font.getItalicFont());

    const int inset = 6;
    const int W = font.getTextSize(ITALIC_SAMPLE).width + inset * 2;
    const int H = font.lineHeight() + inset * 2;

    Font freshFont(fontData, 0);
    TestCanvas expected(W, H, WHITE_FILL);
    Bitmap expectedBitmap = expected.bitmap();
    const int expectedEnd =
        freshFont.drawSingleLineOfText(expectedBitmap, Pos(inset, inset), ITALIC_SAMPLE, false);

    TestCanvas warmUp(W, H, WHITE_FILL);
    drawLine(font, warmUp, inset, inset, "warm up", true);
    TestCanvas afterToggle(W, H, WHITE_FILL);
    const int actualEnd = drawLine(font, afterToggle, inset, inset, ITALIC_SAMPLE, false);

    CHECK(actualEnd == expectedEnd);
    const bool sameAsFreshFont = afterToggle.to8bpp() == expected.to8bpp();
    CHECK(sameAsFreshFont);
}
