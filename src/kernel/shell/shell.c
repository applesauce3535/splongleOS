#include <stdint.h>
#include "stdio.h"
#include "shell.h"
#include "string.h"

void send_command(const char* cmd) {
    if (strcmp(cmd, "help")) {
        printk("\nSay DOW\n");
    }
    else if (strcmp(cmd, "DOW")) {
        printk("\nDOW\n");
    }
    else if (strcmp(cmd, "funny")) {
        printk("\n35\n");
    }
    else {
        printk("\nUnrecognized command (caps sensitive!)\n");
    }
}