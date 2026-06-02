
cmake_minimum_required(VERSION 3.25)
project(Avery LANGUAGES CXX ASM_NASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KERNEL_ELF ${CMAKE_BINARY_DIR}/avery.elf)
set(ISO_ROOT ${CMAKE_BINARY_DIR}/iso)
set(ISO_FILE ${CMAKE_BINARY_DIR}/avery.iso)
set(LIMINE_DIR ${CMAKE_BINARY_DIR}/limine)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

file(GLOB_RECURSE KERNEL_SOURCES CONFIGURE_DEPENDS
        kernel/*.cpp
        kernel/*.asm
        include/*.h
        include/*.hpp
)

find_program(LLD_LINKER NAMES ld.lld REQUIRED)
message(STATUS "Using ELF linker: ${LLD_LINKER}")

add_library(avery_objects OBJECT ${KERNEL_SOURCES})

target_include_directories(avery_objects PRIVATE
        ${LIMINE_DIR}/limine-protocol/include
        include
)

target_compile_options(avery_objects PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-ffreestanding>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-stack-protector>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-pic>
        $<$<COMPILE_LANGUAGE:CXX>:-m64>
        $<$<COMPILE_LANGUAGE:CXX>:-mno-red-zone>
        $<$<COMPILE_LANGUAGE:CXX>:-mcmodel=kernel>
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
)

set(KERNEL_ELF ${CMAKE_BINARY_DIR}/avery.elf)

add_custom_command(
        OUTPUT ${KERNEL_ELF}
        COMMAND ${LLD_LINKER}
        -m elf_x86_64
        -nostdlib
        -static
        -T ${CMAKE_SOURCE_DIR}/linker.ld
        -o ${KERNEL_ELF}
        $<TARGET_OBJECTS:avery_objects>
        DEPENDS
        $<TARGET_OBJECTS:avery_objects>
        ${CMAKE_SOURCE_DIR}/linker.ld
        COMMAND_EXPAND_LISTS
        VERBATIM
)

add_custom_target(avery
        DEPENDS ${KERNEL_ELF}
)

set(LIMINE_BIN ${LIMINE_DIR}/bin)

add_custom_command(
        OUTPUT
        ${LIMINE_BIN}/limine-bios-cd.bin
        ${LIMINE_BIN}/limine-bios.sys
        ${LIMINE_BIN}/limine
        COMMAND /bin/sh -c
        "if [ ! -d '${LIMINE_DIR}/.git' ]; then rm -rf '${LIMINE_DIR}' && git clone --depth=1 --branch v12.3.2 https://github.com/limine-bootloader/limine.git '${LIMINE_DIR}'; fi"
        COMMAND ${CMAKE_COMMAND} -E chdir ${LIMINE_DIR} ./bootstrap
        COMMAND ${CMAKE_COMMAND} -E chdir ${LIMINE_DIR} ./configure --enable-bios --enable-bios-cd
        COMMAND ${CMAKE_COMMAND} -E chdir ${LIMINE_DIR} make
        COMMAND ${CMAKE_COMMAND} -E touch ${LIMINE_BIN}/limine-bios-cd.bin
        VERBATIM
)

add_custom_target(limine
        DEPENDS
        ${LIMINE_BIN}/limine-bios-cd.bin
        ${LIMINE_BIN}/limine-bios.sys
        ${LIMINE_BIN}/limine
)

add_dependencies(avery_objects limine)

add_custom_command(
        OUTPUT ${ISO_FILE}
        DEPENDS
        avery
        limine
        ${KERNEL_ELF}
        ${CMAKE_SOURCE_DIR}/limine.conf
        COMMAND rm -rf ${ISO_ROOT}
        COMMAND mkdir -p ${ISO_ROOT}/boot
        COMMAND cp ${KERNEL_ELF} ${ISO_ROOT}/boot/avery.elf
        COMMAND cp ${CMAKE_SOURCE_DIR}/limine.conf ${ISO_ROOT}/limine.conf
        COMMAND cp ${LIMINE_BIN}/limine-bios.sys ${ISO_ROOT}/limine-bios.sys
        COMMAND cp ${LIMINE_BIN}/limine-bios-cd.bin ${ISO_ROOT}/limine-bios-cd.bin
        COMMAND xorriso -as mkisofs
        -b limine-bios-cd.bin
        -no-emul-boot
        -boot-load-size 4
        -boot-info-table
        -o ${ISO_FILE}
        ${ISO_ROOT}
        COMMAND ${LIMINE_BIN}/limine bios-install ${ISO_FILE}
        VERBATIM
)

add_custom_target(iso
        DEPENDS ${ISO_FILE}
)