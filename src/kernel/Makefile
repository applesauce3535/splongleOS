TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib -I. 
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T linker.ld -nostdlib

ELF_LINKFLAGS = -T linker_elf.ld -nostdlib

HEADERS_C = $(wildcard *.h) \
            $(wildcard */*.h) \
            $(wildcard */*/*.h) \
            $(wildcard */*/*/*.h)

SOURCES_C = $(wildcard *.c) \
            $(wildcard */*.c) \
            $(wildcard */*/*.c) \
            $(wildcard */*/*/*.c) \
            arch/i686/isrs_gen.c

OBJECTS_C = $(patsubst %.c, $(BUILD_DIR)/kernel/c/%.obj, $(SOURCES_C))

HEADERS_ASM = $(wildcard *.inc) \
              $(wildcard */*.inc) \
              $(wildcard */*/*.inc) \
              $(wildcard */*/*/*.inc) \
              arch/i686/isrs_gen.inc

SOURCES_ASM = $(wildcard *.asm) \
              $(wildcard */*.asm) \
              $(wildcard */*/*.asm) \
              $(wildcard */*/*/*.asm)


OBJECTS_ASM = $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.obj, $(SOURCES_ASM))


.PHONY: all kernel kernel_elf clean always

all: kernel kernel_elf


kernel_elf: $(BUILD_DIR)/kernel.elf

$(BUILD_DIR)/kernel.elf: $(OBJECTS_ASM) $(OBJECTS_C)
	$(TARGET_LD) $(ELF_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel_elf.map -o $@ $^ $(TARGET_LIBS)

kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel.map -o $@ $^ $(TARGET_LIBS)

$(BUILD_DIR)/kernel/c/%.obj: %.c $(HEADERS_C)
	mkdir -p $(dir $@)
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/kernel/asm/%.obj: %.asm $(HEADERS_ASM)
	mkdir -p $(dir $@)
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

arch/i686/isrs_gen.c arch/i686/isrs_gen.inc:
	@$(SOURCE_DIR)/build_scripts/gen_isrs.sh $@

always:
	mkdir -p $(BUILD_DIR)/kernel/c
	mkdir -p $(BUILD_DIR)/kernel/asm

clean:
	rm -f $(BUILD_DIR)/kernel.bin