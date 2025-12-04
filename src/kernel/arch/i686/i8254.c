/*
    Intel 8254 PIT device driver
*/

#include "include/i8254.h"

volatile uint32_t ticks = 0;

void PIT_Init() {
    i686_IRQ_RegisterHandler(0, &PIT_Handler);
}

void set_PIT_channel_mode_frequency(uint8_t channel, uint8_t mode, uint16_t freq) {
    if (channel > 2 || mode > 7) return;    // you can't do that, idiot
    // this needs to be done atomically
    i686_DisableInts();
    /* PIT I/O Ports:
     * 0x40 - channel 0     (read/write) 
     * 0x41 - channel 1     (read/write) 
     * 0x42 - channel 2     (read/write) 
     * 0x43 - Mode/Command  (write only) 
     *
     * 0x43 Command register value bits (1 byte):
     * 7-6 select channel:
     *      00 = channel 0
     *      01 = channel 1
     *      10 = channel 2
     *      11 = read-back command
     * 5-4 access mode:
     *      00 = latch count value
     *      01 = lobyte only
     *      10 = hibyte only
     *      11 = lobyte & hibyte
     * 3-1 operating mode:
     *      000 = mode 0 (interrupt on terminal count)
     *      001 = mode 1 (hardware re-triggerable one-shot)
     *      010 = mode 2 (rate generator)
     *      011 = mode 3 (square wave generator)
     *      100 = mode 4 (software triggered strobe)
     *      101 = mode 5 (hardware triggered strobe)
     *      110 = mode 6 (rate generator, same as 010)
     *      111 = mode 7 (square wave generator, same as 011)
     * 0  BCD/Binary mode:
     *      0 = 16bit binary
     *      1 = 4-digit BCD (x86 does not use this!)
     */

    // send command byte
    i686_outb(0x43, (channel << 6) | (0x03 << 4) | (mode << 1));
    
    // send frequency divider
    i686_outb(0x40 + channel, freq & 0xFF);         // lobyte
    i686_outb(0x40 + channel, (freq >> 8) & 0xFF);  // hibyte
    i686_EnableInts();

}

// used for reading time stamp counter (TSC)
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

uint64_t measure_cpu_freq() {
    uint64_t start_tsc, end_tsc;
    uint32_t start_ticks = ticks;

    // wait for a new PIT tick
    uint32_t wait_for = start_ticks + 10;  // wait 10 PIT ticks = ~100ms
    i686_DisableInts();
    start_tsc = rdtsc();
    i686_EnableInts();

    while (ticks < wait_for)
        ;  // spin wait

    i686_DisableInts();
    end_tsc = rdtsc();
    i686_EnableInts();

    uint64_t tsc_delta = end_tsc - start_tsc;
    double seconds = 10.0 / 100.0;  // 10 ticks at 100Hz = 0.1s
    double cpu_freq_hz = (double)tsc_delta / seconds;

    return (uint64_t)cpu_freq_hz;
}

void print_CPU() {
    int X = getX();
    int Y = getY();
    setX(57);
    setY(0);
    setcursor(57, 0);
    uint64_t cpu_freq = measure_cpu_freq();
    printk("CPU Frequency: %llu MHz\n", cpu_freq / 1000000);
    // restore cursor position
    setX(X);
    setY(Y);
    setcursor(X, Y);
}

uint32_t get_ticks() {
    return ticks;
}

// channel 0 handler
void PIT_Handler(Registers* regs) {
    ++ticks;
}

void sleep(uint32_t ms) {
    // make sure interrupts are enabled so PIT fires off
    i686_EnableInts();
    uint32_t start = ticks;
    uint32_t wait_ticks = (ms * 100) / 1000;  // convert ms to ticks (~100 ticks per second, 1000ms in 1s)
    if (wait_ticks == 0) wait_ticks = 1;      // always wait at least one tick

    while ((ticks - start) < wait_ticks) {
        __asm__ __volatile__("hlt");
    }
}