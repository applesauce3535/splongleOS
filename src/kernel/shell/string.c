#include <stdint.h>
#include <stdbool.h>
#include "string.h"

bool strcmp(const char* str1, const char* str2) {

    for (uint32_t i = 0; i < strlen(str2); ++i) {
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}

unsigned strlen(const char* str) {
    unsigned len = 0;
    while (*str) {
        ++len;
        ++str;
    }
    return len;
}
