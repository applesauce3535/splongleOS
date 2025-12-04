#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "isr.h"
#include "stdio.h"
#include "asm_wrappers.h"
#include "irq.h"

#define RTC_DT_AREA     0x1610  // mem address for pointer

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} __attribute__((packed)) datetime_t;

typedef enum {
    cmos_address    = 0x70,
    cmos_data       = 0x71
} CMOS_REGS;

void RTC_Init();
void RTC_Handler(Registers* regs);
// read CMOS update in progress bit in status register A
bool cmos_update_in_progress();
// get an RTC reg value
uint8_t get_RTC_register(uint8_t reg);
void enable_RTC();
void disable_RTC();
void print_datetime();
void blank_datetime();
void set_show_datetime();