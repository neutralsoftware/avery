/*
* keyboard.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Basic PS/2 driver support
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_KEYBOARD_H
#define AVERY_KEYBOARD_H
#include "../core/isr.h"

namespace keyboard {
    extern const unsigned char es[128];
    extern const unsigned char esShift[128];
    extern const unsigned char* activeLayout;

    bool hasChar();
    char getChar();
    void handler(isr::Registers* regs);
    void init();
}

#endif //AVERY_KEYBOARD_H
