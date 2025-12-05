qemu-system-i386 -cdrom build/splongleOS.iso -m 512 -serial stdio \
  -rtc base=localtime,clock=host,driftfix=slew \
  -audiodev id=pa,driver=pa \
  -machine pcspk-audiodev=pa \
  -device ahci,id=ahci0 \
  -drive if=none,file=run/disk.img,id=drive0 \
  -device ide-hd,drive=drive0,bus=ahci0.0