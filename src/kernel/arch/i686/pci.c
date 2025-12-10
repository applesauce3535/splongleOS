#include "include/pci.h"

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

// after reading base address register, the seventh bit will tell us if the device has multiple functions or not
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

                // some devices can have up to 6 BARs, so loop 6 times
                for(int bar_num = 0; bar_num < 6; bar_num++) {
                    BaseAddressRegister bar = GetBAR(bus, device, function, bar_num);
                    if (bar.address && (bar.type == INPUT_OUTPUT_TYPE)) dev.port_base = (uint32_t)bar.address;

                    Driver* driver = GetDriver(dev);

                    if (driver != 0) manager->AddDriver(driver);
                }

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

BaseAddressRegister GetBAR(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar) {
    BaseAddressRegister result;
    /*
    get the header type at config space 0x0E. actual header type is in bits 6-0, so mask off bit 7.
    bit 7 is just if it's multifunctional or not.
    header type 0 = 0x00 - standard PCI device
    header type 1 = 0x01 - PCI to PCI bridge
    header type 2 = 0x02 - cardbus controller
    */
    uint32_t headertype = PCI_read(bus, device, function, 0x0E) & 0x7F;
    /*
    type 0 can have a max of 6 BARs, BARs 0-5
    type 1 can only have 2 BARs
    type 2 is rare to see anymore, and has no BARs
    */
    int8_t max_bars = 6 - (4*headertype);
    if (max_bars < 0) max_bars = 0; // type 2 case, although probably will not encounter it

    if (bar >= max_bars) return result; // if requested BAR exceeds the max for the device, return null BAR

    // BARs start at 0x10 and each one has a size of 4 bytes, so offset the offset by 4 * chosen BAR
    uint32_t bar_value = PCI_read(bus, device, function, 0x10 + 4 * bar);
    // tell whether this BAR is I/O or mmap type
    result.type = (bar_value & 0x1) ? INPUT_OUTPUT_TYPE : MEMORY_MAPPING_TYPE;
    uint32_t temp;

    if (result.type == MEMORY_MAPPING_TYPE) {
        /*
        remove bit 0 that tells us the type, then check bits 1-0
        00: 32 bit BAR
        01: 20 bit BAR
        10: 64 bit BAR
        */
        switch((bar_value >> 1) & 0x3) {
            case 0x00:
            case 0x01:
            case 0x02:
        }
        // prefetchable bit for mmap BAR is bit 3
        result.prefetchable = ((bar_value >> 3) & 0x1) == true;
    }
    else {  // I/O
        // bits 31-2 tell us the port number, bits 1-0 do not, so mask them off
        result.address = (uint8_t*)(bar_value & ~0x3);
        // I/O is never prefetchable
        result.prefetchable = false;
        
    }

    return result;
}

Driver* GetDriver(PCIDeviceDescriptor dev) {
    /*
    TODO:
    later on, store a file on the disk that has info on these PCI device drivers.
    for now, I will have to hard code them
    */
    switch(dev.vendor_id) {
        case 0x1022:    // AMD
            switch(dev.device_id) {
                case 0x2000:    // am79c973
                    // printk("AMD Ethernet controller\n");
                    break;
            }
            break;
        
        case 0x8086:    // Intel
            switch(dev.device_id) {
                case 0x100E:
                    // printk("Intel Gigabit Ethernet Controller\n");
                    break;
                
                case 0x2922:
                    // printk("Intel 6 Port SATA Controller\n");
                    break;
            }
            break;
    }

    // generic devices
    switch(dev.class_id) {
        case 0x03:  // graphics device
            switch(dev.subclass_id) {
                case 0x00:  // VGA devices
                    // printk("VGA Device\n");
                    break;
            }
            break;
    }
    return 0;
}