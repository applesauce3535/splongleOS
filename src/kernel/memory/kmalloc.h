#pragma once

#include <stdint.h>

void Kmalloc_Init(uint32_t initialHeapSize);
void changeHeapSize(int newSize);