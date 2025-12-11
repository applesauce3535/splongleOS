#include "include/vga.h"

VideoGraphicsArray g_VGA;

bool VGA_Init() {
    // assign all the ports
    g_VGA.misc_port = 0x3C2;
    g_VGA.crt_index_port = 0x3D4;
    g_VGA.crt_data_port = 0x3D5;
    g_VGA.sequencer_index_port = 0x3C4;
    g_VGA.sequencer_data_port = 0x3C5;
    g_VGA.graphics_index_port = 0x3CE;
    g_VGA.graphics_data_port = 0x3CF;
    g_VGA.attribute_index_port = 0x3C0;
    g_VGA.attribute_read_port = 0x3C1;
    g_VGA.attribute_write_port = 0x3C0;
    g_VGA.attribute_reset_port = 0x3DA;

    if (SetMode(320, 200, 8)) return true;
    else return false;
}

bool SetMode(uint32_t width, uint32_t height, uint32_t colordepth) {
    // make sure mode is actually supported
    if (!SupportsMode(width, height, colordepth)) return false;

    // these are just all the values this mode needs, gotten from online
    unsigned char g_320x200x256[] =
    {
        /* MISC */
            0x63,
        /* SEQ */
            0x03, 0x01, 0x0F, 0x00, 0x0E,
        /* CRTC */
            0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
            0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
            0xFF,
        /* GC */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
            0xFF,
        /* AC */
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
            0x41, 0x00, 0x0F, 0x00, 0x00
    };

    // okay, now we need to write all of those values to their respective ports
    VGAWriteRegisters(g_320x200x256);
    return true;
}

bool SupportsMode(uint32_t width, uint32_t height, uint32_t colordepth) {
    // for now only supporting 320x200, 8-bit color depth
    return width == 320 && height == 200 && colordepth == 8;
}

void VGAWriteRegisters(uint8_t* registers) {
    // be prepared for some pointer arithmetic
    // first byte of the array is for the misc port
    i686_outb(g_VGA.misc_port, *(registers++));

    // next 5 bytes are for sequencer
    for (uint8_t i = 0; i < 5; i++) {
        // first write the index to the sequencer index port
        i686_outb(g_VGA.sequencer_index_port, i);
        // now write value to the data port
        i686_outb(g_VGA.sequencer_data_port, *(registers++));
    }

    // we need to lock the CRT controller first
    i686_outb(g_VGA.crt_index_port, 0x03);
    i686_outb(g_VGA.crt_data_port, (i686_inb(g_VGA.crt_data_port) | 0x80));     // set bit 7 to 1
    i686_outb(g_VGA.crt_index_port, 0x11);
    i686_outb(g_VGA.crt_data_port, (i686_inb(g_VGA.crt_data_port) & ~0x80));    // set bit 7 to 0

    // now we wanna make sure we also don't overwrite the lock
    registers[0x03] = registers[0x03] | 0x80;
    registers[0x11] = registers[0x11] & ~0x80;

    // next 25 bytes are for CRT
    for (uint8_t i = 0; i < 25; i++) {
        i686_outb(g_VGA.crt_index_port, i);
        i686_outb(g_VGA.crt_data_port, *(registers++));
    }

    // next 9 bytes are for graphics
    for (uint8_t i = 0; i < 9; i++) {
        i686_outb(g_VGA.graphics_index_port, i);
        i686_outb(g_VGA.graphics_data_port, *(registers++));
    }

    // next 21 bytes are for attribute
    for (uint8_t i = 0; i < 21; i++) {
        // we need to reset the attribute controller first
        i686_inb(g_VGA.attribute_reset_port);
        i686_outb(g_VGA.attribute_index_port, i);
        i686_outb(g_VGA.attribute_write_port, *(registers++));
    }

    i686_inb(g_VGA.attribute_reset_port);
    i686_outb(g_VGA.attribute_index_port, 0x20);

}

uint8_t* GetFrameBufferSegment() {
    i686_outb(g_VGA.graphics_index_port, 0x06);
    // we are only interested in bits 2 and 3
    uint8_t segment_num = ((i686_inb(g_VGA.graphics_data_port) >> 2) & 0x03);

    switch(segment_num) {
        case 0:
            return (uint8_t*)0x00000;
        case 1:
            return (uint8_t*)0xA0000;
        case 2:
            return (uint8_t*)0xB0000;
        case 3:
            return (uint8_t*)0x0B800;
    }
}

void PutPixel(uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue) {
    PutPixelWithIndex(x, y, GetColorIndex(red, green, blue));
}

void PutPixelWithIndex(uint32_t x, uint32_t y, uint8_t color_index) {
    // need to figure out where to write this pixel data
    uint8_t* pixel_address = GetFrameBufferSegment() + 320 * y + x;
    *pixel_address = color_index;
}

uint8_t GetColorIndex(uint8_t red, uint8_t green, uint8_t blue) {
    if (red == 0 && green == 0 && blue == 0xA8) return 0x01;
}