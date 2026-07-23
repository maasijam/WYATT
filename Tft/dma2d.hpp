#pragma once
#include "ui_driver.hpp"

#define COLOR565(r, g, b)                                  \
    ((uint16_t(r & 0xF8) << 8) | (uint16_t(g & 0xFC) << 3) \
     | (uint16_t(b & 0xF8) >> 3))

class Dma2DHandle
{
  public:
    void Init(uint8_t* buffer_);
    void FillRect(const Rectangle& rect, uint8_t color, uint8_t alpha = 255);

    void WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint8_t color);

    uint16_t tftPalette[NUMBER_OF_TFT_COLORS];

    void InitPalette()
    {
        // HEX to RBG565 converter: https://trolsoft.ru/en/articles/rgb565-color-picker
        tftPalette[COLOR_BLACK]       = 0x0000;
        tftPalette[COLOR_WHITE]       = 0xffff;
        tftPalette[COLOR_BLUE]        = COLOR565(0x5A, 0x5D, 0xFF);
        tftPalette[COLOR_DARK_BLUE]   = COLOR565(0x19, 0x1C, 0x5A);
        tftPalette[COLOR_YELLOW]      = COLOR565(0xFF, 0xFF, 0x00);
        tftPalette[COLOR_DARK_YELLOW] = COLOR565(0x4A, 0x3D, 0x08);
        tftPalette[COLOR_RED]         = 0xF800;                     // 0xff4010
        tftPalette[COLOR_DARK_RED]    = COLOR565(0x40, 0x10, 0x00); // 0x401000
        tftPalette[COLOR_GREEN]       = 0x07E0;                     // 0x40ff40
        tftPalette[COLOR_DARK_GREEN]  = COLOR565(0x00, 0x40, 0x00); // 0x004000
        tftPalette[COLOR_LIGHT_GRAY]  = COLOR565(0xb0, 0xb0, 0xb0); // 0xb0b0b0
        tftPalette[COLOR_MEDIUM_GRAY] = COLOR565(0x90, 0x90, 0x90); // 0x909090
        tftPalette[COLOR_GRAY]        = COLOR565(0x60, 0x60, 0x60); // 0x606060
        tftPalette[COLOR_DARK_GRAY]   = COLOR565(0x30, 0x30, 0x30); // 0x303030
        tftPalette[COLOR_CYAN]        = 0x76FD;                     // 0x76dfef
        tftPalette[COLOR_ORANGE]      = 0xFBE0;                     // 0xff7f00
        tftPalette[COLOR_LIGHT_GREEN] = 0x6FED;                     // 0x70ff70
        // tftPalette[COLOR_ABL_BG]      = (0x2104); // 0x212121
        tftPalette[COLOR_ABL_BG]     = 0x4A69; // 0x4D4D4D
        tftPalette[COLOR_ABL_LINE]   = 0x39E7; // 0x3d3d3d
        tftPalette[COLOR_ABL_D_LINE] = 0x31A6; // 0x363636
        tftPalette[COLOR_ABL_L_GRAY] = 0x52AA; // 0x555555
        tftPalette[COLOR_ABL_M_GRAY] = 0x4228; // 0x454545
    }

    class Impl;
    Impl* impl;
};