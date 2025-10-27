#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "memory/physical_manager.h"
#include "shell.h"
#include "string.h"
#include "dev/keyboard.h"
#include "arch/i686/rtc.h"

void Shell_Run() {
    char input[128];
    int pos = 0;
    printk("$>");
    while (true) {
        if (keyboard_haschar()) {
            char c = keyboard_getchar();

            if (c == '\n') {
                input[pos] = '\0';
                send_command(input);
                pos = 0;
                printk("$>");
            }
            else if (c == '\b' && pos > 0) {
                pos--;
                eatc();
            }
            else if (pos < sizeof(input) - 1) {
                    input[pos++] = c;
                    printk("%c", c);
            }
        }
    }
}


void send_command(const char* cmd) {
    if (strcmp(cmd, "help please") || strcmp(cmd, "please help")) {
        printk("\ngetmem - display physical memory information \
                \nversion - display splongleOS version \
                \nwhoami - display who you are \
                \ndatetime - toggle the date and time on screen \
                \n");
    }
    else if (strcmp(cmd, "help")) {
        printk("\nSay please\n");
    }
    else if (strcmp(cmd, "getmem")) {
        printk("\n");
        get_block_info();
        printk("\n");
    }
    else if (strcmp(cmd, "version")) {
        printk("\nsplongleOS V 35\n");
    }
    else if (strcmp(cmd, "whoami")) {
        printk("\nsplongle\n");
    }
    else if (strcmp(cmd, "datetime")) {
        printk("\n");
        set_show_datetime();
    }
    else {
        printk("\nUnrecognized command (caps sensitive!)\n");
    }
}