#include "string.h"
#include <stdint.h>
#include <stddef.h>

/*
takes a string and character as input, returns pointer to the first matching instance

str - input string
chr - character to match in the input string
*/
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

/*
copies a string from one buffer to another, takes in destination and source

dst - character buffer of destination
src - character buffer of source string
*/
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

/*
returns the number of characters in a string

str - string to count the characters in
*/
unsigned strlen(const char* str) {
    unsigned len = 0;
    while (*str) {
        ++len;
        ++str;
    }
    return len;
}
