#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "physical_manager.h"
#include "string.h"
#include "keyboard.h"
#include "rtc.h"
#include "pc_speaker.h"
#include "i8254.h"

void Shell_Run();
void send_command(const char* cmd);