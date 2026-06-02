/*
* memory.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Memory functions implementation
* Copyright (c) 2026 Max Van den Eynde
*/

#include <kernel/memory.h>

void memory::copy(u8* dest, const u8* src, int count) {
    for (int i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}
