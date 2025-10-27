#pragma once

void PIT_Init();
// change PIT channel frequency
void set_PIT_channel_mode_frequency(uint8_t channel, uint8_t mode, uint16_t freq);
void print_CPU();
uint32_t get_ticks();
void PIT_Handler();
void sleep(uint32_t ms);