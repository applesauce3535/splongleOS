#include "utility.h"

uint32_t align(uint32_t number, uint32_t alignTo) {
    if (alignTo == 0) {
        return number;
    }
    uint32_t remainder = number % alignTo;
    return (remainder > 0) ? (number + alignTo - remainder) : number;
}
