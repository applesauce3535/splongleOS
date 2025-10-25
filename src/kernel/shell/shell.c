#include <stdint.h>
#include "stdio.h"
#include "memory/physical_manager.h"
#include "shell.h"
#include "string.h"

void send_command(const char* cmd) {
    if (strcmp(cmd, "help")) {
        printk("\ngetmem - display physical memory information \
                \nversion - display splongleOS version \
                \n");
    }
    else if (strcmp(cmd, "getmem")) {
        printk("\n");
        get_block_info();
        printk("\n");
    }
    else if (strcmp(cmd, "version")) {
        printk("\nsplongleOS V 35\n");
    }
    else {
        printk("\nUnrecognized command (caps sensitive!)\n");
    }
}