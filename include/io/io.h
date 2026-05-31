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
}

#endif //AVERY_IO_H
