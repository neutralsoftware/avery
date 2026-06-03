/*
* lapic.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Local Advanced Programmable Interrupt Controler interfaces
* Copyright (c) 2026 Max Van den Eynde
*/

#include "core/apic.h"
#include "kernel/memory.h"

static volatile u32* LapicBase = nullptr;

void lapic::initBase() {
    u64 apicBaseMsr = rdmsr(0x1B);
    uptr phys = apicBaseMsr & 0xFFFFF000;
    u64 hhdm = memory::getHHDMOffset();
    LapicBase = reinterpret_cast<volatile u32*>(hhdm + phys);
}

void lapic::write(u32 reg, u32 value) {
    *reinterpret_cast<volatile u32*>(reinterpret_cast<uptr>(LapicBase) + reg) = value;
}

u32 lapic::read(u32 reg) {
    return *reinterpret_cast<volatile u32*>(reinterpret_cast<uptr>(LapicBase) + reg);
}

void lapic::enable() {
    write(LAPIC_SRV, read(LAPIC_SRV) | 0x100 | 0xFF);
}

void lapic::enableLegacyMode() {
    write(LAPIC_LVT_LINT0, 0x700);
}
