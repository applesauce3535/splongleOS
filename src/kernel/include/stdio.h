#pragma once
#include <stdint.h>

int getX();
int getY();
void setX(int x);
void setY(int y);
void setcursor(int x, int y);
void putc(char c);
void eatc();
void change_color(uint8_t color);
void default_color();
void movecursor(uint32_t scancode);
void puts(const char* str);
void clrscr();
void printk(const char* fmt, ...);