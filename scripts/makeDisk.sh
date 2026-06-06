#!/usr/bin/env bash

set -e

rm disk.img
qemu-img create -f raw disk.img 64M

mkfs.fat -F 32 disk.img
mdir -i disk.img ::
