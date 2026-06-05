/*
* pit.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Programmable Interval Timer setup
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_PIT_H
#define AVERY_PIT_H
#include "../core/isr.h"
#include "driver.h"

extern "C" void time_handler(isr::Registers* regs);

namespace time {
    u64 getUptime();
    void wait(u64 ticksMs);
    bool registerDriver();
    bool registerDevice();
}

namespace core {
    void initPit();
}

#endif //AVERY_PIT_H
