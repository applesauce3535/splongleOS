#include "include/asm_wrappers.h"

// writes nothing to an unused port. completely safe way to wait 1 machine cycle
void i686_io_wait() {
    i686_outb(UNUSED_PORT, 0);
}