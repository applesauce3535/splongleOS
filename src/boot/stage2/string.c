#include "string.h"
#include "stdint.h"

// takes a string and character as input, returns the first matching instance
const char* strchr(const char* str, char chr) {
    if (str == NULL) {
        return NULL;
    }
    while (*str) {
        if (*str == chr) {
            return str;     // pointer to match loc
        }
        ++str;
    }
    return NULL;
}

// copies a string from one buffer to another, takes in destination and source
char* strcpy(char* dst, const char* src) {
    char* origDst = dst;    // points to beginning of destination

    if (dst == NULL) {
        return NULL;
    }
    if (src == NULL) {
        *dst = '\0';
        return dst;
    }
    while (*src) {

        *dst = *src;
        ++src;
        ++dst;
    }
    *dst = '\0';            // need to append 0 to end of destination string
    return origDst;
}

