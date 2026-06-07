/*
* syscalls.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Syscall implementation
* Copyright (c) 2026 Max Van den Eynde
*/

#include "../../include/types.h"
#include <kernel/exec/syscall.h>

#include "kernel/debug.h"
#include "kernel/exec/process.h"

extern "C" u64 syscall_handler(u64 number, u64 arg0, u64 arg1, u64 arg2, u64, u64, u64) {
    switch (number) {
    case syscall::Exit:
        syscall::exit(static_cast<i32>(arg0));
        return 0;
    case syscall::Write:
        return syscall::write(static_cast<i32>(arg0), reinterpret_cast<void*>(arg1), arg2);
    default:
        return static_cast<u64>(-1);
    }
}

void syscall::exit(i32 code) {
    if (process::currentProcess) {
        process::currentProcess->exit(code);
    } else {
        debug::error("Current process does not exist while trying to exit");
    }

    while (true) {
        asm volatile("cli; hlt");
    }
}

usize syscall::write(i32 fd, const void* buffer, usize count) {
    if (fd == 1) {
        // stdout
        const char* charBuff = static_cast<const char*>(buffer);

        for (usize i = 0; i < count; i++) {
            out::putChar(charBuff[i]);
        }

        return count;
    }
    PANIC("Other file descriptors are not yet implemented in syscalls");
    return 0;
}
