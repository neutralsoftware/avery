/*
* console.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Console related files
* Copyright (c) 2026 Max Van den Eynde
*/

#include "graphics/framebuffer.h"
#include "../../include/kernel/console.h"

#include "io/serial.h"

static FramebufferConsole* consoleAccess = nullptr;
FramebufferConsole* GlobalFramebufferConsole = nullptr;

alignas(FramebufferConsole) static u8 consoleStorage[sizeof(FramebufferConsole)];

ConsoleOutputMode out::outputMode = ConsoleOutputMode::Serial;

void out::initFramebufferConsole(const Framebuffer& framebuffer) {
    outputMode = ConsoleOutputMode::Framebuffer;

    auto* consoleMemory = reinterpret_cast<FramebufferConsole*>(consoleStorage);

    new(consoleMemory) FramebufferConsole(framebuffer,
                                          Color{255, 255, 255},
                                          Color{0, 0, 0}, 1.0f);
    consoleAccess = consoleMemory;
    GlobalFramebufferConsole = consoleMemory;
}

void out::print(string str) {
    if (outputMode == ConsoleOutputMode::Framebuffer && consoleAccess != nullptr) {
        consoleAccess->write(str);
    }
    else {
        io::serialWrite(str);
    }
}

void out::clear() {
    if (outputMode == ConsoleOutputMode::Framebuffer && consoleAccess != nullptr) {
        consoleAccess->clear();
    }
    else {
    }
}

void out::println(string str) {
    if (outputMode == ConsoleOutputMode::Framebuffer && consoleAccess != nullptr) {
        consoleAccess->writeLn(str);
    }
    else {
        io::serialWrite(str);
        io::serialWrite("\n");
    }
}

void out::setColor(Color fg, Color bg) {
    if (outputMode == ConsoleOutputMode::Framebuffer && consoleAccess != nullptr) {
        consoleAccess->setColor(fg, bg);
    }
    else {
    }
}

void out::putChar(char c) {
    if (outputMode == ConsoleOutputMode::Framebuffer && consoleAccess != nullptr) {
        consoleAccess->putChar(c);
    }
    else {
        io::serialWriteChar(c);
    }
}

void out::printHex(u64 num) {
    char hex[17]; // 16 digits + null terminator
    hex[16] = '\0';

    const char* digits = "0123456789ABCDEF";

    for (int i = 15; i >= 0; --i) {
        hex[i] = digits[num & 0xF];
        num >>= 4;
    }

    print("0x");
    print(hex);
}
