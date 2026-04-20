include build_scripts/config.mk
include build_scripts/toolchain.mk
.PHONY: all iso hdd kernel clean always

all: iso hdd

#
# ISO
#
iso: $(BUILD_DIR)/SplongleOS.iso

$(BUILD_DIR)/SplongleOS.iso: kernel_elf
	rm -rf iso
	mkdir -p iso/boot/grub
	if [ -f $(BUILD_DIR)/kernel.elf ]; then \
		cp $(BUILD_DIR)/kernel.elf iso/boot/kernel.elf; \
		printf '%s\n' 'menuentry "SplongleOS" {' '  multiboot /boot/kernel.elf' '  boot' '}' > iso/boot/grub/grub.cfg; \
	fi
	grub-mkrescue -o $(BUILD_DIR)/SplongleOS.iso iso || (echo "grub-mkrescue failed - ensure grub and xorriso are installed"; false)

#
# Hard drive image
#
hdd: $(BUILD_DIR)/SplongleOS_hdd.img

$(BUILD_DIR)/SplongleOS_hdd.img: kernel_elf
	@echo "Creating blank 64MB disk image..."
	dd if=/dev/zero of=$@ bs=1M count=64

	@echo "Setting up loop device, partitioning, formatting, and installing kernel..."
	bash -c '\
	LOOPDEV=$$(sudo losetup -f --show $@) && \
	echo "Using loop device: $$LOOPDEV" && \
	sudo parted -s $$LOOPDEV mklabel msdos && \
	sudo parted -s $$LOOPDEV mkpart primary ext2 1MiB 100% && \
	sudo partprobe $$LOOPDEV && \
	sudo mkfs.ext2 -F $${LOOPDEV}p1 && \
	mkdir -p mnt && \
	sudo mount $${LOOPDEV}p1 mnt && \
	sudo mkdir -p mnt/boot/grub && \
	sudo cp $(BUILD_DIR)/kernel.elf mnt/boot/kernel.elf && \
	echo "menuentry \"SplongleOS\" {" | sudo tee mnt/boot/grub/grub.cfg > /dev/null && \
	echo "  multiboot /boot/kernel.elf" | sudo tee -a mnt/boot/grub/grub.cfg > /dev/null && \
	echo "  boot" | sudo tee -a mnt/boot/grub/grub.cfg > /dev/null && \
	echo "}" | sudo tee -a mnt/boot/grub/grub.cfg > /dev/null && \
	sudo grub-install --target=i386-pc --boot-directory=mnt/boot --force $$LOOPDEV && \
	sudo umount mnt && \
	sudo losetup -d $$LOOPDEV \
	'

	@echo "SplongleOS HDD image created at $@"

#
# kernel
#
kernel: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: always
	$(MAKE) -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR))

kernel_elf: $(BUILD_DIR)/kernel.elf
$(BUILD_DIR)/kernel.elf: always
	$(MAKE) -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR))

#
# always
#
always:
	mkdir -p $(BUILD_DIR)

#
# clean
#
clean:
	$(MAKE) -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	rm -rf $(BUILD_DIR)/*