#pragma once
#include <stdint.h>

#define UNUSED_PORT 0x80
#define ASMCALL __attribute__((cdecl))

void ASMCALL i686_outb(uint16_t port, uint8_t value);
uint8_t ASMCALL i686_inb(uint16_t port);

void ASMCALL i686_outl(uint16_t port, uint32_t value);
uint32_t ASMCALL i686_inl(uint16_t port);

uint8_t ASMCALL i686_EnableInts();
uint8_t ASMCALL i686_DisableInts();

// just writes something to an unused port
void ASMCALL i686_io_wait();

// lock system
void ASMCALL i686_panic();

void ASMCALL i686_InvalidatePage(uint32_t page);
void ASMCALL i686_ChangePD(uint32_t pd);
void ASMCALL i686_EnablePaging();

uint32_t ASMCALL i686_ReadCR2();