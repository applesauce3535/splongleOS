#include "stdio.h"
#include "arch/i686/io.h"
#include <stdarg.h>
#include <stdbool.h>

const unsigned SCREEN_WIDTH = 80;
const unsigned SCREEN_HEIGHT = 25;
const uint8_t DEFAULT_COLOR = 0x7;

uint8_t* g_ScreenBuffer = (uint8_t*)0xB8000;
int g_ScreenX= 0, g_ScreenY = 0;

void putchr(int x, int y, char c) {
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)] = c;
}

void putcolor(int x, int y, uint8_t color) {
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1] = color;
}

char getchr(int x, int y) {
    return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)];
}

uint8_t getcolor(int x, int y) {
    return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1];
}

/*
this function writes to a VGA port and tells it to move the cursor
*/
void setcursor(int x, int y) {
    int pos = y * SCREEN_WIDTH + x;
    // setting lower byte on 3D4 port
    i686_outb(0x3D4, 0x0F);
    i686_outb(0x3D5, (uint8_t)(pos & 0xFF));
    // setting upper byte on 3D5 port
    i686_outb(0x3D4, 0x0E);
    i686_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clrscr() {
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            putchr(x, y, ' ');
            putcolor(x, y, DEFAULT_COLOR);
        }
    }
    g_ScreenX = 0;
    g_ScreenY = 0;
    setcursor(g_ScreenX, g_ScreenY);
}

void scrollback(int lines) {
    for (int y = lines; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            putchr(x, y - lines, getchr(x, y));
            putcolor(x, y - lines, getcolor(x, y));
        }
    }

    for (int y = SCREEN_HEIGHT - lines; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            putchr(x, y, ' ');
            putcolor(x, y, DEFAULT_COLOR);
        }
    }
    g_ScreenY -= lines;
}

/*
writes a character to the output hardware

c - character to output
*/
void putc(char c) {
    switch (c) {
        case '\n':
            g_ScreenX = 0;
            g_ScreenY++;
            break;

        case '\r':
            g_ScreenX = 0;
            break;

        case '\t':
            for (int i = 0; i < 4 - (g_ScreenX % 4); ++i) {
                putc(' ');
            }
            break;
        
        default:
            putchr(g_ScreenX, g_ScreenY, c);
            g_ScreenX++;
            break;

    }
    if (g_ScreenX >= SCREEN_WIDTH) {
        g_ScreenY++;
        g_ScreenX = 0;
    }

    if (g_ScreenY >= SCREEN_HEIGHT) {
        scrollback(1);
    }

    setcursor(g_ScreenX, g_ScreenY);
}

/*
writes a string to the output hardware

str - string to output
*/
void puts(const char* str) {
    while(*str) {
        putc(*str);
        str++;
    }
}

const char g_HexChars[] = "0123456789abcdef";

/*
formats a number to a formatted string

argp - number to format
length - length of number in bits
sign - determines if the number is signed or not
radix - base number system used for number
*/
void printk_unsigned(unsigned long long number, int radix) {
    char buffer[32];
    int pos = 0;                // keeps track of current buffer position

    // convert number to ASCII
    do {
        unsigned long long remainder = number % radix;
        number /= radix;
        buffer[pos++] = g_HexChars[remainder];
    } while(number > 0);

    // number is in reverse order in buffer
    while(--pos >= 0) {
        putc(buffer[pos]);
    }
}

void printk_signed(signed long long number, int radix) {
    if (number < 0) {
        putc('-');
        printk_unsigned(-number, radix);
    }
    else {
        printk_unsigned(number, radix);
    }
}

#define PRINTF_STATE_NORMAL 0
#define PRINTF_STATE_LENGTH 1
#define PRINTF_STATE_LENGTH_SHORT 2
#define PRINTF_STATE_LENGTH_LONG 3
#define PRINTF_STATE_SPEC 4

#define PRINTF_LENGTH_DEFAULT 0
#define PRINTF_LENGTH_SHORT_SHORT 1
#define PRINTF_LENGTH_SHORT 2
#define PRINTF_LENGTH_LONG 3
#define PRINTF_LENGTH_LONG_LONG 4


/*
writes a formatted string to the output hardware

fmt - string containing format specifications
all formatted parameters follow the string section
*/
void printk(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    bool sign = false;
    bool number = false;
    
    while(*fmt) {
        switch(state) {
            // handle normal state
            case PRINTF_STATE_NORMAL:
                switch(*fmt) {
                    case '%':
                        state = PRINTF_STATE_LENGTH;
                        break;
                    default:
                        putc(*fmt);
                        break;
                }
                break;
            // handle length state
            case PRINTF_STATE_LENGTH:
                switch(*fmt) {
                    case 'h':
                        length = PRINTF_LENGTH_SHORT;
                        state = PRINTF_STATE_LENGTH_SHORT;
                        break;
                    case 'l':
                        length = PRINTF_LENGTH_LONG;
                        state = PRINTF_STATE_LENGTH_LONG;
                        break;
                    default:
                        goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_LENGTH_SHORT:
                if(*fmt == 'h') {
                    length = PRINTF_LENGTH_SHORT_SHORT;
                    state = PRINTF_STATE_SPEC;
                }
                else {
                    goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_LENGTH_LONG:
                if(*fmt == 'l') {
                    length = PRINTF_LENGTH_LONG_LONG;
                    state = PRINTF_STATE_SPEC;
                }
                else {
                    goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_SPEC:
                PRINTF_STATE_SPEC_:
                switch(*fmt){
                    
                    case 'c':
                        putc((char)va_arg(args, int));
                        break;
                        
                    case 's':
                        puts(va_arg(args, const char*));
                        break;

                    case '%':
                        putc('%');
                        break;

                    case 'd':
                    case 'i':
                        radix = 10;
                        sign = true;
                        number = true;
                        break;

                    case 'u':
                        radix = 10;
                        sign = false;
                        number = true;
                        break;
                        
                    case 'X':
                    case 'x':
                    case 'p':
                        radix = 16;
                        sign = false;
                        number = true;
                        break;

                    case 'o':
                        radix = 8;
                        sign = false;
                        number = true;
                        break;
                    // ignore invalid specifiers
                    default:
                        break;
                }

                if (number) {
                    if (sign) {
                        switch (length) {
                            case PRINTF_LENGTH_SHORT_SHORT:  
                            case PRINTF_LENGTH_SHORT: 
                            case PRINTF_LENGTH_DEFAULT: 
                                printk_signed(va_arg(args, int), radix);
                                break;
                            
                            case PRINTF_LENGTH_LONG:
                                printk_signed(va_arg(args, long), radix);
                                break;

                            case PRINTF_LENGTH_LONG_LONG:
                                printk_signed(va_arg(args, long long), radix);
                                break;
                        }
                    }
                    else {
                        switch (length) {
                            case PRINTF_LENGTH_SHORT_SHORT:  
                            case PRINTF_LENGTH_SHORT: 
                            case PRINTF_LENGTH_DEFAULT: 
                                printk_unsigned(va_arg(args, unsigned int), radix);
                                break;
                            
                            case PRINTF_LENGTH_LONG:
                                printk_unsigned(va_arg(args, unsigned long), radix);
                                break;

                            case PRINTF_LENGTH_LONG_LONG:
                                printk_unsigned(va_arg(args, unsigned long long), radix);
                                break;
                        }
                    }
                }

                // reset state and variables
                state = PRINTF_STATE_NORMAL;
                length = PRINTF_LENGTH_DEFAULT;
                radix = 10;
                sign = false;
                number = false;
                break;
                
            default:
                break;
        }
        fmt++;
    }
    va_end(args);
}

