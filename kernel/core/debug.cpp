/*
* debug.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Debugging functions and more
* Copyright (c) 2026 Max Van den Eynde
*/

#include "../../include/kernel/debug.h"

#include "io/serial.h"
#include "kernel/console.h"

[[noreturn]] void debug::kernelPanic(const char* message, const char* file,
                                     [[maybe_unused]] int line, const char* function) {
    out::setColor(Color::white, Color::red);
    out::clear();

    out::println("");
    out::println("The Avery Kernel panicked!");
    out::println("Avery entered a bad execution state that resulted in an unrecoverable state.");
    out::println("Restart your computer and contact the developers if the problem keeps happening");
    out::println("=================");
    out::println("Crash Information:");
    out::println(message);
    out::println("");
    out::print("At file: ");
    out::println(file);
    out::println("");
    out::print("At function: ");
    out::println(function);

    asm volatile("cli");

    while (true) {
        asm volatile("hlt");
    }
}

void debug::log(const char* message, LogType logType) {
    if (logType == LogType::Serial) {
        io::serialWrite("[LOG] ");
        io::serialWrite(message);
        io::serialWrite("\n");
    }
    else {
        out::setColor(Color::green, Color::black);
        out::print("[LOG]");
        out::println(message);
    }
}


void debug::warn(const char* message, LogType logType) {
    if (logType == LogType::Serial) {
        io::serialWrite("[WARNING] ");
        io::serialWrite(message);
        io::serialWrite("\n");
    }
    else {
        out::setColor(Color::yellow, Color::black);
        out::print("[WARNING] ");
        out::println(message);
    }
}


void debug::error(const char* message, LogType logType) {
    if (logType == LogType::Serial) {
        io::serialWrite("[ERROR] ");
        io::serialWrite(message);
        io::serialWrite("\n");
    }
    else {
        out::setColor(Color::red, Color::black);
        out::print("[ERROR] ");
        out::println(message);
    }
}



