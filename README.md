# Toy OS project
My own OS kernel. Utilizes GRUB as the bootloader, perhaps I'll change this at some point.
## How Do You Use This?
Download the ISO in build, install QEMU and simply execute "./run/run_iso.sh". Or if you want, put it on a flash drive and run on real hardware, it won't do much though.
## Current Features
-   GDT
-   IDT
-   ISR
-   IRQ
-   i8259A PIC driver
-   PS/2 keyboard driver
-   Memory detection with help from GRUB
## Upcoming Features
-   Memory management (almost done!)
-   Process scheduling (threading? synchronization primitives?)
-   NIC driver
-   Shell
-   System API
-   USB stack
-   VFS
-   Graphics?
-   FAT32 FS?