qemu-system-i386 \
  -m 512 \
  -serial stdio \
  -rtc base=localtime,clock=host,driftfix=slew \
  -audiodev id=pa,driver=pa \
  -machine pcspk-audiodev=pa \
  -boot d \
  -drive file=build/splongleOS.iso,media=cdrom,if=ide