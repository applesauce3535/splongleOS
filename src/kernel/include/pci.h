#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "asm_wrappers.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

uint32_t PCI_read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset);
void PCI_write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value);
bool do_you_have_functions(uint16_t bus, uint16_t device);