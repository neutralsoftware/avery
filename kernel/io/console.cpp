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

#include "drivers/keyboard.h"
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

void out::print(cstring str) {
    if (outputMode == ConsoleOutputMode::Framebuffer && consoleAccess != nullptr) {
        consoleAccess->write(str);
    }
    else {
        io::serialWrite(str);
    }
}

void out::print(const string& str) {
    print(str.cStr());
}

void out::clear() {
    if (outputMode == ConsoleOutputMode::Framebuffer && consoleAccess != nullptr) {
        consoleAccess->clear();
    }
    else {
    }
}

void out::println(cstring str) {
    if (outputMode == ConsoleOutputMode::Framebuffer && consoleAccess != nullptr) {
        consoleAccess->writeLn(str);
    }
    else {
        io::serialWrite(str);
        io::serialWrite("\n");
    }
}

void out::println(const string& str) {
    println(str.cStr());
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
    char hex[19];
    const char* digits = "0123456789ABCDEF";
    usize index = 2;
    bool started = false;

    hex[0] = '0';
    hex[1] = 'x';

    for (u64 shift = 60; ; shift -= 4) {
        auto digit = static_cast<usize>((num >> shift) & 0xFULL);

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

    print(hex);
}

void out::printNumber(u64 num) {
    char buffer[21];
    usize index = 20;

    buffer[index] = '\0';

    do {
        index--;
        buffer[index] = static_cast<char>('0' + (num % 10));
        num /= 10;
    }
    while (num != 0);

    print(&buffer[index]);
}

char in::getChar() {
    while (!keyboard::hasChar()) {
        if (out::outputMode == ConsoleOutputMode::Framebuffer && GlobalFramebufferConsole != nullptr) {
            GlobalFramebufferConsole->updateCursor();
        }

        asm volatile("pause");
    }

    return keyboard::getChar();
}

string in::getLine(cstring prompt) {
    string line;

    if (prompt != nullptr) {
        out::print(prompt);
    }

    while (true) {
        char c = getChar();

        if (c == '\n') {
            out::putChar('\n');
            return line;
        }

        if (c == '\b') {
            if (!line.empty()) {
                char removed = line.popBack();

                if (out::outputMode == ConsoleOutputMode::Framebuffer && GlobalFramebufferConsole != nullptr) {
                    GlobalFramebufferConsole->backspace(removed);
                }
                else {
                    out::putChar('\b');
                }
            }

            continue;
        }

        line.append(c);
        out::putChar(c);
    }
}
