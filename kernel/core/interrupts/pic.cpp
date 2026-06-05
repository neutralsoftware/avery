/*
* pic.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Functions for the PIC
* Copyright (c) 2026 Max Van den Eynde
*/

#include <core/pic.h>

#include "io/io.h"

namespace pic {
    constexpr u16 PIC1_COMMAND = 0x20;
    constexpr u16 PIC1_DATA = 0x21;
    constexpr u16 PIC2_COMMAND = 0xA0;
    constexpr u16 PIC2_DATA = 0xA1;

    void remap() {
        io::outb(PIC1_COMMAND, 0x11);
        io::outb(PIC2_COMMAND, 0x11);

        io::outb(PIC1_DATA, 0x20);
        io::outb(PIC2_DATA, 0x28);

        io::outb(PIC1_DATA, 0x04);
        io::outb(PIC2_DATA, 0x02);

        io::outb(PIC1_DATA, 0x01);
        io::outb(PIC2_DATA, 0x01);

        io::outb(PIC1_DATA, 0x00);
        io::outb(PIC2_DATA, 0x00);
    }

    void maskAll() {
        io::outb(PIC1_DATA, 0xFF);
        io::outb(PIC2_DATA, 0xFF);
    }

    void enableIRQ(u8 irq) {
        u16 port;

        if (irq < 8) {
            port = PIC1_DATA;
        }
        else {
            port = PIC2_DATA;
            irq -= 8;
        }

        u8 value = io::inb(port) & ~(1 << irq);
        io::outb(port, value);
    }

    void disableIRQ(u8 irq) {
        u16 port;

        if (irq < 8) {
            port = PIC1_DATA;
        }
        else {
            port = PIC2_DATA;
            irq -= 8;
        }

        u8 value = io::inb(port) | static_cast<u8>(1 << irq);
        io::outb(port, value);
    }

    void eoi(u8 irq) {
        if (irq >= 8) {
            io::outb(PIC2_COMMAND, 0x20);
        }
        io::outb(PIC1_COMMAND, 0x20);
    }
}
