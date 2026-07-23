
#pragma once

#include "ui_driver.hpp"
using namespace daisy;

#include "sys/dma.h"

/**
 * SPI Transport for ILI9341 TFT display devices
 */
class ILI9341SpiTransport
{
  public:
    uint32_t update_time = 0;
    uint32_t start_time  = 0;

    void Init(u_int8_t *buffer)
    {
        /*
        Display FPS is bound to two things:
        1. SPI clock speed;
        2. CPU time required to draw to a buffer.
        
        1 - addressed by increasing SPI clock speed in system.cpp: PeriphClkInitStruct.PLL2.PLL2P = 2;
        Note, max is PLL2P = 1, but ILI9341 does not support this speed.
        For 40ish FPS:
        Make sure GPIO pins are also set to very_high,

        2 - addressed by using DMA2D and getting rid of transparent drawing.
        */
        SpiHandle::Config spi_config;
        spi_config.periph          = SpiHandle::Config::Peripheral::SPI_1;
        spi_config.mode            = SpiHandle::Config::Mode::MASTER;
        spi_config.direction       = SpiHandle::Config::Direction::TWO_LINES;
        spi_config.clock_polarity  = SpiHandle::Config::ClockPolarity::LOW;
        spi_config.baud_prescaler  = SpiHandle::Config::BaudPrescaler::PS_2;
        spi_config.clock_phase     = SpiHandle::Config::ClockPhase::ONE_EDGE;
        spi_config.nss             = SpiHandle::Config::NSS::SOFT;
        spi_config.datasize        = 8;
        //spi_config.pin_config.nss  = {DSY_GPIOG, 10}; // D7
        //spi_config.pin_config.sclk = {DSY_GPIOG, 11}; // D8
        //spi_config.pin_config.mosi = {DSY_GPIOB, 5};  // D10

        spi_config.pin_config.nss  = seed::D7; // D7
        spi_config.pin_config.sclk = seed::D8; // D8
        spi_config.pin_config.mosi = seed::D10;  // D10

        //spi_config.pin_config.miso = {DSY_GPIOX, 0};  // not used

        auto dc_pin    = seed::D17;
        auto reset_pin = seed::D23;
        auto cs_pin    = seed::D7;

        pin_dc_.Init(dc_pin,
                     GPIO::Mode::OUTPUT,
                     GPIO::Pull::NOPULL,
                     GPIO::Speed::VERY_HIGH);

        pin_reset_.Init(reset_pin,
                        GPIO::Mode::OUTPUT,
                        GPIO::Pull::NOPULL,
                        GPIO::Speed::VERY_HIGH);

        pin_cs_.Init(cs_pin,
                     GPIO::Mode::OUTPUT,
                     GPIO::Pull::NOPULL,
                     GPIO::Speed::VERY_HIGH);


        spi_.Init(spi_config);

        frame_buffer = buffer;

        InitPalette();
    };

    void Reset()
    {
        pin_reset_.Write(false);
        System::Delay(10);
        pin_reset_.Write(true);
        System::Delay(10);
    }

    // an internal function to handle SPI DMA callbacks
    // called when an DMA transmission completes and the next driver must be updated
    static void TxCompleteCallback(void* context, SpiHandle::Result result)
    {
        auto transport = static_cast<ILI9341SpiTransport*>(context);
        if(result == SpiHandle::Result::OK)
        {
            if(transport->remaining_buff > 0)
            {
                auto transfer_size = transport->GetTransferSize();
                transport->SendDataDMA(
                    &frame_buffer[transport->buffer_size
                                  - transport->remaining_buff],
                    transfer_size);
            }
            else
            {
                transport->dma_busy = false;
                // transport->spi_.SetMode(8);
                transport->update_time
                    = System::GetNow() - transport->start_time;
                // dsy_dma_clear_cache_for_buffer(transport->frame_buffer,
                //                                transport->buffer_size);
            }
        }
        else
        {
            __asm("BKPT #0");
        }
    }

    static void TxStartCallback(void* context)
    {
        // auto transport = static_cast<ILI9341SpiTransport*>(context);
        // transport->spi_.SetMode(16);
    }

    SpiHandle::Result SendDataDMA()
    {
        remaining_buff = buffer_size;
        dma_busy       = true;
        start_time     = System::GetNow();

        // Manual cache invalidation, useful if you don't want to change MPU in system.cpp
        // dsy_dma_clear_cache_for_buffer(frame_buffer, buffer_size);
        return SendDataDMA(frame_buffer, buf_chunk_size);
    };

    SpiHandle::Result SendDataDMA(uint8_t* buff, size_t size)
    {
        pin_dc_.Write(true);
        auto result = spi_.DmaTransmit(
            buff, size, &TxStartCallback, &TxCompleteCallback, this);
        remaining_buff -= size;
        return result;
    };

    uint32_t GetTransferSize() const
    {
        return remaining_buff < buf_chunk_size ? remaining_buff
                                               : buf_chunk_size;
    }

    SpiHandle::Result SendCommand(uint8_t cmd)
    {
        pin_dc_.Write(false);
        return spi_.BlockingTransmit(&cmd, 1);
    };

    SpiHandle::Result SendData(uint8_t* buff, size_t size)
    {
        pin_dc_.Write(true);
        auto result = spi_.BlockingTransmit(buff, size);
        if(result != SpiHandle::Result::OK)
        {
            __asm("BKPT #0");
        }
        return result;
    };

    void SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
    {
        // column address set
        SendCommand(0x2A); // CASET
        {
            uint8_t data[4] = {static_cast<uint8_t>((x0 >> 8) & 0xFF),
                               static_cast<uint8_t>(x0 & 0xFF),
                               static_cast<uint8_t>((x1 >> 8) & 0xFF),
                               static_cast<uint8_t>(x1 & 0xFF)};
            SendData(data, 4);
        }

        // row address set
        SendCommand(0x2B); // RASET
        {
            uint8_t data[4] = {static_cast<uint8_t>((y0 >> 8) & 0xFF),
                               static_cast<uint8_t>(y0 & 0xFF),
                               static_cast<uint8_t>((y1 >> 8) & 0xFF),
                               static_cast<uint8_t>(y1 & 0xFF)};
            SendData(data, 4);
        }

        // write to RAM
        SendCommand(0x2C); // RAMWR
    }

    void PaintPixel(uint32_t id, uint8_t color_id, uint8_t alpha = 255) const
    {
        auto color = tftPalette[color_id];

        // Update the color to match corresponding alpha value
        if(alpha != 255)
        {
            // auto bg_color = tftPalette[color_mem[id]];
            uint16_t bg_color = frame_buffer[id] << 8 | frame_buffer[id + 1];
            color             = Blend565(color, bg_color, alpha);
        }
        // store current color in the buffer
        // color_mem[id] = color_id;

        frame_buffer[id]     = color >> 8;
        frame_buffer[id + 1] = color & 0xFF;
    }

    uint16_t GetPixel(uint32_t id) { return color_mem[id]; }

    bool     dma_busy       = false;
    uint32_t remaining_buff = 0;

    static uint32_t const buffer_size = 153600; // 320 * 240 * 2
    // const uint16_t        buf_chunk_size = buffer_size / 3; // 8bit data
    static constexpr uint16_t buf_chunk_size = UINT16_MAX;
    // const uint16_t buf_chunk_size = buffer_size / 4; // 16bit data
    //static uint8_t DMA_BUFFER_MEM_SECTION frame_buffer[buffer_size];
    static uint8_t DSY_SDRAM_BSS          color_mem[buffer_size / 2];
    SpiHandle                             spi_;

    static uint8_t *frame_buffer;

    uint16_t tftPalette[NUMBER_OF_TFT_COLORS];

  private:
    GPIO pin_dc_;
    GPIO pin_reset_;
    GPIO pin_cs_;


    uint16_t Blend565(uint16_t fg, uint16_t bg, uint8_t alpha) const
    {
        int max_alpha   = 64;
        int mask_mul_rb = 4065216; // 0b1111100000011111
        int mask_mul_g  = 129024;  // 0b0000011111100000
        int mask_rb     = 63519;   // 0b1111100000011111000000
        int mask_g      = 2016;    // 0b0000011111100000000000

        // alpha for foreground multiplication  convert from 8bit to (6bit+1) with rounding will be in [0..64] inclusive
        alpha = (alpha + 2) >> 2;
        // "beta" for background multiplication; (6bit+1); will be in [0..64] inclusive
        uint8_t beta = max_alpha - alpha;
        // so (0..64)*alpha + (0..64)*beta always in 0..64
        return (uint16_t)((((alpha * (uint32_t)(fg & mask_rb)
                             + beta * (uint32_t)(bg & mask_rb))
                            & mask_mul_rb)
                           | ((alpha * (fg & mask_g) + beta * (bg & mask_g))
                              & mask_mul_g))
                          >> 6);
    }

    void InitPalette()
    {
        // HEX to RBG565 converter: https://trolsoft.ru/en/articles/rgb565-color-picker
        tftPalette[COLOR_BLACK]       = 0x0000;
        tftPalette[COLOR_WHITE]       = 0xffff;
        tftPalette[COLOR_BLUE]        = 0x5AFF;
        tftPalette[COLOR_DARK_BLUE]   = 0x18EB;
        tftPalette[COLOR_YELLOW]      = 0xFFE0;
        tftPalette[COLOR_DARK_YELLOW] = 0x49E1;
        tftPalette[COLOR_RED]         = 0xF9E1; // 0xff4010
        tftPalette[COLOR_DARK_RED]    = 0x4880; // 0x401000
        tftPalette[COLOR_GREEN]       = 0x3FE7; // 0x40ff40
        tftPalette[COLOR_DARK_GREEN]  = 0x01E0; // 0x004000
        tftPalette[COLOR_LIGHT_GRAY]  = 0xAD75; // 0xb0b0b0
        tftPalette[COLOR_MEDIUM_GRAY] = 0x8C71; // 0x909090
        tftPalette[COLOR_GRAY]        = 0x5AEB; // 0x606060
        tftPalette[COLOR_DARK_GRAY]   = 0x2965; // 0x303030
        tftPalette[COLOR_CYAN]        = 0x76FD; // 0x76dfef
        tftPalette[COLOR_ORANGE]      = 0xFBE0; // 0xff7f00
        tftPalette[COLOR_LIGHT_GREEN] = 0x6FED; // 0x70ff70
        // tftPalette[COLOR_ABL_BG]      = (0x2104); // 0x212121
        tftPalette[COLOR_ABL_BG]     = 0x4A69; // #4D4D4D
        tftPalette[COLOR_ABL_LINE]   = 0x39E7; // 0x3d3d3d
        tftPalette[COLOR_ABL_D_LINE] = 0x31A6; // 0x363636
        tftPalette[COLOR_ABL_L_GRAY] = 0x52AA; // 0x555555
        tftPalette[COLOR_ABL_M_GRAY] = 0x4228; // 0x454545
    }
};
