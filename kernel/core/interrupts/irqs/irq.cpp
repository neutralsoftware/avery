/*
* irq.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Interrupt Requests
* Copyright (c) 2026 Max Van den Eynde
*/

#include "core/irq.h"

#include "core/apic.h"
#include "core/idt.h"
#include "io/io.h"

irq::IRQHandler irq_routines[16] = {
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};

void irq::installHandler(int irq, IRQHandler handler) {
    irq_routines[irq] = handler;
    enable(irq);
}

void irq::uninstallHandler(int irq) {
    disable(irq);
    irq_routines[irq] = nullptr;
}

void irq::enable(int irq) {
    interruptController::enableIRQ(static_cast<u8>(irq));
}

void irq::disable(int irq) {
    interruptController::disableIRQ(static_cast<u8>(irq));
}

void irq::eoi(int irq) {
    interruptController::eoi(static_cast<u8>(irq));
}

void core::initIrq() {
    idt::setGate(32, reinterpret_cast<uptr>(irq::irq0), 0x08, 0x8E);
    idt::setGate(33, reinterpret_cast<uptr>(irq::irq1), 0x08, 0x8E);
    idt::setGate(34, reinterpret_cast<uptr>(irq::irq2), 0x08, 0x8E);
    idt::setGate(35, reinterpret_cast<uptr>(irq::irq3), 0x08, 0x8E);
    idt::setGate(36, reinterpret_cast<uptr>(irq::irq4), 0x08, 0x8E);
    idt::setGate(37, reinterpret_cast<uptr>(irq::irq5), 0x08, 0x8E);
    idt::setGate(38, reinterpret_cast<uptr>(irq::irq6), 0x08, 0x8E);
    idt::setGate(39, reinterpret_cast<uptr>(irq::irq7), 0x08, 0x8E);
    idt::setGate(40, reinterpret_cast<uptr>(irq::irq8), 0x08, 0x8E);
    idt::setGate(41, reinterpret_cast<uptr>(irq::irq9), 0x08, 0x8E);
    idt::setGate(42, reinterpret_cast<uptr>(irq::irq10), 0x08, 0x8E);
    idt::setGate(43, reinterpret_cast<uptr>(irq::irq11), 0x08, 0x8E);
    idt::setGate(44, reinterpret_cast<uptr>(irq::irq12), 0x08, 0x8E);
    idt::setGate(45, reinterpret_cast<uptr>(irq::irq13), 0x08, 0x8E);
    idt::setGate(46, reinterpret_cast<uptr>(irq::irq14), 0x08, 0x8E);
    idt::setGate(47, reinterpret_cast<uptr>(irq::irq15), 0x08, 0x8E);

    interruptController::initAPICCompat();
}

extern "C" void irq_handler(isr::Registers* regs) {
    u64 vector = regs->int_no;

    if (vector < 32 || vector > 47)
        return;

    u8 irqNumber = static_cast<u8>(vector - 32);

    if (irq::IRQHandler handler = irq_routines[irqNumber]) {
        handler(regs);
    }

    irq::eoi(irqNumber);
}
