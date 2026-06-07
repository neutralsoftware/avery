/*
* idt.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: ${FILE_DESCRIPTION}
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_IDT_H
#define AVERY_IDT_H

#pragma once
#include "../types.h"

struct IDTEntry {
    u16 offsetLow;
    u16 selector;
    u8 ist;
    u8 flags;
    u16 offsetMid;
    u32 offsetHigh;
    u32 zero;
} __attribute__((packed));

struct IDTPtr {
    u16 limit;
    uptr base;
} __attribute__((packed));

extern "C" void idt_load();

namespace core {
    void initIdt();
}

namespace idt {
    void setGate(u8 num, u64 base, u16 sel, u8 flags);
    void setGateWithIst(u8 num, u64 base, u16 sel, u8 flags, u8 ist);
}


#endif //AVERY_IDT_H
