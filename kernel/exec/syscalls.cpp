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

extern "C" u64 syscall_handle(u64 number, u64 arg0, u64, u64, u64, u64, u64) {
    switch (number) {
    case syscall::Exit:
        syscall::exit(static_cast<i32>(arg0));
        return 0;
    default:
        return static_cast<u64>(-1);
    }
}

void syscall::exit(i32 code) {
    if (process::currentProcess) {
        process::currentProcess->exit(code);
        debug::log("Process ", process::currentProcess->pid, " exited with code ", code);
    }

    while (true) {
        asm volatile("cli; hlt");
    }
}
