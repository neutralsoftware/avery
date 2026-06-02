/*
* console.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Console modes and toggling
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_CONSOLE_H
#define AVERY_CONSOLE_H
#include "../types.h"
#include "../graphics/graphicsTypes.h"

#pragma once

class Framebuffer;
class FramebufferConsole;

enum class ConsoleOutputMode {
    Serial,
    Framebuffer,
};

extern FramebufferConsole* GlobalFramebufferConsole;

namespace out {
    extern ConsoleOutputMode outputMode;
    void initFramebufferConsole(const Framebuffer& framebuffer);
    void print(string str);
    void println(string str);
    void putChar(char c);
    void clear();

    inline void switchTo(ConsoleOutputMode mode) {
        outputMode = mode;
    }

    void setColor(Color fg, Color bg);
};


#endif //AVERY_CONSOLE_H
