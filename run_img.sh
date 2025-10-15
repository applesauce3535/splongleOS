qemu-system-i386 \
    -drive file=build/splongleOS.img,format=raw,if=floppy \
    # debugging crap
    # -d cpu,exec,int,mmu \
    # -D qemu.log \
    # -no-reboot \
    # -no-shutdown \
    # -s -S