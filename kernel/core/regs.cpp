/*
* regs.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: 
* Copyright (c) 2026 Max Van den Eynde
*/

#include <core/regs.h>

void regs::enableSSE() {
    u64 cr0 = read_cr0();

    cr0 &= ~(1ull << 2);
    cr0 |= (1ull << 1);

    write_cr0(cr0);

    u64 cr4 = read_cr4();

    cr4 |= (1ull << 9);
    cr4 |= (1ull << 10);

    write_cr4(cr4);

    asm volatile("fninit");
}
