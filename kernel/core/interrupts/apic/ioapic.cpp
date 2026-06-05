/*
* ioapic.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Input / Output Advanced Programmable Interrupt Controller
* Copyright (c) 2026 Max Van den Eynde
*/

#include <core/apic.h>

#include "drivers/driver.h"

namespace ioapic {
    static mmio::Interface* interface = nullptr;
    constexpr u32 IOAPIC_REGSEL = 0x00;
    constexpr u32 IOAPIC_WINDOW = 0x10;

    constexpr u8 IOAPIC_REDTBL = 0x10;

    bool init(physAddr physicalBase) {
        interface = new mmio::Interface(physicalBase, 0x1000);
        return interface->isValid();
    }

    u32 read(u8 reg) {
        interface->write<u32>(IOAPIC_REGSEL, reg);
        return interface->read<u32>(IOAPIC_WINDOW);
    }

    void write(u8 reg, u32 value) {
        interface->write<u32>(IOAPIC_REGSEL, reg);
        interface->write<u32>(IOAPIC_WINDOW, value);
    }

    void redirectIRQ(u8 irq, u8 vector, u8 lapicId) {
        u8 lowReg = IOAPIC_REDTBL + irq * 2;
        u8 highReg = lowReg + 1;

        u32 low = vector;

        low |= 0 << 8;

        low |= 0 << 11;

        low |= 0 << 13;

        low |= 0 << 15;

        low &= ~(1u << 16);

        u32 high = static_cast<u32>(lapicId) << 24;

        write(highReg, high);
        write(lowReg, low);
    }

    void maskIRQ(u8 irq) {
        u8 lowReg = IOAPIC_REDTBL + irq * 2;

        u32 low = read(lowReg);
        low |= 1u << 16;

        write(lowReg, low);
    }

    void unmaskIRQ(u8 irq) {
        u8 lowReg = IOAPIC_REDTBL + irq * 2;

        u32 low = read(lowReg);
        low &= ~(1u << 16);

        write(lowReg, low);
    }
}
