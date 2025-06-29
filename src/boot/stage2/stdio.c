#include "stdio.h"
#include "x86.h"

/*
writes a character to the output hardware

c - character to output
*/
void putc(char c) {
    x86_Video_WriteCharTeletype(c, 0);
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

int* printf_number(int* argp, int length, bool sign, int radix);

/*
writes a formatted string to the output hardware

fmt - string containing format specifications
all formatted parameters follow the string section
*/
void _cdecl printf(const char* fmt, ...) {
    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    bool sign = false;
    int* argp = (int*)&fmt;             // point to first arg, stack is aligned to size of int
    argp++;                             // increment argp to the next arg
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
                        putc((char)*argp);
                        argp++;
                        break;
                    case 's':
                        puts(*(char**)argp);
                        argp++;
                        break;
                    case '%':
                        putc('%');
                        break;
                    case 'd':
                    case 'i':
                        radix = 10;
                        sign = true;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    case 'u':
                        radix = 10;
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    case 'X':
                    case 'x':
                    case 'p':
                        radix = 16;
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    case 'o':
                        radix = 8;
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    // ignore invalid specifiers
                    default:
                        break;
                }
                // reset state and variables
                state = PRINTF_STATE_NORMAL;
                length = PRINTF_LENGTH_DEFAULT;
                radix = 10;
                sign = false;
                break;
                
            default:
                break;
        }
        fmt++;
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
int* printf_number(int* argp, int length, bool sign, int radix) {
    char buffer[32];
    unsigned long long number;
    int number_sign = 1;
    int pos = 0;                // keeps track of current buffer position

    // process length
    switch(length) {
        case PRINTF_LENGTH_SHORT_SHORT:
        case PRINTF_LENGTH_SHORT:
        case PRINTF_LENGTH_DEFAULT:
            if(sign) {
                int n = *argp;          // get the number
                if(n < 0) {
                    n = -n;             // need to do this to properly cast as u long long
                    number_sign = -1;
                }
                number = (unsigned long long)n;             // cast as u long long
            }
            else {
                number = *(unsigned int*)argp;
            }
            argp++;
            break;
        case PRINTF_LENGTH_LONG:
            if(sign) {
                long int n = *(long int*)argp;          // get the number
                if(n < 0) {
                    n = -n;             // need to do this to properly cast as u long long
                    number_sign = -1;
                }
                number = (unsigned long long)n;             // cast as u long long
            }
            else {
                number = *(unsigned long int*)argp;
            }
            argp+=2;
            break;
        case PRINTF_LENGTH_LONG_LONG:
            if(sign) {
                long long int n = *(long long int*)argp;          // get the number
                if(n < 0) {
                    n = -n;             // need to do this to properly cast as u long long
                    number_sign = -1;
                }
                number = (unsigned long long)n;             // cast as u long long
            }
            else {
                number = *(unsigned long long*)argp;
            }
            argp+=4;
            break;
    }

    // convert number to ASCII
    do {
        uint32_t remainder;
        x86_div64_32(number, radix, &number, &remainder);
        buffer[pos++] = g_HexChars[remainder];
    } while(number > 0);

    // add sign
    if(sign && number_sign < 0) {
        buffer[pos++] = '-';
    }

    // number is in reverse order in buffer
    while(--pos >= 0) {
        putc(buffer[pos]);
    }

    return argp;
}
