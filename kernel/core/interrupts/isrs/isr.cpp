/*
* isr.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: ISR binding and definition
* Copyright (c) 2026 Max Van den Eynde
*/

#include "types.h"
#include "core/isr.h"

#include "core/idt.h"
#include "kernel/console.h"
#include "kernel/debug.h"

string exception_messages[] = {
    "Division By Zero Exception",
    "Debug Exception",
    "Non Maskable Interrupt Exception",
    "Breakpoint Exception",
    "Into Detected Overflow Exception",
    "Out of Bounds Exception",
    "Invalid Opcode Exception",
    "No Coprocessor Exception",
    "Double Fault Exception",
    "Coprocessor Segment Overrun Exception",
    "Bad TSS Exception",
    "Segment Not Present Exception",
    "Stack Fault Exception",
    "General Protection Fault Exception",
    "Page Fault Exception",
    "Unknown Interrupt Exception",
    "Coprocessor Fault Exception",
    "Alignment Check Exception (486+)",
    "Machine Check Exception (Pentium/586+)",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void core::initIsrs() {
    idt::setGate(0, reinterpret_cast<uptr>(isr::isr0), 0x08, 0x8E);
    idt::setGate(1, reinterpret_cast<uptr>(isr::isr1), 0x08, 0x8E);
    idt::setGate(2, reinterpret_cast<uptr>(isr::isr2), 0x08, 0x8E);
    idt::setGate(3, reinterpret_cast<uptr>(isr::isr3), 0x08, 0x8E);
    idt::setGate(4, reinterpret_cast<uptr>(isr::isr4), 0x08, 0x8E);
    idt::setGate(5, reinterpret_cast<uptr>(isr::isr5), 0x08, 0x8E);
    idt::setGate(6, reinterpret_cast<uptr>(isr::isr6), 0x08, 0x8E);
    idt::setGate(7, reinterpret_cast<uptr>(isr::isr7), 0x08, 0x8E);
    idt::setGate(8, reinterpret_cast<uptr>(isr::isr8), 0x08, 0x8E);
    idt::setGate(9, reinterpret_cast<uptr>(isr::isr9), 0x08, 0x8E);
    idt::setGate(10, reinterpret_cast<uptr>(isr::isr10), 0x08, 0x8E);
    idt::setGate(11, reinterpret_cast<uptr>(isr::isr11), 0x08, 0x8E);
    idt::setGate(12, reinterpret_cast<uptr>(isr::isr12), 0x08, 0x8E);
    idt::setGate(13, reinterpret_cast<uptr>(isr::isr13), 0x08, 0x8E);
    idt::setGate(14, reinterpret_cast<uptr>(isr::isr14), 0x08, 0x8E);
    idt::setGate(15, reinterpret_cast<uptr>(isr::isr15), 0x08, 0x8E);
    idt::setGate(16, reinterpret_cast<uptr>(isr::isr16), 0x08, 0x8E);
    idt::setGate(17, reinterpret_cast<uptr>(isr::isr17), 0x08, 0x8E);
    idt::setGate(18, reinterpret_cast<uptr>(isr::isr18), 0x08, 0x8E);
    idt::setGate(19, reinterpret_cast<uptr>(isr::isr19), 0x08, 0x8E);
    idt::setGate(20, reinterpret_cast<uptr>(isr::isr20), 0x08, 0x8E);
    idt::setGate(21, reinterpret_cast<uptr>(isr::isr21), 0x08, 0x8E);
    idt::setGate(22, reinterpret_cast<uptr>(isr::isr22), 0x08, 0x8E);
    idt::setGate(23, reinterpret_cast<uptr>(isr::isr23), 0x08, 0x8E);
    idt::setGate(24, reinterpret_cast<uptr>(isr::isr24), 0x08, 0x8E);
    idt::setGate(25, reinterpret_cast<uptr>(isr::isr25), 0x08, 0x8E);
    idt::setGate(26, reinterpret_cast<uptr>(isr::isr26), 0x08, 0x8E);
    idt::setGate(27, reinterpret_cast<uptr>(isr::isr27), 0x08, 0x8E);
    idt::setGate(28, reinterpret_cast<uptr>(isr::isr28), 0x08, 0x8E);
    idt::setGate(29, reinterpret_cast<uptr>(isr::isr29), 0x08, 0x8E);
    idt::setGate(30, reinterpret_cast<uptr>(isr::isr30), 0x08, 0x8E);
    idt::setGate(31, reinterpret_cast<uptr>(isr::isr31), 0x08, 0x8E);
}

extern "C" void fault_handler(isr::Registers* regs) {
    if (regs->int_no < 32) {
        out::print("Panic with error code: ");
        out::printHex(regs->err_code);
        PANIC(exception_messages[regs->int_no]);
    }
}
