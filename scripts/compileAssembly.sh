#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 3 ]; then
    printf 'Usage: %s <file.asm> <mtools-destination> <disk.img>\n' "$0" >&2
    exit 1
fi

asm_file="$1"
mtools_destination="$2"
disk_image="$3"

if [ ! -f "$asm_file" ]; then
    printf 'Assembly file not found: %s\n' "$asm_file" >&2
    exit 1
fi

if [ ! -f "$disk_image" ]; then
    printf 'Disk image not found: %s\n' "$disk_image" >&2
    exit 1
fi

mkdir -p build

base_name="$(basename "$asm_file")"
program_name="${base_name%.*}"
object_file="build/${program_name}.o"
elf_file="build/${program_name}"

nasm -f elf64 "$asm_file" -o "$object_file"
x86_64-elf-ld -e _start -nostdlib "$object_file" -o "$elf_file"
mcopy -o -i "$disk_image" "$elf_file" "$mtools_destination"
