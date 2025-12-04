#include "include/pci.h"

uint32_t PCI_read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset) {
    // construct the identifier to send to the PCI command port
    uint32_t id = 
    0x1 << 31
    | ((bus & 0xFF) << 16)
    | ((device & 0x1F) << 11)
    | ((function & 0x07) << 8)
    | (registeroffset & 0xFC);
    i686_outl(PCI_CONFIG_ADDRESS, id);  // send the address on its way
    uint32_t result = i686_inl(PCI_CONFIG_DATA);
    return result >> (8 * (registeroffset % 4));
}

void PCI_write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value) {
    // construct the identifier to send to the PCI command port
    uint32_t id = 
    0x1 << 31
    | ((bus & 0xFF) << 16)
    | ((device & 0x1F) << 11)
    | ((function & 0x07) << 8)
    | (registeroffset & 0xFC);
    i686_outl(PCI_CONFIG_ADDRESS, id);  // send the address on its way
    i686_outl(PCI_CONFIG_DATA, value);
}

bool do_you_have_functions(uint16_t bus, uint16_t device) {
    return PCI_read(bus, device, 0, 0x0E) & (1 << 7);
}