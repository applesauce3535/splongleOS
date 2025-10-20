#pragma once
#include <stdint.h>

void putc(char c);
void eatc();
void movecursor(uint32_t scancode);
void puts(const char* str);
void clrscr();
void printk(const char* fmt, ...);