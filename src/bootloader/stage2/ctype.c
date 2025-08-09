#include "ctype.h"

/*
checks if a character is lower-case

chr - character to check
*/
bool islower(char chr) {
    return chr >= 'a' && chr <= 'z';
}

/*
converts a character to its uppercase variant if it is lowercase

chr - character to convert
*/
char toupper(char chr) {
    return islower(chr) ? (chr - 'a' + 'A') : chr;
}
