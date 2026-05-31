#!/usr/bin/env bash

set -e

QEMU="$HOME/opt/qemu-custom/bin/qemu-system-x86_64"

"$QEMU" \
    -cdrom cmake-build-debug/avery.iso \
    -boot d \
    -m 256M \
    -debugcon stdio \
    -display cocoa