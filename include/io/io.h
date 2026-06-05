/*
* io.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: I/O Mapping and writing functions
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_IO_H
#define AVERY_IO_H
#include "../types.h"

namespace io {
    inline void outb(u16 port, u8 value) {
        asm volatile(
            "outb %0, %1"
            :
            : "a"(value), "Nd"(port)
        );
    }

    inline u8 inb(u16 port) {
        u8 val;

        asm volatile(
            "inb %1, %0"
            : "=a"(val)
            : "Nd"(port)
        );

        return val;
    }

    inline void wait() {
        outb(0x80, 0);
    }

    inline void outl(u16 port, u32 value) {
        asm volatile(
            "outl %0, %1"
            :
            : "a"(value), "Nd"(port)
        );
    }

    inline void outw(u16 port, u16 value) {
        asm volatile(
            "outw %0, %1"
            :
            : "a"(value), "Nd"(port)
        );
    }

    inline u32 inl(u16 port) {
        u32 val;

        asm volatile(
            "inl %1, %0"
            : "=a"(val)
            : "Nd"(port)
        );

        return val;
    }

    inline u16 inw(u16 port) {
        u16 val;

        asm volatile(
            "inw %1, %0"
            : "=a"(val)
            : "Nd"(port)
        );

        return val;
    }
}
#endif //AVERY_IO_H
