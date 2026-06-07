/*
* idt.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Idt-related function implementation
* Copyright (c) 2026 Max Van den Eynde
*/

#include <core/idt.h>

#include "kernel/memory.h"

IDTEntry idtEntries[256];
IDTPtr idtp;

void idt::setGateWithIst(u8 num, u64 base, u16 sel, u8 flags, u8 ist) {
    idtEntries[num].offsetLow = (base & 0xFFFF);
    idtEntries[num].selector = sel;
    idtEntries[num].ist = static_cast<u8>(ist & 0x7u);
    idtEntries[num].flags = flags;
    idtEntries[num].offsetMid = (base >> 16) & 0xFFFF;
    idtEntries[num].offsetHigh = (base >> 32) & 0xFFFFFFFF;
    idtEntries[num].zero = 0;
}

void idt::setGate(u8 num, u64 base, u16 sel, u8 flags) {
    setGateWithIst(num, base, sel, flags, 0);
}

void core::initIdt() {
    idtp.limit = (sizeof(IDTEntry) * 256) - 1;
    idtp.base = reinterpret_cast<uptr>(&idtEntries);

    memory::set<u8>(reinterpret_cast<u8*>(&idtEntries), 0, sizeof(IDTEntry) * 256);

    idt_load();
}
