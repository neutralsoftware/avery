/*
* lapic.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Local Advanced Programmable Interrupt Controller definitions
* Copyright (c) 2026 Max Van den Eynde
*/

#include <core/apic.h>

#include "drivers/driver.h"

namespace lapic {
    static mmio::Interface* interface = nullptr;

    constexpr physAddr LAPIC_PHYS = 0xFEE00000;

    constexpr u32 LAPIC_ID = 0x020;
    constexpr u32 LAPIC_EOI = 0x0B0;
    constexpr u32 LAPIC_SVR = 0x0F0;

    static u32 read(u32 offset) {
        return interface->read<u32>(offset);
    }

    static void write(u32 offset, u32 value) {
        interface->write<u32>(offset, value);
    }

    bool init() {
        interface = new mmio::Interface(LAPIC_PHYS, 0x1000);

        if (!interface->isValid()) {
            return false;
        }

        u32 svr = read(LAPIC_SVR);

        write(LAPIC_SVR, svr | 0x100 | 0xFF);

        return true;
    }

    void eoi() {
        write(LAPIC_EOI, 0);
    }

    u32 id() {
        return read(LAPIC_ID) >> 24;
    }
}
