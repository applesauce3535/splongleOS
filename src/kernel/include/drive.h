#pragma once

#include <stdint.h>
#include "multiboot.h"
#include "stdio.h"

void parse_multiboot_driveinfo(multiboot_info_t* mbinfo);