#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver.h"
#include "asm_wrappers.h"

typedef struct {
    uint16_t misc_port;
    uint16_t crt_index_port;
    uint16_t crt_data_port;
    uint16_t sequencer_index_port;
    uint16_t sequencer_data_port;
    uint16_t graphics_index_port;
    uint16_t graphics_data_port;
    uint16_t attribute_index_port;
    uint16_t attribute_read_port;
    uint16_t attribute_write_port;
    uint16_t attribute_reset_port;
} VideoGraphicsArray;

bool VGA_Init();

bool SetMode(uint32_t width, uint32_t height, uint32_t colordepth);
// tells us if VGA supports the mode we want to set
bool SupportsMode(uint32_t width, uint32_t height, uint32_t colordepth);
void VGAWriteRegisters(uint8_t* registers);
// get the segment offset from the framebuffer we want to use
uint8_t* GetFrameBufferSegment();
// puts a pixel on screen at (x,y) with RGB 24-bit value
void PutPixel(uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue);
void PutPixelWithIndex(uint32_t x, uint32_t y, uint8_t color_index);
uint8_t GetColorIndex(uint8_t red, uint8_t green, uint8_t blue);
