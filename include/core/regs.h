/*
* regs.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Register functions
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_REGS_H
#define AVERY_REGS_H

#include <types.h>

namespace regs {
    inline u64 read_cr0() {
        u64 val;
        asm volatile(
            "mov %%cr0, %0"
            : "=r"(val)
        );

        return val;
    }

    inline void write_cr0(u64 val) {
        asm volatile(
            "mov %0, %%cr0" : : "r"(val) : "memory"
        );
    }

    inline u64 read_cr4() {
        u64 val;
        asm volatile(
            "mov %%cr4, %0"
            : "=r"(val));

        return val;
    }

    inline void write_cr4(u64 val) {
        asm volatile(
            "mov %0, %%cr4" : : "r"(val) : "memory"
        );
    }

    void enableSSE();
}


#endif //AVERY_REGS_H
