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

void core::initSystems() {
    initGdt();
    debug::log("Initialized GDT");
    initIdt();
    debug::log("Initialized IDT");
}
