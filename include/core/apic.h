/*
* apic.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Advanced PIC implementation
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_APIC_H
#define AVERY_APIC_H
#include "../types.h"

namespace lapic {
    inline u64 rdmsr(u32 msr) {
        u32 lo, hi;
        asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
        return (static_cast<u64>(hi) << 32) | lo;
    }

    inline void wrmsr(u32 msr, u64 value) {
        u32 lo = value & 0xffffffff;
        u32 hi = value >> 32;

        asm volatile("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
    }

    void initBase();
    void write(u32 reg, u32 value);
    u32 read(u32 reg);

    constexpr u32 LAPIC_SRV = 0xF0;
    void enable();

    constexpr u32 LAPIC_LVT_LINT0 = 0x350;
    void enableLegacyMode();
}

#endif //AVERY_APIC_H
