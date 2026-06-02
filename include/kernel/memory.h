/*
* memory.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Memory related functions
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_MEMORY_H
#define AVERY_MEMORY_H
#include "../types.h"

namespace memory {
    void copy(u8* dest, const u8* src, int count);

    template <typename T>
    void set(T* dest, T val, int count) {
        for (int i = 0; i < count; i++) {
            dest[i] = val;
        }
    }
}

#endif //AVERY_MEMORY_H
