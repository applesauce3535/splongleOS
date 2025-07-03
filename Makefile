include build_scripts/config.mk

.PHONY: all floppy_image kernel bootloader clean always tools_fat

all: floppy_image tools_fat

include build_scripts/toolchain.mk

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
	mmd -i $(BUILD_DIR)/splongleOS.img "::bitch"
	mcopy -i $(BUILD_DIR)/splongleOS.img test.txt "::bitch/test.txt" 


#
# bootloader
#
bootloader: stage1 stage2

stage1: $(BUILD_DIR)/stage1.bin

$(BUILD_DIR)/stage1.bin: always
	$(MAKE) -C src/boot/stage1 BUILD_DIR=$(abspath $(BUILD_DIR))

stage2: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin: always
	$(MAKE) -C src/boot/stage2 BUILD_DIR=$(abspath $(BUILD_DIR))

#
# kernel
#
kernel: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: always
	$(MAKE) -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR))

#
# tools
#
tools_fat: $(BUILD_DIR)/tools/fat
$(BUILD_DIR)/tools/fat: always tools/fat/fat.c
	mkdir -p $(BUILD_DIR)/tools
	$(CC) -g -o $(BUILD_DIR)/tools/fat tools/fat/fat.c

#
# always
#
always:
	mkdir -p $(BUILD_DIR)

#
# clean
#
clean:
	$(MAKE) -C src/boot/stage1 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(MAKE) -C src/boot/stage2 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(MAKE) -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	rm -rf $(BUILD_DIR)/*