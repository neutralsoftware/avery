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

enum class LogType {
    Console,
    Serial,
};

#define PANIC(msg) debug::kernelPanic(msg, __FILE__, __LINE__, __func__)

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

#ifndef NDEBUG
#define ASSERT(expr) VERIFY(expr)
#else
#define ASSERT(expr) do { } while (0)
#endif

namespace debug {
    [[noreturn]] void kernelPanic(const char* message, const char* file, int line, const char* function);
    void log(const char* message, LogType logType = LogType::Serial);
    void warn(const char* message, LogType logType = LogType::Serial);
    void error(const char* message, LogType logType = LogType::Serial);
}

#endif //AVERY_DEBUG_H
