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
    bool init();
    void eoi();

    u32 id();
}

namespace ioapic {
    bool init(physAddr physicalBase);

    u32 read(u8 reg);
    void write(u8 reg, u32 value);

    void redirectIRQ(u8 irq, u8 vector, u8 lapicId);
    void maskIRQ(u8 irq);
    void unmaskIRQ(u8 irq);
}

namespace interruptController {
    enum class Backend {
        PIC,
        APIC
    };

    void initPIC();
    void initAPICCompat();

    void enableIRQ(u8 irq);
    void disableIRQ(u8 irq);
    void eoi(u8 irq);

    Backend currentBackend();
}

#endif //AVERY_APIC_H
