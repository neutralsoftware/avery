/*
* gdt.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Managers for the Global Descriptor Table
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_GDT_H
#define AVERY_GDT_H
#include "../types.h"

#pragma once

struct GDTEntry {
    u16 limit_low;
    u16 base_low;
    u8 base_middle;
    u8 access;
    u8 granularity;
    u8 base_high;
} __attribute__((packed));

struct GDTPtr {
    u16 limit;
    uptr base;
} __attribute__((packed));

// Defined in assembly
extern "C" void gdt_flush();

namespace core {
    void initGdt();
}

namespace gdt {
    void setGate(i32 num, u64 base, u64 limit, u8 access, u8 granularity);
}

#endif //AVERY_GDT_H
