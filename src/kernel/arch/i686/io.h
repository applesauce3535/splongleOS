#pragma once
#include <stdint.h>

void __attribute__((cdecl)) i686_outb(uint16_t port, uint8_t value);
uint8_t __attribute__((cdecl)) i686_inb(uint16_t port);

uint8_t __attribute__((cdecl)) i686_EnableInts();
uint8_t __attribute__((cdecl)) i686_DisableInts();

// just writes something to an unused port
void i686_io_wait();

void __attribute__((cdecl)) i686_panic();