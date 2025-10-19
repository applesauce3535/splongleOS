#pragma once
#include <stdint.h>

#define ASMCALL __attribute__((cdecl))

void ASMCALL i686_outb(uint16_t port, uint8_t value);
uint8_t ASMCALL i686_inb(uint16_t port);

uint8_t ASMCALL i686_EnableInts();
uint8_t ASMCALL i686_DisableInts();

// just writes something to an unused port
void i686_io_wait();

void ASMCALL i686_InvalidatePage(uint32_t page);

void ASMCALL i686_panic();