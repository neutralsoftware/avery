/*
* gdt.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Global Descriptor Table implementations
* Copyright (c) 2026 Max Van den Eynde
*/

#include <core/gdt.h>

GDTEntry gdtEntries[3];
GDTPtr gp;

void gdt::setGate(i32 num, u64 base, u64 limit, u8 access, u8 granularity) {
    gdtEntries[num].base_low = (base & 0xFFFF);
    gdtEntries[num].base_middle = (base >> 16) & 0xFF;
    gdtEntries[num].base_high = (base >> 24) & 0xFF;

    gdtEntries[num].limit_low = (limit & 0xFFFF);
    gdtEntries[num].granularity = (limit >> 16) & 0x0F;

    gdtEntries[num].granularity |= (granularity & 0xF0);
    gdtEntries[num].access = access;
}

void core::initGdt() {
    gp.limit = (sizeof(GDTEntry) * 3) - 1;
    gp.base = reinterpret_cast<uptr>(&gdtEntries);

    gdt::setGate(0, 0, 0, 0, 0);
    gdt::setGate(1, 0, 0xFFFFFFFF, 0x9A, 0xA0);
    gdt::setGate(2, 0, 0xFFFFFFFF, 0x92, 0xC0);
    gdt_flush();
}
