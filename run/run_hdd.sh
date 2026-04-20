qemu-system-i386 \
  -m 512 \
  -serial stdio \
  -boot d \
  -cdrom build/splongleOS.iso \
  -drive file=build/splongleOS_hdd.img,format=raw,if=ide