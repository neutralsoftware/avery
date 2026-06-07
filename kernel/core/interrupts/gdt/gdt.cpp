/*
* gdt.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Global Descriptor Table implementations
* Copyright (c) 2026 Max Van den Eynde
*/

#include <core/gdt.h>

#include "kernel/memory.h"

GDTEntry gdtEntries[7];
GDTPtr gp;
TSS tss;

namespace {
    alignas(16) u8 doubleFaultStack[16 * 1024];
}

void gdt::setGate(i32 num, u64 base, u64 limit, u8 access, u8 granularity) {
    gdtEntries[num].base_low = (base & 0xFFFF);
    gdtEntries[num].base_middle = (base >> 16) & 0xFF;
    gdtEntries[num].base_high = (base >> 24) & 0xFF;

    gdtEntries[num].limit_low = (limit & 0xFFFF);
    gdtEntries[num].granularity = (limit >> 16) & 0x0F;

    gdtEntries[num].granularity |= (granularity & 0xF0);
    gdtEntries[num].access = access;
}

void gdt::setTSSGate(i32 num, u64 base, u32 limit) {
    gdtEntries[num].limit_low = limit & 0xFFFF;
    gdtEntries[num].base_low = (base & 0xFFFF);
    gdtEntries[num].base_middle = (base >> 16) & 0xFF;

    gdtEntries[num].access = 0x89;
    gdtEntries[num].granularity = (limit >> 16) & 0x0F;

    gdtEntries[num].base_high = (base >> 24) & 0xFF;

    gdtEntries[num + 1].base_low = (base >> 32) & 0xFFFF;
    gdtEntries[num + 1].base_middle = (base >> 48) & 0xFF;
    gdtEntries[num + 1].base_high = 0;

    gdtEntries[num + 1].granularity = 0;
    gdtEntries[num + 1].access = 0;
    gdtEntries[num + 1].limit_low = 0;
}

void core::initGdt() {
    memory::set(reinterpret_cast<u8*>(&tss), static_cast<u8>(0), sizeof(TSS));

    tss.rsp0 = memory::getKernelStackTop();
    tss.ist1 = reinterpret_cast<u64>(doubleFaultStack + sizeof(doubleFaultStack));
    tss.ioMapBase = sizeof(TSS);

    gp.limit = sizeof(gdtEntries) - 1;
    gp.base = reinterpret_cast<uptr>(&gdtEntries);

    gdt::setGate(0, 0, 0, 0, 0);

    gdt::setGate(1, 0, 0xFFFFF, 0x9A, 0xA0); // kernel code
    gdt::setGate(2, 0, 0xFFFFF, 0x92, 0xC0); // kernel data

    gdt::setGate(3, 0, 0xFFFFF, 0xFA, 0xA0); // user code
    gdt::setGate(4, 0, 0xFFFFF, 0xF2, 0xC0); // user data

    gdt::setTSSGate(5, reinterpret_cast<u64>(&tss), sizeof(TSS) - 1);

    gdt_flush();
    tss_flush();
}
