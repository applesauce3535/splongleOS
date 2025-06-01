ASM=nasm
CC=gcc

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
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/splongleOS.img conv=notrunc
	mcopy -i $(BUILD_DIR)/splongleOS.img $(BUILD_DIR)/kernel.bin "::kernel.bin" 

#
# bootloader
#
bootloader: $(BUILD_DIR)/bootloader.bin
$(BUILD_DIR)/bootloader.bin: always
	$(ASM) $(SRC_DIR)/boot/boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin

#
# kernel
#
kernel: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: always
	$(ASM) $(SRC_DIR)/kernel/kernel.asm -f bin -o $(BUILD_DIR)/kernel.bin

#
# tools
#
tools_fat: $(BUILD_DIR)/tools/fat
$(BUILD_DIR)/tools/fat: always $(TOOLS_DIR)/fat/fat.c
	mkdir -p $(BUILD_DIR)/tools
	$(CC) -g -o $(BUILD_DIR)/tools/fat

#
# always
#
always:
	mkdir -p $(BUILD_DIR)

#
# clean
#
clean:
	rm -rf $(BUILD_DIR)/*