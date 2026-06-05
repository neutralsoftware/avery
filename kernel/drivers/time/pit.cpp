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

namespace {
    class PitDevice final : public Device {
    public:
        PitDevice() : Device("pit", DeviceType::Timer) {
        }
    };

    class PitDriver final : public Driver {
    public:
        string name() const override {
            return "pit";
        }

        bool probe(Device& device) override {
            return device.type() == DeviceType::Timer;
        }

        bool start(Device&) override {
            int divisor = 1193180 / TICKS_PER_SECOND;
            io::outb(0x43, 0x36);
            io::outb(0x40, static_cast<u8>(divisor & 0xFF));
            io::outb(0x40, static_cast<u8>((divisor >> 8) & 0xFF));

            irq::installHandler(0, time_handler);
            return true;
        }

        bool stop(Device&) override {
            irq::uninstallHandler(0);
            return true;
        }
    };

    PitDriver* registeredDriver = nullptr;
    PitDevice* registeredDevice = nullptr;
}

extern "C" void time_handler([[maybe_unused]] isr::Registers* regs) {
    timerTicks = timerTicks + 1;
}

void core::initPit() {
    time::registerDriver();
    time::registerDevice();
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

bool time::registerDriver() {
    if (!registeredDriver) {
        registeredDriver = new PitDriver();
    }

    return DriverManager::registerDriver(registeredDriver);
}

bool time::registerDevice() {
    if (!registeredDevice) {
        registeredDevice = new PitDevice();
    }

    return DeviceManager::registerDevice(registeredDevice);
}
