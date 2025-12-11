ENTRY(start)
OUTPUT_FORMAT("elf32-i386")
phys = 0x00100000;

PHDRS
{
    text PT_LOAD FLAGS(5);   /* R + X = 0b101 = 5 */
    data PT_LOAD FLAGS(6);   /* R + W = 0b110 = 6 */
}

SECTIONS
{
    . = phys;

    /* ------ Executable, read-only segment ------ */
    .multiboot : {
        *(.multiboot)
    } :text

    .entry : {
        __entry_start = .;
        *(.entry)
    } :text

    .text : {
        __text_start = .;
        *(.text*)
    } :text

    .rodata : {
        __rodata_start = .;
        *(.rodata*)
    } :text


    /* Align to page boundary so .data starts cleanly */
    . = ALIGN(4096);

    /* ------ Writable data segment ------ */
    .data : {
        __data_start = .;
        *(.data*)
    } :data

    .bss : {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
    } :data

    __end = .;
}
