ASM=nasm
CC=gcc
CC16=/opt/watcom/binl/wcc
LD16=/opt/watcom/binl/wlink

SRC_DIR=src
TOOLS_DIR=tools
BUILD_DIR=build

.PHONY: all floppy_image kernel bootloader clean always tools_fat

all: floppy_image tools_fat

#
# floppy image
#
floppy_image: $(BUILD_DIR)/splongleOS.img

$(BUILD_DIR)/splongleOS.img: bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/splongleOS.img bs=512 count=2880
	mkfs.fat -F 12 -n "NBOS" $(BUILD_DIR)/splongleOS.img 
	dd if=$(BUILD_DIR)/stage1.bin of=$(BUILD_DIR)/splongleOS.img conv=notrunc
	mcopy -i $(BUILD_DIR)/splongleOS.img $(BUILD_DIR)/stage2.bin "::stage2.bin" 
	mcopy -i $(BUILD_DIR)/splongleOS.img $(BUILD_DIR)/kernel.bin "::kernel.bin" 
	mcopy -i $(BUILD_DIR)/splongleOS.img test.txt "::test.txt" 

#
# bootloader
#
bootloader: stage1 stage2

stage1: $(BUILD_DIR)/stage1.bin

$(BUILD_DIR)/stage1.bin: always
	$(MAKE) -C $(SRC_DIR)/boot/stage1 BUILD_DIR=$(abspath $(BUILD_DIR))

stage2: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin: always
	$(MAKE) -C $(SRC_DIR)/boot/stage2 BUILD_DIR=$(abspath $(BUILD_DIR))

#
# kernel
#
kernel: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: always
	$(MAKE) -C $(SRC_DIR)/kernel BUILD_DIR=$(abspath $(BUILD_DIR))

#
# tools
#
tools_fat: $(BUILD_DIR)/tools/fat
$(BUILD_DIR)/tools/fat: always $(TOOLS_DIR)/fat/fat.c
	mkdir -p $(BUILD_DIR)/tools
	$(CC) -g -o $(BUILD_DIR)/tools/fat $(TOOLS_DIR)/fat/fat.c

#
# always
#
always:
	mkdir -p $(BUILD_DIR)

#
# clean
#
clean:
	$(MAKE) -C $(SRC_DIR)/boot/stage1 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(MAKE) -C $(SRC_DIR)/boot/stage2 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(MAKE) -C $(SRC_DIR)/kernel BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	rm -rf $(BUILD_DIR)/*