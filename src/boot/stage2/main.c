#include "stdint.h"
#include "stdio.h"

#pragma aux _cstart "*"

void _cdecl _cstart(uint16_t bootDrive) {
    puts("SplongleOS printf tests\r\n");
    printf("Formatted %% %c %s \r\n", 'a', "string");
    printf("Formatted %d %i %x %p %o %hd %hi %hhu %hhi \r\n", 1234, -5678, 0xdead, 0xbeef, 012345, (short)27, (short)-27, (unsigned char)35, (char)-10);
    printf("Formatted %ld %lx %lld %llx \r\n", -100000000l, 0xdeadbeeful, 10200300400ll, 0xdeadbeeffeebdaedull);
    for(;;);
}
