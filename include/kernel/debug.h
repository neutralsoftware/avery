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

enum class LogType {
    Console,
    Serial,
};

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
    void log(const char* message, LogType logType = LogType::Serial);
    void warn(const char* message, LogType logType = LogType::Serial);
    void error(const char* message, LogType logType = LogType::Serial);

    struct StackFrame {
        StackFrame* rbp;
        uptr rip;
    };

    void stackTrace(LogType logType = LogType::Serial);
}

#endif //AVERY_DEBUG_H
