#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "asm_wrappers.h"
#include "driver.h"
#include "stdio.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

typedef enum {
    MEMORY_MAPPING_TYPE = 0,
    INPUT_OUTPUT_TYPE   = 1
} BaseAddressRegisterType;

typedef struct {
    bool prefetchable;  // only in mm type, bit 3, 1: data can be read multiple times without changing value, 0: reads have side effects
    uint8_t* address;
    uint32_t size;
    BaseAddressRegisterType type;
} BaseAddressRegister;

typedef struct {
    uint32_t port_base;     // communication port
    uint32_t interrupt;     // interrupt number of device
    uint16_t bus;           // bus of the device
    uint16_t device;        // actual device
    uint16_t function;      // number of the function
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_id;
    uint8_t subclass_id;
    uint8_t interface_id;
    uint8_t revision;
} PCIDeviceDescriptor;

uint32_t PCI_read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset);
void PCI_write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value);
bool do_you_have_functions(uint16_t bus, uint16_t device);
void SelectDrivers(DriverManager* manager);
BaseAddressRegister GetBAR(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar);
Driver* GetDriver(PCIDeviceDescriptor dev);