#pragma once

#include "ui_driver.hpp"

using namespace daisy;

#include "ili9341_transport.hpp"
#include "dma2d.hpp"

/**
 * A driver implementation for the ILI9341
 */
class ILI9341UiDriver : public _UiDriver
{
  public:
    using _UiDriver::DrawRect;
    using _UiDriver::WriteString;

    virtual ~ILI9341UiDriver() {}

    void Init(uint8_t *buffer) override
    {
        screen_update_period_ = 24; // 17 is roughly 60Hz
        screen_update_last_   = System::GetNow();
        localbuffer = buffer;

        InitDriver(Orientation::RRight,localbuffer);
        Start();
        //dma2d_.Init(transport_.frame_buffer);
        dma2d_.Init(transport_.frame_buffer);
    }

    uint32_t Time() override { return transport_.update_time; }

    void DrawLine(uint16_t x1,
                  uint16_t y1,
                  uint16_t x2,
                  uint16_t y2,
                  uint8_t  color,
                  uint8_t  alpha = 255) override
    {
        if(x1 == x2)
        {
            return DrawVLine(x1, y1, y2 - y1 + 1, color, alpha);
        }
        else if(y1 == y2)
        {
            return DrawHLine(x1, y1, x2 - x1 + 1, color, alpha);
        }

        auto deltaX = abs((int_fast16_t)x2 - (int_fast16_t)x1);
        auto deltaY = abs((int_fast16_t)y2 - (int_fast16_t)y1);
        auto signX  = ((x1 < x2) ? 1 : -1);
        auto signY  = ((y1 < y2) ? 1 : -1);
        auto error  = deltaX - deltaY;

        DrawPixel(x2, y2, color, alpha);

        while((x1 != x2) || (y1 != y2))
        {
            DrawPixel(x1, y1, color, alpha);
            auto error2 = error * 2;
            if(error2 > -deltaY)
            {
                error -= deltaY;
                x1 += signX;
            }

            if(error2 < deltaX)
            {
                error += deltaX;
                y1 += signY;
            }
        }
    }

    void DrawRect(uint16_t x,
                  uint16_t y,
                  uint16_t w,
                  uint16_t h,
                  uint8_t  color,
                  uint8_t  alpha = 255) override
    {
        auto x2 = x + w;
        auto y2 = y + h;
        DrawLine(x, y, x, y2, color, alpha);
        DrawLine(x, y, x2, y, color, alpha);
        DrawLine(x, y2, x2, y2, color, alpha);
        DrawLine(x2, y, x2, y2, color, alpha);
    }

    void
    FillRect(const Rectangle& rect, uint8_t color, uint8_t alpha = 255) override
    {
        return dma2d_.FillRect(rect, color, alpha);

        // for(int16_t i = rect.GetX(); i < rect.GetRight(); i++)
        // {
        //     DrawVLine(i, rect.GetY(), rect.GetHeight(), color, alpha);
        // }
    };

    void DrawTriangle(int16_t x0,
                      int16_t y0,
                      int16_t x1,
                      int16_t y1,
                      int16_t x2,
                      int16_t y2,
                      uint8_t color,
                      uint8_t alpha = 255) override
    {
        DrawLine(x0, y0, x1, y1, color, alpha);
        DrawLine(x1, y1, x2, y2, color, alpha);
        DrawLine(x2, y2, x0, y0, color, alpha);
    }

    void FillTriangle(int16_t x0,
                      int16_t y0,
                      int16_t x1,
                      int16_t y1,
                      int16_t x2,
                      int16_t y2,
                      uint8_t color,
                      uint8_t alpha = 255) override
    {
        int16_t a, b, y, last;

        // Sort coordinates by Y order (y2 >= y1 >= y0)
        if(y0 > y1)
        {
            std::swap(y0, y1);
            std::swap(x0, x1);
        }
        if(y1 > y2)
        {
            std::swap(y2, y1);
            std::swap(x2, x1);
        }
        if(y0 > y1)
        {
            std::swap(y0, y1);
            std::swap(x0, x1);
        }

        if(y0 == y2)
        { // Handle awkward all-on-same-line case as its own thing
            a = b = x0;
            if(x1 < a)
                a = x1;
            else if(x1 > b)
                b = x1;
            if(x2 < a)
                a = x2;
            else if(x2 > b)
                b = x2;
            DrawHLine(a, y0, b - a + 1, color, alpha);
            return;
        }

        int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
                dx12 = x2 - x1, dy12 = y2 - y1;
        int32_t sa = 0, sb = 0;

        // For upper part of triangle, find scanline crossings for segments
        // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
        // is included here (and second loop will be skipped, avoiding a /0
        // error there), otherwise scanline y1 is skipped here and handled
        // in the second loop...which also avoids a /0 error here if y0=y1
        // (flat-topped triangle).
        if(y1 == y2)
        {
            last = y1; // Include y1 scanline
        }
        else
        {
            last = y1 - 1; // Skip it
        }

        for(y = y0; y <= last; y++)
        {
            a = x0 + sa / dy01;
            b = x0 + sb / dy02;
            sa += dx01;
            sb += dx02;
            /* longhand:
            a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
            */
            if(a > b)
            {
                std::swap(a, b);
            }
            DrawHLine(a, y, b - a + 1, color, alpha);
        }

        // For lower part of triangle, find scanline crossings for segments
        // 0-2 and 1-2.  This loop is skipped if y1=y2.
        sa = (int32_t)dx12 * (y - y1);
        sb = (int32_t)dx02 * (y - y0);
        for(; y <= y2; y++)
        {
            a = x1 + sa / dy12;
            b = x0 + sb / dy02;
            sa += dx12;
            sb += dx02;
            /* longhand:
                a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
                b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
            */
            if(a > b)
            {
                std::swap(a, b);
            }
            DrawHLine(a, y, b - a + 1, color, alpha);
        }
    }

    void WriteString(const char* str,
                     uint16_t    x,
                     uint16_t    y,
                     FontDef      font,
                     uint8_t     color) override
    {
        SetCursor(x, y);
        while(*str) // Write until null-byte
        {
            if(WriteChar(*str, font, color) != *str)
            {
                return; // Char could not be written
            }
            str++; // Next char
        }
    }
    

    uint16_t GetStringWidth(const char* str, FontDef font) const override
    {
        uint16_t font_width = 0;
        // Loop until null-byte
        while(*str)
        {
            font_width += font.FontWidth;
            str++;
        }

        return font_width;
    }

    void DrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color)
    {
        int16_t f     = 1 - r;
        int16_t ddF_x = 1;
        int16_t ddF_y = -2 * r;
        int16_t x     = 0;
        int16_t y     = r;

        DrawPixel(x0, y0 + r, color);
        DrawPixel(x0, y0 - r, color);
        DrawPixel(x0 + r, y0, color);
        DrawPixel(x0 - r, y0, color);

        while(x < y)
        {
            if(f >= 0)
            {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            DrawPixel(x0 + x, y0 + y, color);
            DrawPixel(x0 - x, y0 + y, color);
            DrawPixel(x0 + x, y0 - y, color);
            DrawPixel(x0 - x, y0 - y, color);
            DrawPixel(x0 + y, y0 + x, color);
            DrawPixel(x0 - y, y0 + x, color);
            DrawPixel(x0 + y, y0 - x, color);
            DrawPixel(x0 - y, y0 - x, color);
        }
    }

    void FillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color)
    {
        DrawLine(x0, y0 - r, x0, y0 + 1 * r + 1, color);
        FillCircleHelper(x0, y0, r, 3, 0, color);
    }

    void FillCircleHelper(int16_t x0,
                          int16_t y0,
                          int16_t r,
                          uint8_t cornername,
                          int16_t delta,
                          uint8_t color)
    {
        int16_t f     = 1 - r;
        int16_t ddF_x = 1;
        int16_t ddF_y = -2 * r;
        int16_t x     = 0;
        int16_t y     = r;

        delta++;

        while(x < y)
        {
            if(f >= 0)
            {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            if(cornername & 0x1)
            {
                DrawLine(
                    x0 + x, y0 - y, x0 + x, y0 - y + 2 * y + delta - 1, color);
                DrawLine(
                    x0 + y, y0 - x, x0 + y, y0 - x + 2 * x + delta - 1, color);
            }
            if(cornername & 0x2)
            {
                DrawLine(
                    x0 - x, y0 - y, x0 - x, y0 - y + 2 * y + delta - 1, color);
                DrawLine(
                    x0 - y, y0 - x, x0 - y, y0 - x + 2 * x + delta - 1, color);
            }
        }
    }

   

    void Update() override
    {
        transport_.SendDataDMA();
        UpdateFrameRate();
    }

    bool IsRender() override
    {
        if(transport_.dma_busy == false)
        {
            // diff = System::GetNow() - screen_update_last_;
            // if(diff > screen_update_period_) {}
            // screen_update_last_ = System::GetNow();
            return true;
        }

        return false;
    }

    void UpdateFrameRate()
    {
        ++frames;
        if(System::GetNow() - fps_update_last_ > 1000)
        {
            fps              = frames;
            frames           = 0;
            fps_update_last_ = System::GetNow();
        }
    }

    void DrawPixel(uint_fast16_t x,
                   uint_fast16_t y,
                   uint8_t       color,
                   uint8_t       alpha = 255)
    {
        if(x >= width || y >= height)
            return;

        auto id = 2 * (x + y * width);

        // NOTE: Probably we should check the color id before accessing the array
        transport_.PaintPixel(id, color, alpha);

        // Lets divide the whole screen in 10 sectors, 32 pixel high each
        // uint8_t screen_sector     = y / 32;
        // dirty_buff[screen_sector] = 1;
    }

    uint16_t Fps() const override { return fps; }

  private:
    void Start() { transport_.SetAddressWindow(0, 0, width - 1, height - 1); }

    

    enum class Orientation
    {
        Default = 0,
        RRight,
        RLeft,
        UpsideDown,
    };

    // FIXME: Maybe use approach from https://github.com/MarlinFirmware/Marlin/blob/273cbc6871491a3c1c5eff017c3ccc5ce56bb123/Marlin/src/lcd/tft_io/ili9341.h#L140
    void InitDriver(Orientation ori = Orientation::RLeft,uint8_t *buffer = nullptr)
    {
        transport_.Init(buffer);

        SetOrientation(ori);

        transport_.Reset();

        //Software Reset
        transport_.SendCommand(0x01);
        // System::Delay(100); // TODO: maybe less?

        // command list is based on https://github.com/martnak/STM32-ILI9341

        // POWER CONTROL A
        transport_.SendCommand(0xCB);
        {
            uint8_t data[5] = {0x39, 0x2C, 0x00, 0x34, 0x02};
            transport_.SendData(data, 5);
        }

        // POWER CONTROL B
        transport_.SendCommand(0xCF);
        {
            uint8_t data[3] = {0x00, 0xC1, 0x30};
            transport_.SendData(data, 3);
        }

        // DRIVER TIMING CONTROL A
        transport_.SendCommand(0xE8);
        {
            uint8_t data[3] = {0x85, 0x00, 0x78};
            transport_.SendData(data, 3);
        }

        // DRIVER TIMING CONTROL B
        transport_.SendCommand(0xEA);
        {
            uint8_t data[2] = {0x00, 0x00};
            transport_.SendData(data, 2);
        }

        // POWER ON SEQUENCE CONTROL
        transport_.SendCommand(0xED);
        {
            uint8_t data[4] = {0x64, 0x03, 0x12, 0x81};
            transport_.SendData(data, 4);
        }

        // PUMP RATIO CONTROL
        transport_.SendCommand(0xF7);
        {
            uint8_t data[1] = {0x20};
            transport_.SendData(data, 1);
        }

        // POWER CONTROL,VRH[5:0]
        transport_.SendCommand(0xC0);
        {
            uint8_t data[1] = {0x23};
            transport_.SendData(data, 1);
        }

        // POWER CONTROL,SAP[2:0];BT[3:0]
        transport_.SendCommand(0xC1);
        {
            uint8_t data[1] = {0x10};
            transport_.SendData(data, 1);
        }

        // VCM CONTROL
        transport_.SendCommand(0xC5);
        {
            uint8_t data[2] = {0x3E, 0x28};
            transport_.SendData(data, 2);
        }

        // VCM CONTROL 2
        transport_.SendCommand(0xC7);
        {
            uint8_t data[1] = {0x86};
            transport_.SendData(data, 1);
        }

        // MEMORY ACCESS CONTROL
        transport_.SendCommand(0x36);
        {
            uint8_t data[1] = {0x48};
            transport_.SendData(data, 1);
        }

        // PIXEL FORMAT
        transport_.SendCommand(0x3A);
        {
            uint8_t data[1] = {0x55};
            transport_.SendData(data, 1);
        }

        // FRAME RATIO CONTROL, STANDARD RGB COLOR
        transport_.SendCommand(0xB1);
        {
            uint8_t data[2] = {0x00, 0x18};
            transport_.SendData(data, 2);
        }

        // DISPLAY FUNCTION CONTROL
        transport_.SendCommand(0xB6);
        {
            uint8_t data[3] = {0x08, 0x82, 0x27};
            transport_.SendData(data, 3);
        }

        // 3GAMMA FUNCTION DISABLE
        transport_.SendCommand(0xF2);
        {
            uint8_t data[1] = {0x00};
            transport_.SendData(data, 1);
        }

        // GAMMA CURVE SELECTED
        transport_.SendCommand(0x26);
        {
            uint8_t data[1] = {0x01};
            transport_.SendData(data, 1);
        }

        // POSITIVE GAMMA CORRECTION
        transport_.SendCommand(0xE0);
        {
            uint8_t data[15] = {0x0F,
                                0x31,
                                0x2B,
                                0x0C,
                                0x0E,
                                0x08,
                                0x4E,
                                0xF1,
                                0x37,
                                0x07,
                                0x10,
                                0x03,
                                0x0E,
                                0x09,
                                0x00};
            transport_.SendData(data, 15);
        }

        // NEGATIVE GAMMA CORRECTION
        transport_.SendCommand(0xE1);
        {
            uint8_t data[15] = {0x00,
                                0x0E,
                                0x14,
                                0x03,
                                0x11,
                                0x07,
                                0x31,
                                0xC1,
                                0x48,
                                0x08,
                                0x0F,
                                0x0C,
                                0x31,
                                0x36,
                                0x0F};
            transport_.SendData(data, 15);
        }

        // EXIT SLEEP
        transport_.SendCommand(0x11);
        System::Delay(10);

        // TURN ON DISPLAY
        transport_.SendCommand(0x29);

        // MADCTL
        transport_.SendCommand(0x36);
        System::Delay(10);

        {
            uint8_t data[1] = {rotation};
            transport_.SendData(data, 1);
        }
    };

    void SetOrientation(Orientation ori)
    {
        uint8_t ili_bgr = 0x08;
        uint8_t ili_mx  = 0x40;
        uint8_t ili_my  = 0x80;
        uint8_t ili_mv  = 0x20;
        switch(ori)
        {
            case Orientation::RRight:
            {
                width    = 320;
                height   = 240;
                rotation = ili_mx | ili_my | ili_mv | ili_bgr;
                return;
            }
            case Orientation::RLeft:
            {
                width    = 320;
                height   = 240;
                rotation = ili_mv | ili_bgr;
                return;
            }
            case Orientation::UpsideDown:
            {
                width    = 240;
                height   = 320;
                rotation = ili_my | ili_bgr;
                return;
            }
            default:
            {
                width    = 240;
                height   = 320;
                rotation = ili_mx | ili_bgr;
            };
        }
    }


    void DrawVLine(int16_t x,
                   int16_t y,
                   int16_t h,
                   uint8_t color,
                   uint8_t alpha = 255)
    {
        if(alpha == 255)
        {
            return dma2d_.FillRect(Rectangle(x, y, 1, h), color, alpha);
        }

        for(int16_t i = y; i < y + h; i++)
        {
            DrawPixel(x, i, color, alpha);
        }
    }

    void DrawHLine(int16_t x,
                   int16_t y,
                   int16_t w,
                   uint8_t color,
                   uint8_t alpha = 255)
    {
        if(alpha == 255)
        {
            return dma2d_.FillRect(Rectangle(x, y, w, 1), color, alpha);
        }
        for(int16_t i = x; i < x + w; i++)
        {
            DrawPixel(i, y, color, alpha);
        }
    }

    char WriteChar(char ch, FontDef font, uint8_t color)
    {
        // Check if character is valid
        if(ch < 32 || ch > 126)
            return 0;

        // Check remaining space on current line
        if(width < (currentX_ + font.FontWidth)
           || height < (currentY_ + font.FontHeight))
        {
            return 0;
        }

        // Use the font to write
        for(auto i = 0; i < font.FontHeight; i++)
        {
            auto b = font.data[(ch - 32) * font.FontHeight + i];
            for(auto j = 0; j < font.FontWidth; j++)
            {
                if((b << j) & 0x8000)
                {
                    DrawPixel(currentX_ + j, (currentY_ + i), color);
                }
            }
        }

        // dma2d_.WriteChar(ch, currentX_, currentY_, font, color);

        // The current space is now taken
        SetCursor(currentX_ + font.FontWidth, currentY_);

        return ch;
    }

    /**
     * @brief Moves the 'Cursor' position used for WriteChar, and WriteStr to the specified coordinate.
     * 
     * 
     * @param x x pos
     * @param y y pos
     */
    void SetCursor(uint16_t x, uint16_t y)
    {
        currentX_ = (x >= width) ? width - 1 : x;
        currentY_ = (y >= height) ? height - 1 : y;
    }


    /*!
    @brief   Given 8-bit red, green and blue values, return a 'packed'
             16-bit color value in '565' RGB format (5 bits red, 6 bits
             green, 5 bits blue). This is just a mathematical operation,
             no hardware is touched.
    @param   red    8-bit red brightnesss (0 = off, 255 = max).
    @param   green  8-bit green brightnesss (0 = off, 255 = max).
    @param   blue   8-bit blue brightnesss (0 = off, 255 = max).
    @return  'Packed' 16-bit color value (565 format).
    */
    uint16_t Color565(uint8_t red, uint8_t green, uint8_t blue) const
    {
        return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
    }

    uint32_t screen_update_last_, screen_update_period_, fps_update_last_;

    ILI9341SpiTransport transport_;
    uint8_t *localbuffer;

    uint8_t  rotation;
    uint32_t diff;
    uint16_t frames = 0;

    uint16_t currentX_;
    uint16_t currentY_;
    // 2 * width * 32; // 2 bits per pixel, 32 rows
    // static uint16_t const num_sectors     = 10;
    // static uint16_t const sector_size     = buffer_size / num_sectors;
    // static uint16_t       dirty_buff[num_sectors]; // = {0};
    // = {0}; // DMA max (?) 65536 // full screen - 153600

    uint16_t fps = 0;

    Dma2DHandle dma2d_;
};
