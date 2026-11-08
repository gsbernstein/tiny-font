# Tiny Font

![Header Image](https://github.com/sindarin-inc/tiny-font/blob/main/tests/Images/ibmf_hello_face2.png)

A font system for embedded platforms. Originally built for the [Sol Reader](https://solreader.com) E-Reader glasses. Currently compiles for ESP32* devices as an [ESP-IDF](https://github.com/espressif/esp-idf) component but could probably be made to compile for other embedded platforms without too much pain.

The main reason this library exists is to provide a high-quality 1-bit-per-pixel (1bpp) bitmap font (SolSans) with kerning and ligature support. This is used with Sol Reader's monochrome EInk displays. This uses a bitmap font format called IBMF created by [@turgu1](https://github.com/turgu1).

TTF support (via [FreeType](https://freetype.org/)) is also included to support integrating with higher-resolution grayscale displays. It uses a similar API as the 1bpp so the two font systems can be swapped relatively simply.

## Goals

* Embedded friendly - very small binary with included font for use on embedded platforms.
* 1bpp font support with ligatures and kerning.
* TTF vector font support for grayscale and higher density displays.

## IBMF synthetic italics

All three SolSans sizes use a baseline-anchored, one-pixel shift per three rows:
`shift(h) = floor(h / 3)`, where `h` is the row height above the baseline.
Rows 0-2 stay unshifted, rows 3-5 shift right one pixel, and rows -1 through -3
shift left one pixel. The row placement is intentional rather than nearest-pixel
angle rounding. Upright kerning and advances remain unchanged; the returned
italic end position includes a correction using the same staircase.

SolSans medium can additionally use 20 hand-edited italic glyphs by constructing
`Font(data, 1, true)`. This option must only be enabled for `SolSans_75` face 1.
Other faces and glyphs retain the synthetic staircase, and upright text is unaffected.
Custom glyphs keep the original metrics, ligatures and kerning; end-of-run spacing
also accounts for their rightmost ink.

The editable source is `src/IBMFFonts/SolSans_75_MediumItalic.json`. Regenerate the
compact row-mask header with `node scripts/generate-italic-overrides.mjs`, then
format the generated header with clang-format.

### Preserved glyph lab

The reproducible medium glyph editor lives in [scripts/italic-lab](scripts/italic-lab).
From this repository's root:

```sh
cmake -S scripts/italic-lab -B scripts/italic-lab/build
cmake --build scripts/italic-lab/build --target italic-lab
scripts/italic-lab/build/italic-lab scripts/italic-lab/build
```

Open `scripts/italic-lab/build/italic-glyph-editor.html` in a browser. It is
self-contained and can be saved elsewhere. The generator loads the committed
medium italic JSON by default; an optional second argument loads another export.
It uses the current upright SolSans font and verifies its sample pixels and
positions against the native synthetic renderer before writing the page.
The samples do not cover every pair or ligature combination.

Edit pixels, inspect native-size and enlarged context samples, then use
**Export custom glyphs** to copy the JSON back into
`src/IBMFFonts/SolSans_75_MediumItalic.json`. Browser edits are previews only and
are lost on reload unless exported. Regenerate and format the header afterward,
review the italic golden images, and rerun the IBMF italic tests:

```sh
cmake --build tests/build --target tests_ibmf
tests/build/tests_ibmf '[italic]'
```

The lab needs a C++17 compiler and CMake. It reuses the sibling `cpp-misc`
checkout in embedded-app; standalone checkouts fetch the same dependency used
by this repository's tests. No display server, TTF stack or image library is needed.

### When changing the upright font

Custom italic bitmaps are independent of the upright glyphs: they do not
automatically follow edits to `SolSans_75.h`. After changing medium glyph shapes,
baseline/offsets, advances, or kerning, rebuild the lab and review the custom
italics as well. Keep the JSON coordinate origin/baseline consistent with the
renderer and generator; other font sizes need their own visual review.

No custom kerning pairs or advances are stored in the italic JSON. The overrides
inherit those values from the upright font. "Review spacing" means checking
neighbouring ink in context, not maintaining a separate kerning table.
Only the end-of-run italic correction additionally accounts for custom rightmost
ink so following upright text is not crowded. Check mixed italic/upright runs,
punctuation, capitals and digits after edits, as well as clipping and inversion.

## See also

* [IBMF Font Editor app](https://github.com/turgu1/IBMFFontEditor)
* [IBMF Font Generator](https://github.com/turgu1/IBMF-font-generator)
