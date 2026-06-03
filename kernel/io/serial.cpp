/*
* serial.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Serial implementation
* Copyright (c) 2026 Max Van den Eynde
*/

#include "../../include/types.h"
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

void io::serialWriteHex(u64 num) {
    char hex[19];
    const char* digits = "0123456789ABCDEF";
    usize index = 2;
    bool started = false;

    hex[0] = '0';
    hex[1] = 'x';

    for (u64 shift = 60; ; shift -= 4) {
        usize digit = static_cast<usize>((num >> shift) & 0xFULL);

        if (digit != 0 || started || shift == 0) {
            hex[index] = digits[digit];
            index++;
            started = true;
        }

        if (shift == 0) {
            break;
        }
    }

    hex[index] = '\0';

    serialWrite(hex);
}

void io::serialWriteNumber(u64 num) {
    char buffer[21];
    usize index = 20;

    buffer[index] = '\0';

    do {
        index--;
        buffer[index] = static_cast<char>('0' + (num % 10));
        num /= 10;
    }
    while (num != 0);

    serialWrite(&buffer[index]);
}
