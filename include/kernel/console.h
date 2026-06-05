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
    void print(cstring str);
    void print(const string& str);
    void print(char c);
    void print(bool value);
    void print(u8 num);
    void print(u16 num);
    void print(u32 num);
    void print(u64 num);
    void print(i16 num);
    void print(i32 num);
    void print(i64 num);
    void print(const void* ptr);
    void println(cstring str);
    void println(const string& str);
    void println();
    void printHex(u64 num);
    void printNumber(u64 num);
    void printSigned(i64 num);
    void putChar(char c);
    void clear();

    template <typename T>
        requires ByteNumber<T>
    void print(T num) {
        if constexpr (T(-1) < T(0)) {
            printSigned(static_cast<i64>(num));
        }
        else {
            printNumber(static_cast<u64>(num));
        }
    }

    template <typename... Args>
        requires(sizeof...(Args) > 1)
    void print(const Args&... args) {
        (print(args), ...);
    }

    template <typename... Args>
        requires(sizeof...(Args) > 1)
    void println(const Args&... args) {
        print(args...);
        putChar('\n');
    }

    inline void switchTo(ConsoleOutputMode mode) {
        outputMode = mode;
    }

    void setColor(Color fg, Color bg);
};

namespace in {
    string getLine(cstring prompt = nullptr);
    char getChar();
}


#endif //AVERY_CONSOLE_H
