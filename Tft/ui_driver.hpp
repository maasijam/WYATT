#pragma once
#include "daisy_seed.h"
//#include "../../daisy_wyatt.h"
//#include "daisy_pod.h"
#include "util/oled_fonts.h"
#include "hid/disp/graphics_common.h"
//#include "custom_fonts.h"
using daisy::Rectangle;



enum TFT_COLOR
{
    COLOR_BLACK = 0,
    COLOR_WHITE,
    COLOR_BLUE,
    COLOR_DARK_BLUE,
    COLOR_CYAN,
    COLOR_YELLOW,
    COLOR_DARK_YELLOW,
    COLOR_ORANGE,
    COLOR_RED,
    COLOR_DARK_RED,
    COLOR_GREEN,
    COLOR_DARK_GREEN,
    COLOR_LIGHT_GREEN,
    COLOR_GRAY,
    COLOR_DARK_GRAY,
    COLOR_LIGHT_GRAY,
    COLOR_MEDIUM_GRAY,
    COLOR_ABL_BG,
    COLOR_ABL_LINE,
    COLOR_ABL_D_LINE,
    COLOR_ABL_L_GRAY,
    COLOR_ABL_M_GRAY,
    NUMBER_OF_TFT_COLORS
};

class _UiDriver
{
  protected:
    //Screen dimension constants
     uint16_t width  = 320;
     uint16_t height = 240;
     uint8_t  header = 0;
     uint8_t  footer = 0;

  public:
    virtual ~_UiDriver() = default;
    virtual void DrawLine(uint16_t x1,
                          uint16_t y1,
                          uint16_t x2,
                          uint16_t y2,
                          uint8_t  color,
                          uint8_t  alpha = 255)
        = 0;

    virtual void DrawRect(uint16_t x,
                          uint16_t y,
                          uint16_t w,
                          uint16_t h,
                          uint8_t  color,
                          uint8_t  alpha = 255)
        = 0;

    virtual void
    FillRect(const Rectangle& rect, uint8_t color, uint8_t alpha = 255)
        = 0;

    virtual void DrawTriangle(int16_t x0,
                              int16_t y0,
                              int16_t x1,
                              int16_t y1,
                              int16_t x2,
                              int16_t y2,
                              uint8_t color,
                              uint8_t alpha = 255)
        = 0;

    virtual void FillTriangle(int16_t x0,
                              int16_t y0,
                              int16_t x1,
                              int16_t y1,
                              int16_t x2,
                              int16_t y2,
                              uint8_t color,
                              uint8_t alpha = 255)
        = 0;

    virtual void WriteString(const char* str,
                             uint16_t    x,
                             uint16_t    y,
                             FontDef      font,
                             uint8_t     color)
        = 0;

    virtual bool IsRender() = 0;

    virtual void Init(uint8_t *buffer) = 0;

    static void TrimString(char*    str,
                           char*    str_trimmed,
                           uint16_t str_len,
                           uint16_t str_width,
                           FontDef   font)
    {
        if(str_len <= str_width)
        {
            return;
        }

        uint16_t max_chars = str_width / font.FontWidth;
        strncpy(str_trimmed, str, max_chars);
        str_trimmed[max_chars] = '\0';
    }

    virtual uint16_t GetStringWidth(const char* str, FontDef font) const = 0;


    [[nodiscard]] Rectangle GetBounds() const
    {
        return {static_cast<int16_t>(width), static_cast<int16_t>(height)};
    }

    [[nodiscard]] Rectangle GetDrawableFrame() const
    {
        return Rectangle(static_cast<int16_t>(width),
                         static_cast<int16_t>(height))
            .WithTrimmedTop(header)
            .WithTrimmedBottom(footer);
    }

    void Fill(uint8_t color)
    {
        Rectangle rect
            = {0, 0, static_cast<int16_t>(width), static_cast<int16_t>(height)};
        FillRect(rect, color);
    }

    Rectangle WriteStringAligned(const char*      str,
                                 const FontDef&    font,
                                 const Rectangle& boundingBox,
                                 daisy::Alignment alignment,
                                 uint8_t          color)
    {
        auto alignedRect
            = GetTextRect(str, font).AlignedWithin(boundingBox, alignment);
        if(alignedRect.GetX() < 1)
        {
            alignedRect = alignedRect.WithLeft(0);
        }
        WriteString(str, alignedRect.GetX(), alignedRect.GetY(), font, color);
        return alignedRect;
    }

    void WriteString(const char* str, uint16_t x, uint16_t y, FontDef font)
    {
        WriteString(str, x, y, font, COLOR_WHITE);
    }

    void DrawRect(const Rectangle& rect, uint8_t color, uint8_t alpha = 255)
    {
        this->DrawRect(rect.GetX(),
                       rect.GetY(),
                       rect.GetWidth(),
                       rect.GetHeight(),
                       color,
                       alpha);
    }

    virtual void Update() {}

    virtual uint32_t Time() { return 0; }

    //   private:
    Rectangle GetTextRect(const char* text, const FontDef& font) const
    {
        return {static_cast<int16_t>(GetStringWidth(text, font)),
                font.FontHeight};
    }

    [[nodiscard]] virtual uint16_t Fps() const { return 99; }
};
