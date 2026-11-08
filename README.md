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

## See also

* [IBMF Font Editor app](https://github.com/turgu1/IBMFFontEditor)
* [IBMF Font Generator](https://github.com/turgu1/IBMF-font-generator)
