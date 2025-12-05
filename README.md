# Toy OS project
My own 32-bit OS kernel. Utilizes GRUB as the bootloader, perhaps I'll change this at some point.
## How Do You Use This?
Clone the repo and execute "run_iso.sh". Or if you want, put it on a flash drive and run on real hardware, it won't do much though.
## Current Features
-   i8259A PIC driver
-   PS/2 keyboard driver
-   Memory detection with help from GRUB
-   Working on PCI stuff
-   PMM and VMM with paging
-   A minimal shell, essentially a proof of concept right now
-   i8254A PIT driver
## Upcoming Features
-   Dynamic memory management
-   Multitasking
-   Networking
-   System API with syscalls
-   USB stack
-   VFS
-   Graphics? Unnecessary but fun
## Other Plans
I have been considering porting the kernel over to 64-bit and using Limine or writing my own bootloader. Also, working with UEFI instead of BIOS. As of now, this OS was outdated from the moment I made it 32-bit, but that seemed to be the simplest path to take at the moment.