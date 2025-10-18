include build_scripts/config.mk
include build_scripts/toolchain.mk
.PHONY: all iso hdd kernel clean always

all: iso hdd

#
# ISO
#
iso: $(BUILD_DIR)/splongleOS.iso

$(BUILD_DIR)/splongleOS.iso: kernel_elf
	rm -rf iso
	mkdir -p iso/boot/grub
	if [ -f $(BUILD_DIR)/kernel.elf ]; then \
		cp $(BUILD_DIR)/kernel.elf iso/boot/kernel.elf; \
		printf '%s\n' 'menuentry "splongleOS" {' '  multiboot /boot/kernel.elf' '  boot' '}' > iso/boot/grub/grub.cfg; \
	fi
	grub-mkrescue -o $(BUILD_DIR)/splongleOS.iso iso || (echo "grub-mkrescue failed - ensure grub and xorriso are installed"; false)

#
# Hard drive image
#
hdd: $(BUILD_DIR)/splongleOS_hdd.img

$(BUILD_DIR)/splongleOS_hdd.img: kernel_elf
# make blank 64MB disk
	dd if=/dev/zero of=$@ bs=1M count=64
# setup loop device
	LOOPDEV=$(sudo losetup -f)
	sudo losetup $LOOPDEV $@
	sudo mkfs.ext2 $$LOOPDEV
# mount kernel and copy to GRUB
	mkdir -p mnt
	sudo mount $$LOOPDEV mnt
	sudo mkdir -p mnt/boot/grub
	sudo cp $(BUILD_DIR)/kernel.elf mnt/boot/kernel.elf
	echo 'menuentry "splongleOS" {' | sudo tee mnt/boot/grub/grub.cfg
	echo '  multiboot /boot/kernel.elf' | sudo tee -a mnt/boot/grub/grub.cfg
	echo '  boot' | sudo tee -a mnt/boot/grub/grub.cfg
	echo '}' | sudo tee -a mnt/boot/grub/grub.cfg
# install GRUB
	sudo grub-install --target=i386-pc --boot-directory=mnt/boot --force $$LOOPDEV
# cleanup
	sudo umount mnt
	sudo losetup -d $$LOOPDEV

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