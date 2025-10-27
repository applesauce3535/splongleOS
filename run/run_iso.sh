qemu-system-i386 -cdrom build/splongleOS.iso -m 512 -serial stdio \
  -rtc base=localtime,clock=host,driftfix=slew \
  -audiodev id=pa,driver=pa \
  -machine pcspk-audiodev=pa