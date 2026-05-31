/*
* serial.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Serial implementation
* Copyright (c) 2026 Max Van den Eynde
*/

#include "types.h"
#include "io/serial.h"
#include "io/io.h"

void io::serialWriteChar(char c) {
    outb(0xE9, static_cast<u8>(c));
}

void io::serialWrite(string input) {
    while (*input) {
        serialWriteChar(*input);
        input++;
    }
}
