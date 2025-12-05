#include "include/pci.h"

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

PCIDeviceDescriptor GetDevDescriptor(uint16_t bus, uint16_t device, uint16_t function) {
    PCIDeviceDescriptor result;
    result.bus = bus;
    result.device = device;
    result.function = function;
    // now we're gonna get all the other metadata by reading at different offsets
    result.vendor_id = PCI_read(bus, device, function, 0x00);
    result.device_id = PCI_read(bus, device, function, 0x02);
    result.class_id = PCI_read(bus, device, function, 0x0B);
    result.subclass_id = PCI_read(bus, device, function, 0x0A);
    result.interface_id = PCI_read(bus, device, function, 0x09);
    result.revision = PCI_read(bus, device, function, 0x08);
    result.interrupt = PCI_read(bus, device, function, 0x3C);
    return result;
}

uint32_t PCI_read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset) {
    /* 
    construct the identifier to send to the PCI command port
    bit 31 - enable bit
    bits 30-24 - reserved
    bits 23-16 - bus number
    bits 15-11 - device number
    bits 11-8 - function number
    bits 7-0 - register offset
    */
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
    /* 
    construct the identifier to send to the PCI command port
    bit 31 - enable bit
    bits 30-24 - reserved
    bits 23-16 - bus number
    bits 15-11 - device number
    bits 11-8 - function number
    bits 7-0 - register offset
    */
    uint32_t id = 
    0x1 << 31
    | ((bus & 0xFF) << 16)
    | ((device & 0x1F) << 11)
    | ((function & 0x07) << 8)
    | (registeroffset & 0xFC);
    i686_outl(PCI_CONFIG_ADDRESS, id);  // send the address on its way
    i686_outl(PCI_CONFIG_DATA, value);
}

// after reading this address, the seventh bit will tell us if the device has functions or not
bool do_you_have_functions(uint16_t bus, uint16_t device) {
    return PCI_read(bus, device, 0, 0x0E) & (1 << 7);
}

void SelectDrivers(DriverManager* manager) {
    for(int bus = 0; bus < 8; bus++) {
        for(int device = 0; device < 32; device++) {
            int num_functions = do_you_have_functions(bus, device) ? 8 : 1;
            for(int function = 0; function < num_functions; function++) {
                PCIDeviceDescriptor dev = GetDevDescriptor(bus, device, function);
                // if the vendor ID is all 0s or 1s, then there is no device or no function
                if (dev.vendor_id == 0x0000 || dev.vendor_id == 0xFFFF) continue;

                // keep in mind everything gathered from the PCI is little endian. no need
                // to swap here because Intel is little endian
                printk("PCI BUS 0x%x ", bus);
                printk("DEVICE 0x%x ", device);
                printk("FUNCTION 0x%x ", function);
                printk("= VENDOR 0x%x ", dev.vendor_id);
                printk("DEV ID 0x%x\n", dev.device_id);
            }
        }
    }
}