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

void debug::kernelPanic(const char* message, const char* file,
                        [[maybe_unused]] int line, const char* function, bool hlt) {
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

    if (hlt) {
        while (true) {
            asm volatile("hlt");
        }
    }
}

void debug::log(const char* message, LogType logType) {
    log(logType, message);
}


void debug::warn(const char* message, LogType logType) {
    warn(logType, message);
}


void debug::error(const char* message, LogType logType) {
    error(logType, message);
}

void debug::writePrefix(LogType logType, cstring prefix) {
    if (logType == LogType::Serial) {
        io::serialWrite(prefix);
        return;
    }

    if (prefix[1] == 'L') {
        out::setColor(Color::green, Color::black);
    }
    else if (prefix[1] == 'W') {
        out::setColor(Color::yellow, Color::black);
    }
    else {
        out::setColor(Color::red, Color::black);
    }

    out::print(prefix);
}

void debug::writeValue(LogType logType, cstring value) {
    if (logType == LogType::Serial) {
        io::serialWrite(value);
        return;
    }

    out::print(value);
}

void debug::writeValue(LogType logType, const string& value) {
    writeValue(logType, value.cStr());
}

void debug::writeValue(LogType logType, char value) {
    if (logType == LogType::Serial) {
        io::serialWriteChar(value);
        return;
    }

    out::putChar(value);
}

void debug::writeValue(LogType logType, bool value) {
    writeValue(logType, value ? "true" : "false");
}

void debug::writeValue(LogType logType, u8 value) {
    writeValue(logType, static_cast<u64>(value));
}

void debug::writeValue(LogType logType, u16 value) {
    writeValue(logType, static_cast<u64>(value));
}

void debug::writeValue(LogType logType, u32 value) {
    writeValue(logType, static_cast<u64>(value));
}

void debug::writeValue(LogType logType, u64 value) {
    if (logType == LogType::Serial) {
        io::serialWriteNumber(value);
        return;
    }

    out::printNumber(value);
}

void debug::writeValue(LogType logType, i16 value) {
    writeValue(logType, static_cast<i64>(value));
}

void debug::writeValue(LogType logType, i32 value) {
    writeValue(logType, static_cast<i64>(value));
}

void debug::writeValue(LogType logType, i64 value) {
    if (value < 0) {
        writeValue(logType, '-');
        writeValue(logType, static_cast<u64>(-(value + 1)) + 1);
        return;
    }

    writeValue(logType, static_cast<u64>(value));
}

void debug::writeValue(LogType logType, const void* value) {
    if (logType == LogType::Serial) {
        io::serialWriteHex(reinterpret_cast<u64>(value));
        return;
    }

    out::printHex(reinterpret_cast<u64>(value));
}

void debug::writeLineEnd(LogType logType) {
    writeValue(logType, '\n');
}

void debug::stackTrace(LogType type) {
    StackFrame* frame;

    asm volatile("mov %%rbp, %0" : "=r"(frame));

    if (type == LogType::Serial) {
        io::serialWrite("Stack Trace: \n");
    }
    else {
        out::println("Stack Trace:");
    }

    for (int i = 0; frame && i < 32; i++) {
        if (type == LogType::Serial) {
            io::serialWrite("[STACK ");
            io::serialWriteNumber(static_cast<u32>(i));
            io::serialWrite("]: ");
            io::serialWriteHex(frame->rip);
            io::serialWrite("\n");
        }
        else {
            out::print("[STACK ");
            out::printNumber(static_cast<u32>(i));
            out::print("]: ");
            out::printHex(frame->rip);
            out::print("\n");
        }

        frame = frame->rbp;
    }
}


