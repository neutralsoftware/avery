/*
* debug.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: ${FILE_DESCRIPTION}
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_DEBUG_H
#define AVERY_DEBUG_H

#pragma once
#include "types.h"
#include "console.h"

enum class LogType {
    Console,
    Serial,
};

namespace isr {
    struct Registers;
}

#define PANIC(msg) debug::kernelPanic(msg, __FILE__, __LINE__, __func__)
#define PANICNOHLT(msg) debug::kernelPanic(msg, __FILE__, __LINE__, __func__, false)

#define VERIFY(expr)                                      \
do {                                                  \
if (!(expr)) {                                    \
debug::kernelPanic(                                  \
"VERIFY failed: " #expr,                  \
__FILE__,                                \
__LINE__,                                \
__func__                                 \
);                                            \
while (true) {} \
}                                                 \
} while (0)

#define CHECK(expr) \
do \
 {                                                  \
if (!(expr)) {                                    \
debug::warn(                                  \
"Condition not met" #expr "in" __FILE__                  \
);                                            \
}                                                 \
} while (0)

#define EXPECT(expr) \
do \
{                                                  \
if (!(expr)) {                                    \
debug::error(                                  \
"Condition not met" #expr "in" __FILE__                  \
);                                            \
}                                                 \
} while (0)

#define TEST_RESULT(expr) \
do \
{                                                  \
if (expr) {                                    \
out::setColor(Color::green, Color::blue); \
out::println("========================"); \
out::println("TEST SUCCEEDED"); \
out::setColor(Color::white, Color::blue); \
}                                                 \
else { \
out::setColor(Color::red, Color::blue); \
out::println("========================"); \
out::println("TEST FAILED"); \
out::setColor(Color::white, Color::blue); \
} \
} while (0)

#ifndef NDEBUG
#define ASSERT(expr) VERIFY(expr)
#else
#define ASSERT(expr) do { } while (0)
#endif

namespace debug {
    void kernelPanic(const char* message, const char* file, int line, const char* function,
                     bool hlt = true);
    void kernelPanic(const char* message, const char* file, int line, const char* function,
                     const isr::Registers* registers, bool hlt = true);
    void log(const char* message, LogType logType = LogType::Serial);
    void warn(const char* message, LogType logType = LogType::Serial);
    void error(const char* message, LogType logType = LogType::Serial);
    void writePrefix(LogType logType, cstring prefix);
    void writeValue(LogType logType, cstring value);
    void writeValue(LogType logType, const string& value);
    void writeValue(LogType logType, char value);
    void writeValue(LogType logType, bool value);
    void writeValue(LogType logType, u8 value);
    void writeValue(LogType logType, u16 value);
    void writeValue(LogType logType, u32 value);
    void writeValue(LogType logType, u64 value);
    void writeValue(LogType logType, i16 value);
    void writeValue(LogType logType, i32 value);
    void writeValue(LogType logType, i64 value);
    void writeValue(LogType logType, const void* value);
    void writeLineEnd(LogType logType);

    template <typename T>
        requires ByteNumber<T>
    void writeValue(LogType logType, T value) {
        writeValue(logType, static_cast<u64>(value));
    }

    template <typename... Args>
    void writeValues(LogType logType, const Args&... args) {
        (writeValue(logType, args), ...);
    }

    template <typename... Args>
        requires(sizeof...(Args) > 0)
    void log(LogType logType, const Args&... args) {
        writePrefix(logType, "[LOG] ");
        writeValues(logType, args...);
        writeLineEnd(logType);
    }

    template <typename... Args>
        requires(sizeof...(Args) > 1)
    void log(const Args&... args) {
        log(LogType::Serial, args...);
    }

    template <typename... Args>
        requires(sizeof...(Args) > 0)
    void warn(LogType logType, const Args&... args) {
        writePrefix(logType, "[WARNING] ");
        writeValues(logType, args...);
        writeLineEnd(logType);
    }

    template <typename... Args>
        requires(sizeof...(Args) > 1)
    void warn(const Args&... args) {
        warn(LogType::Serial, args...);
    }

    template <typename... Args>
        requires(sizeof...(Args) > 0)
    void error(LogType logType, const Args&... args) {
        writePrefix(logType, "[ERROR] ");
        writeValues(logType, args...);
        writeLineEnd(logType);
    }

    template <typename... Args>
        requires(sizeof...(Args) > 1)
    void error(const Args&... args) {
        error(LogType::Serial, args...);
    }

    inline void serialError(const char* message) {
        error(message, LogType::Serial);
    }

    struct StackFrame {
        StackFrame* rbp;
        uptr rip;
    };

    void stackTrace(LogType logType = LogType::Serial);
}

#endif //AVERY_DEBUG_H
