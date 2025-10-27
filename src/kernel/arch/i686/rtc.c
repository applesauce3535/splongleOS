#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "asm_wrappers.h"
#include "irq.h"
#include "rtc.h"

#define RTC_DT_AREA     0x1610

datetime_t* g_datetime = (datetime_t*) RTC_DT_AREA;
bool show_datetime = false;

void RTC_Init() {
    i686_IRQ_RegisterHandler(8, &RTC_Handler);
}
void RTC_Handler() {
    datetime_t new_datetime, old_datetime;
    uint8_t reg_B;
    static uint16_t RTC_ticks = 0;
    i686_DisableInts();
    RTC_ticks++;
    // if 1 second passed get new date time values
    if (RTC_ticks % 1024 == 0) {
        RTC_ticks = 0;
        while (cmos_update_in_progress()) ; // spin
        new_datetime.second = get_RTC_register(0x00);
        new_datetime.minute = get_RTC_register(0x02);
        new_datetime.hour = get_RTC_register(0x04);
        new_datetime.day = get_RTC_register(0x07);
        new_datetime.month = get_RTC_register(0x08);
        new_datetime.year = get_RTC_register(0x09);

        do {
            old_datetime = new_datetime;

            while (cmos_update_in_progress()) ; // spin
            new_datetime.second = get_RTC_register(0x00);
            new_datetime.minute = get_RTC_register(0x02);
            new_datetime.hour   = get_RTC_register(0x04);
            new_datetime.day    = get_RTC_register(0x07);
            new_datetime.month  = get_RTC_register(0x08);
            new_datetime.year   = get_RTC_register(0x09);
        } while (old_datetime.second != new_datetime.second ||
                old_datetime.minute != new_datetime.minute ||
                old_datetime.hour != new_datetime.hour ||
                old_datetime.day != new_datetime.day ||
                old_datetime.month != new_datetime.month ||
                old_datetime.year != new_datetime.year);

        reg_B = get_RTC_register(0x0B);

        // convert BCD to binary (if bit 2 is clear)
        if (!(reg_B & 0x04)) {
            new_datetime.second = (new_datetime.second & 0x0F) + ((new_datetime.second / 16) * 10);
            new_datetime.minute = (new_datetime.minute & 0x0F) + ((new_datetime.minute / 16) * 10);
            new_datetime.hour   = ((new_datetime.hour & 0x0F) + (((new_datetime.hour & 0x70) / 16) * 10)) | (new_datetime.hour & 0x80);
            new_datetime.day    = (new_datetime.day & 0x0F) + ((new_datetime.day / 16) * 10);
            new_datetime.month  = (new_datetime.month & 0x0F) + ((new_datetime.month / 16) * 10);
            new_datetime.year   = (new_datetime.year & 0x0F) + ((new_datetime.year / 16) * 10);
        }

        // convert 12hr to 24hr (if bit 1 is clear and top bit of hour is set)
        if (!(reg_B & 0x02) && (new_datetime.hour & 0x80)) {
            new_datetime.hour = ((new_datetime.hour & 0x7F) + 12) % 24;
        }
        // get year by adding the current century, someone in the year 2100 remember to update this!
        new_datetime.year += 2000;
        // put values in memory
        *g_datetime = new_datetime;
        if (show_datetime) {
            print_datetime();
        }
        if (!show_datetime) {
            blank_datetime();
        }
    }
    // read reg C so future IRQ8s can occur
    get_RTC_register(0x0C);
    i686_EnableInts();
}

bool cmos_update_in_progress() {
    i686_outb(cmos_address, 0x8A);      // reads from status reg A, 0x80 disables nonmaskable interrupts
    return i686_inb(cmos_data) & 0x80;  // if reg A top bit is set, CMOS update is in progress
}

uint8_t get_RTC_register(uint8_t reg) {
    i686_outb(cmos_address, reg | 0x80);
    return i686_inb(cmos_data);
}

void enable_RTC() {
    uint8_t prev_B = get_RTC_register(0x0B);
    i686_outb(cmos_address, 0x8B);          // reselect reg B because reading a CMOS reg resets to reg D
    i686_outb(cmos_data, prev_B | 0x40);    // set bit 6 to enable periodic interrupts at 1024Hz
    get_RTC_register(0x0C);                 // read reg C to clear pending IRQ8 interrupts
}

void disable_RTC() {
    uint8_t prev_B;
    i686_DisableInts();
    prev_B = get_RTC_register(0x0B);
    i686_outb(cmos_address, 0x8B);
    i686_outb(cmos_data, prev_B & 0xBF);    // clear bit 6 to disable periodic interrupts
    i686_EnableInts();
}

void print_datetime() {
    int X = getX();
    int Y = getY();
    setX(60);
    setY(0);
    setcursor(60, 0);
    if (g_datetime->second >= 10) printk("%d-%d-%d %d:%d:%d", g_datetime->day, g_datetime->month, g_datetime->year,\
                                                            g_datetime->hour, g_datetime->minute, g_datetime->second);
    else printk("%d-%d-%d %d:%d:0%d", g_datetime->day, g_datetime->month, g_datetime->year,\
                                                            g_datetime->hour, g_datetime->minute, g_datetime->second);
    // restore cursor position
    setX(X);
    setY(Y);
    setcursor(X, Y);
}

void blank_datetime() {
    int X = getX();
    int Y = getY();
    setX(60);
    setY(0);
    setcursor(60, 0);
    printk("                    ");  // probably good enough
    // restore cursor position
    setX(X);
    setY(Y);
    setcursor(X, Y);
}

void set_show_datetime() {
    show_datetime = !show_datetime;
}