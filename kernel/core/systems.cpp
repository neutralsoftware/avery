/*
* systems.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Core Kernel systems
* Copyright (c) 2026 Max Van den Eynde
*/

#include <core/gdt.h>
#include "kernel/debug.h"
#include <core/systems.h>

#include "core/idt.h"
#include "core/irq.h"
#include "core/isr.h"
#include "drivers/pci.h"
#include "kernel/memory.h"

void core::initSystems(volatile struct limine_memmap_request& request) {
    initGdt();
    debug::log("Initialized GDT");
    initIdt();
    debug::log("Initialized IDT");
    initIsrs();
    debug::log("All ISRs bound correctly");

    memory::initMemoryServices(request);

    initIrq();
    debug::log("All IRQs bound correctly");

    pci::enumerateAndCreateDevices();
    debug::log("Enumerated and created all PCI devices");

    asm volatile("sti");
}

extern "C" [[noreturn]] void enter_usermode_asm(u64 entry, u64 userStackTop);

void core::enterUserMode(u64 entry, u64 userStackTop) {
    debug::log("Entering usermode: entry ", entry, " user stack top ", userStackTop);
    enter_usermode_asm(entry, userStackTop);
}
