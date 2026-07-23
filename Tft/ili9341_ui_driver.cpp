#include "ili9341_ui_driver.hpp"

 uint8_t *ILI9341SpiTransport::frame_buffer; // DMA max (?) 65536 // full screen - 153600

uint8_t ILI9341SpiTransport::color_mem[ILI9341SpiTransport::buffer_size / 2]
    = {};