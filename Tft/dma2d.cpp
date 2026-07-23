#include "dma2d.hpp"
#include "stm32h7xx_hal.h"

#define DMA2D_POSITION_NLR_PL \
    (uint32_t) POSITION_VAL(  \
        DMA2D_NLR_PL) /*!< Required left shift to set pixels per lines value */

#define DMA2D_POSITION_FGPFCCR_AI (uint32_t) POSITION_VAL(DMA2D_FGPFCCR_AI)

#define IS_DMA2D_READY() (hdma2d.Instance->CR & DMA2D_CR_START) == 0
#define START_DMA2D() hdma2d.Instance->CR |= DMA2D_CR_START

static const uint32_t         color_mem_size = 320 * 240;
static uint32_t DSY_SDRAM_BSS color_mem[color_mem_size];

static void CpltCallback(DMA2D_HandleTypeDef* hdma2d)
{
    if(hdma2d->ErrorCode != HAL_DMA2D_ERROR_NONE)
    {
        __asm__("BKPT");
    }
}
static void ErrorCallback(DMA2D_HandleTypeDef* hdma2d)
{
    __asm__("BKPT");
}

class Dma2DHandle::Impl
{
  public:
    void Init(uint8_t* buffer_)
    {
        memset(color_mem, 0, color_mem_size);

        buffer = buffer_;

        // hdma2d.Instance           = DMA2D;
        // hdma2d.Init.Mode          = DMA2D_R2M;
        // hdma2d.Init.ColorMode     = DMA2D_OUTPUT_RGB565; // DMA2D_OUTPUT_RGB888
        // hdma2d.Init.OutputOffset  = 0;
        // hdma2d.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
        // hdma2d.State = HAL_DMA2D_STATE_RESET;
        InitDma2D();
    }
    void InitDma2D()
    {
        hdma2d.Instance       = DMA2D;
        hdma2d.Init.Mode      = DMA2D_M2M_BLEND;
        hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
        // hdma2d.Init.RedBlueSwap  = DMA2D_RB_REGULAR;
        // hdma2d.Init.BytesSwap    = DMA2D_BYTES_REGULAR;
        hdma2d.Init.OutputOffset = 0;

        // hdma2d.LayerCfg[1].InputOffset = 0;
        // hdma2d.LayerCfg[1].InputColorMode    = DMA2D_INPUT_A8;
        // hdma2d.LayerCfg[1].AlphaMode         = DMA2D_NO_MODIF_ALPHA;
        // hdma2d.LayerCfg[1].InputAlpha        = 0xffffff;
        // hdma2d.LayerCfg[1].AlphaInverted     = DMA2D_REGULAR_ALPHA;
        // hdma2d.LayerCfg[1].RedBlueSwap       = DMA2D_RB_REGULAR;
        // hdma2d.LayerCfg[1].ChromaSubSampling = DMA2D_NO_CSS;
        // hdma2d.LayerCfg[0].InputOffset       = 0;
        // hdma2d.LayerCfg[0].InputColorMode    = DMA2D_INPUT_RGB565;
        // hdma2d.LayerCfg[0].AlphaMode         = DMA2D_NO_MODIF_ALPHA;
        // hdma2d.LayerCfg[0].InputAlpha        = 0;
        // hdma2d.LayerCfg[0].AlphaInverted     = DMA2D_REGULAR_ALPHA;
        // hdma2d.LayerCfg[0].RedBlueSwap       = DMA2D_RB_REGULAR;
        _init2d();
    }
    void _init2d()
    {
        hdma2d.LayerCfg[0].ChromaSubSampling = DMA2D_NO_CSS;
        hdma2d.LayerCfg[1].ChromaSubSampling = DMA2D_NO_CSS;
        hdma2d.XferCpltCallback              = CpltCallback;
        hdma2d.XferErrorCallback             = ErrorCallback;
        if(HAL_DMA2D_Init(&hdma2d) != HAL_OK)
        {
            __asm__("BKPT");
        }
        if(HAL_DMA2D_ConfigLayer(&hdma2d, 0) != HAL_OK)
        {
            __asm__("BKPT");
        }
        if(HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
        {
            __asm__("BKPT");
        }
    }

    void FillRect(const Rectangle& rect, uint16_t color)
    {
        while(!IS_DMA2D_READY()) {}
        HAL_DMA2D_PollForTransfer(&hdma2d, 100);
        auto offset = (rect.GetX() + rect.GetY() * screen_width)
                      * 2; // 2 bytes per pixel

        hdma2d.Init.Mode      = DMA2D_R2M;
        hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
        // hdma2d.Init.AlphaInverted = DMA2D_INVERTED_ALPHA;
        hdma2d.Init.OutputOffset = screen_width - rect.GetWidth();

        _init2d();
        auto result = HAL_DMA2D_Start(&hdma2d,
                                      //   color,
                                      __builtin_bswap16(color),
                                      (uint32_t)(buffer + offset),
                                      rect.GetWidth(),
                                      rect.GetHeight());

        if(result != HAL_OK)
        {
            __asm__("BKPT");
        }

        HAL_DMA2D_PollForTransfer(&hdma2d, 100);
    }

    void DMA2D_DrawImage(const Rectangle& rect, uint16_t color, uint8_t alpha)
    {
        HAL_DMA2D_PollForTransfer(&hdma2d, 100);

        auto offset = (rect.GetX() + rect.GetY() * screen_width) * 2;

        hdma2d.Instance           = DMA2D;
        hdma2d.Init.Mode          = DMA2D_M2M_BLEND;
        hdma2d.Init.ColorMode     = DMA2D_OUTPUT_RGB565;
        hdma2d.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
        hdma2d.Init.RedBlueSwap   = DMA2D_RB_REGULAR;
        hdma2d.Init.OutputOffset  = screen_width - rect.GetWidth();
        // Foreground
        hdma2d.LayerCfg[1].AlphaMode      = DMA2D_REPLACE_ALPHA;
        hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
        hdma2d.LayerCfg[1].InputOffset    = 0;
        hdma2d.LayerCfg[1].InputAlpha     = alpha;
        hdma2d.LayerCfg[1].RedBlueSwap    = DMA2D_RB_REGULAR;
        hdma2d.LayerCfg[1].AlphaInverted  = DMA2D_REGULAR_ALPHA;

        // Background
        hdma2d.LayerCfg[0].InputAlpha     = 255;
        hdma2d.LayerCfg[0].AlphaMode      = DMA2D_REPLACE_ALPHA;
        hdma2d.LayerCfg[0].InputColorMode = DMA2D_INPUT_RGB565;
        hdma2d.LayerCfg[0].InputOffset    = screen_width - rect.GetWidth();
        hdma2d.LayerCfg[0].RedBlueSwap    = DMA2D_RB_REGULAR;
        hdma2d.LayerCfg[0].AlphaInverted  = DMA2D_REGULAR_ALPHA;
        HAL_DMA2D_DeInit(&hdma2d);
        _init2d();

        HAL_DMA2D_ConfigLayer(&hdma2d, 1);
        HAL_DMA2D_ConfigLayer(&hdma2d, 0);

        HAL_DMA2D_BlendingStart(&hdma2d,
                                *color_mem,
                                // clr,
                                (uint32_t)(buffer + offset),
                                (uint32_t)(buffer + offset),
                                rect.GetWidth(),
                                rect.GetHeight());
        HAL_DMA2D_PollForTransfer(&hdma2d, 100);
    }

    // It works, but the colors are messed up.
    void FillRectReg(const Rectangle& rect, uint16_t color)
    {
        while(!IS_DMA2D_READY()) {}

        InitDma2D();

        while(!IS_DMA2D_READY()) {}

        MODIFY_REG(hdma2d.Instance->CR, DMA2D_CR_MODE, DMA2D_R2M);
        WRITE_REG(hdma2d.Instance->OCOLR, __builtin_bswap16(color));

        auto offset = (rect.GetX() + rect.GetY() * screen_width) * 2;

        MODIFY_REG(hdma2d.Instance->OOR, DMA2D_OOR_LO, 320 - rect.GetWidth());
        MODIFY_REG(
            hdma2d.Instance->NLR,
            (DMA2D_NLR_NL | DMA2D_NLR_PL),
            (rect.GetHeight() | (rect.GetWidth() << DMA2D_POSITION_NLR_PL)));
        WRITE_REG(hdma2d.Instance->OMAR, (uint32_t)(buffer + offset));
        START_DMA2D();
        HAL_DMA2D_PollForTransfer(&hdma2d, 100);
    }

    void FillRect(const Rectangle& rect, uint16_t color, uint8_t alpha)
    {
        HAL_DMA2D_PollForTransfer(&hdma2d, 100);

        if(alpha == 255)
        {
            // return FillRect(rect, color);
            return FillRectReg(rect, color);
        }
        // return FillRectReg(rect, color);


        return FillTransparentRect(rect, color, alpha);
    }

    void
    FillTransparentRect(const Rectangle& rect, uint16_t color, uint8_t alpha)
    {
        // ============================ 1 ============================
        // Fill the buffer with color

        while(!IS_DMA2D_READY()) {}

        InitDma2D();

        while(!IS_DMA2D_READY()) {}

        // MODIFY_REG(hdma2d.Instance->CR, DMA2D_CR_MODE, DMA2D_R2M);
        // WRITE_REG(hdma2d.Instance->OCOLR, __builtin_bswap16(color));

        // MODIFY_REG(hdma2d.Instance->OOR, DMA2D_OOR_LO, 0);
        // MODIFY_REG(
        //     hdma2d.Instance->NLR,
        //     (DMA2D_NLR_NL | DMA2D_NLR_PL),
        //     (rect.GetHeight() | (rect.GetWidth() << DMA2D_POSITION_NLR_PL)));
        // WRITE_REG(hdma2d.Instance->OMAR, (uint32_t)(color_mem));
        // START_DMA2D();
        // HAL_DMA2D_PollForTransfer(&hdma2d, 100);

        hdma2d.Init.Mode      = DMA2D_R2M;
        hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
        // hdma2d.Init.AlphaInverted = DMA2D_INVERTED_ALPHA;
        hdma2d.Init.OutputOffset = screen_width - rect.GetWidth();

        // hdma2d.Init.BytesSwap = DMA2D_BYTES_SWAP;
        // hdma2d.Init.RedBlueSwap = DMA2D_RB_SWAP;

        _init2d();
        auto result2 = HAL_DMA2D_Start(&hdma2d,
                                       color,
                                       (uint32_t)(color_mem),
                                       rect.GetWidth(),
                                       rect.GetHeight());

        if(result2 != HAL_OK)
        {
            __asm__("BKPT");
        }
        HAL_DMA2D_PollForTransfer(&hdma2d, 100);


        // ============================ 2 ============================
        // Blend two buffers
        // TODO: Try with additional intermidiate buffer: RM > M2M_Blend > M2M

        // R2M with blending is directly not supported. However, you can do these steps:
        // 1. Use R2M mode and fill your rectangle memory outside layers
        // 2. Perform M2M with blending
        //      1. For front layer set memory of your rectangle you just filled
        //      2. For back layer set memory position where you want to put your rectangle
        //      3. Set front layer alpha for transparency value of whole layer

        auto offset = (rect.GetX() + rect.GetY() * screen_width) * 2;

        hdma2d.Instance           = DMA2D;
        hdma2d.Init.Mode          = DMA2D_M2M_BLEND;
        hdma2d.Init.ColorMode     = DMA2D_OUTPUT_RGB565;
        hdma2d.Init.AlphaInverted = DMA2D_INVERTED_ALPHA;
        hdma2d.Init.RedBlueSwap   = DMA2D_RB_REGULAR;
        hdma2d.Init.OutputOffset  = screen_width - rect.GetWidth();
        // Foreground
        hdma2d.LayerCfg[1].AlphaMode      = DMA2D_REPLACE_ALPHA;
        hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
        hdma2d.LayerCfg[1].InputOffset    = 0;
        hdma2d.LayerCfg[1].InputAlpha     = alpha;
        hdma2d.LayerCfg[1].RedBlueSwap    = DMA2D_RB_REGULAR;
        hdma2d.LayerCfg[1].AlphaInverted  = DMA2D_REGULAR_ALPHA;

        // Background
        hdma2d.LayerCfg[0].AlphaMode      = DMA2D_NO_MODIF_ALPHA;
        hdma2d.LayerCfg[0].InputColorMode = DMA2D_INPUT_RGB565;
        hdma2d.LayerCfg[0].InputOffset    = screen_width - rect.GetWidth();
        hdma2d.LayerCfg[0].RedBlueSwap    = DMA2D_RB_REGULAR;
        hdma2d.LayerCfg[0].AlphaInverted  = DMA2D_REGULAR_ALPHA;
        _init2d();

        auto result = HAL_DMA2D_BlendingStart(&hdma2d,
                                              *color_mem,
                                              (uint32_t)(buffer + offset),
                                              (uint32_t)(buffer + offset),
                                              rect.GetWidth(),
                                              rect.GetHeight());

        if(result != HAL_OK)
        {
            __asm__("BKPT");
        }

        HAL_DMA2D_PollForTransfer(&hdma2d, 100);
    }

    void WriteChar(uint16_t x,
                   uint16_t y,
                   char     ch,
                   FontDef   font,
                   uint16_t color) const
    {
        while(!IS_DMA2D_READY()) {}

        MODIFY_REG(hdma2d.Instance->FGPFCCR,
                   (DMA2D_FGPFCCR_CM | DMA2D_FGPFCCR_AI),
                   (DMA2D_INPUT_A4
                    | (1 << DMA2D_POSITION_FGPFCCR_AI))); // A4 + alpha inverted

        WRITE_REG(hdma2d.Instance->FGCOLR,
                  (((((color & 0xF800) >> 11) * 255 / 31) << 16)
                   | ((((color & 0x07e0) >> 5) * 255 / 63) << 8)
                   | (((color & 0x001f)) * 255 / 31)));

        WRITE_REG(hdma2d.Instance->BGCOLR, 0x0000);
        MODIFY_REG(hdma2d.Instance->CR, DMA2D_CR_MODE, DMA2D_M2M_BLEND);


        uint32_t offset = y * 240 + x;
        // auto offset = (x + y * screen_width) * 2;

        auto mod = screen_width;
        offset -= mod * font.FontHeight;

        MODIFY_REG(hdma2d.Instance->OOR, DMA2D_OOR_LO, mod - font.FontWidth);
        MODIFY_REG(hdma2d.Instance->BGOR, DMA2D_OOR_LO, mod - font.FontWidth);
        MODIFY_REG(
            hdma2d.Instance->NLR,
            (DMA2D_NLR_NL | DMA2D_NLR_PL),
            (font.FontHeight | (font.FontWidth << DMA2D_POSITION_NLR_PL)));
        WRITE_REG(hdma2d.Instance->FGMAR,
                  (uint32_t)(font.data[(ch - 32) * font.FontHeight]));

        // WRITE_REG(hdma2d.Instance->BGMAR, (uint32_t)(buffer + offset));
        WRITE_REG(hdma2d.Instance->OMAR, (uint32_t)(buffer + offset));
        START_DMA2D();

        // HAL_DMA2D_PollForTransfer(&hdma2d, 100);
    }

    uint32_t RGB565toARGB8888(uint16_t rgb565Color, uint8_t alpha)
    {
        // Extract the RGB components from RGB565
        uint8_t r5 = (rgb565Color >> 11) & 0x1F;
        uint8_t g6 = (rgb565Color >> 5) & 0x3F;
        uint8_t b5 = rgb565Color & 0x1F;

        // Scale and expand the RGB components to 8 bits
        uint8_t r8 = (r5 << 3) | (r5 >> 2); // 5-bit to 8-bit
        uint8_t g8 = (g6 << 2) | (g6 >> 4); // 6-bit to 8-bit
        uint8_t b8 = (b5 << 3) | (b5 >> 2); // 5-bit to 8-bit

        // Compose the ARGB8888 color with the specified alpha value
        uint32_t argb8888Color = (static_cast<uint32_t>(alpha) << 24)
                                 | (r8 << 16) | (g8 << 8) | b8;

        return argb8888Color;
    }

    uint32_t RGB888toARGB8888(uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
    {
        // Compose the ARGB8888 color with the specified alpha value
        uint32_t argb8888Color = (static_cast<uint32_t>(alpha) << 24)
                                 | (static_cast<uint32_t>(r) << 16)
                                 | (static_cast<uint32_t>(g) << 8) | b;

        return argb8888Color;
    }


    uint32_t RGB565toRGB888(uint16_t rgb565Color)
    {
        // Extract the RGB components from RGB565
        uint8_t r5 = (rgb565Color >> 11) & 0x1F;
        uint8_t g6 = (rgb565Color >> 5) & 0x3F;
        uint8_t b5 = rgb565Color & 0x1F;

        // Scale and expand the RGB components to 8 bits
        uint8_t r8 = (r5 << 3) | (r5 >> 2); // 5-bit to 8-bit
        uint8_t g8 = (g6 << 2) | (g6 >> 4); // 6-bit to 8-bit
        uint8_t b8 = (b5 << 3) | (b5 >> 2); // 5-bit to 8-bit

        // Compose and return the RGB888 color as a 24-bit value
        uint32_t rgb888Color = (static_cast<uint32_t>(r8) << 16)
                               | (static_cast<uint32_t>(g8) << 8)
                               | static_cast<uint32_t>(b8);

        return rgb888Color;
    }

  private:
    uint16_t            screen_width  = 320;
    uint16_t            screen_height = 240;
    DMA2D_HandleTypeDef hdma2d{};
    uint8_t*            buffer{};
};

static Dma2DHandle::Impl hdma2d_handle;

void Dma2DHandle::Init(uint8_t* buffer_)
{
    InitPalette();
    impl = &hdma2d_handle;
    impl->Init(buffer_);
}

void Dma2DHandle::FillRect(const Rectangle& rect,
                           uint8_t          color_id,
                           uint8_t          alpha)
{
    auto color = tftPalette[color_id];
    impl->FillRect(rect, color, alpha);
}

void Dma2DHandle::WriteChar(uint16_t x,
                            uint16_t y,
                            char     ch,
                            FontDef   font,
                            uint8_t  color_id)
{
    auto color = tftPalette[color_id];

    impl->WriteChar(x, y, ch, font, color);
}

void HAL_DMA2D_MspInit(DMA2D_HandleTypeDef* hdma2d)
{
    if(hdma2d->Instance == DMA2D)
    {
        /* Peripheral clock enable */
        __HAL_RCC_DMA2D_CLK_ENABLE();
    }
}

void HAL_SPI_MspDeInit(DMA2D_HandleTypeDef* hdma2d)
{
    if(hdma2d->Instance == DMA2D)
    {
        /* Peripheral clock disable */
        __HAL_RCC_DMA2D_CLK_DISABLE();
    }
}
