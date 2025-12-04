#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const char* Name;
    void (*SendEndOfInt)(int irq);
    bool (*Probe)(void);
    void (*Initialize)(uint8_t offsetPIC1, uint8_t offsetPIC2);
    void (*Disable)(void);
    void (*Mask)(int irq);
    void (*Unmask)(int irq);
} PICDriver;