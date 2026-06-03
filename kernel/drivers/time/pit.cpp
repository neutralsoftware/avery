/*
* pit.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Programmable Interval Timer
* Copyright (c) 2026 Max Van den Eynde
*/

#include <drivers/pit.h>

#include "core/irq.h"
#include "io/io.h"

volatile u64 timerTicks;

constexpr int TICKS_PER_SECOND = 1000;

extern "C" void time_handler([[maybe_unused]] isr::Registers* regs) {
    timerTicks = timerTicks + 1;
}

void core::initPit() {
    int divisor = 1193180 / TICKS_PER_SECOND;
    io::outb(0x43, 0x36);
    io::outb(0x40, divisor & 0xFF);
    io::outb(0x40, (divisor >> 8) & 0xFF);

    irq::installHandler(0, time_handler);
}

u64 time::getUptime() {
    return timerTicks;
}

void time::wait(u64 ms) {
    u64 end = timerTicks + ms;

    while (timerTicks < end) {
        asm volatile("pause");
    }
}

