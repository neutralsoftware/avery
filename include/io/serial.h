/*
* serial.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: The serial output for Avery
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_SERIAL_H
#define AVERY_SERIAL_H
#include "../types.h"

namespace io {
    void serialWriteChar(char c);
    void serialWrite(string input);
    void serialWriteHex(u64 num);
    void serialWriteNumber(u64 num);
}

#endif //AVERY_SERIAL_H
