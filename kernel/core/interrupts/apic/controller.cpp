/*
* controller.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Interrupt controller
* Copyright (c) 2026 Max Van den Eynde
*/

#include <core/apic.h>

#include "core/pic.h"

namespace interruptController {
    static Backend backend = Backend::PIC;

    constexpr u8 IRQ_BASE = 0x20;

    constexpr physAddr IOAPIC_PHYS = 0xFEC00000;

    static u8 irqToVector(u8 irq) {
        return IRQ_BASE + irq;
    }

    void initPIC() {
        backend = Backend::PIC;

        pic::remap();
    }

    void initAPICCompat() {
        if (!ioapic::init(IOAPIC_PHYS)) {
            initPIC();
            return;
        }

        if (!lapic::init()) {
            initPIC();
            return;
        }

        backend = Backend::APIC;

        pic::maskAll();

        u8 lapicId = static_cast<u8>(lapic::id());

        for (u8 irq = 0; irq < 16; irq++) {
            ioapic::redirectIRQ(irq, irqToVector(irq), lapicId);
            ioapic::maskIRQ(irq);
        }
    }

    void enableIRQ(u8 irq) {
        if (backend == Backend::PIC) {
            pic::enableIRQ(irq);
        }
        else {
            ioapic::unmaskIRQ(irq);
        }
    }

    void disableIRQ(u8 irq) {
        if (backend == Backend::PIC) {
            pic::disableIRQ(irq);
        }
        else {
            ioapic::maskIRQ(irq);
        }
    }

    void eoi(u8 irq) {
        if (backend == Backend::PIC) {
            pic::eoi(irq);
        }
        else {
            lapic::eoi();
        }
    }

    Backend currentBackend() {
        return backend;
    }
}
