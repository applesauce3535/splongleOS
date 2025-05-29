cmake_minimum_required(VERSION 3.20)

project(splongleOS)

set(BOOTLOADER_SRC ${CMAKE_SOURCE_DIR}/src/main.asm)

set(BOOTLOADER_BIN ${CMAKE_BINARY_DIR}/main.bin)

enable_language(ASM_NASM)

add_custom_command(
    OUTPUT ${BOOTLOADER_BIN}
    COMMAND nasm -f bin ${BOOTLOADER_SRC} -o ${BOOTLOADER_BIN}
    DEPENDS ${BOOTLOADER_SRC}
    COMMENT "Assembling bootloader to binary format"
)

add_custom_target(
    bootloader ALL
    DEPENDS ${BOOTLOADER_BIN}
)