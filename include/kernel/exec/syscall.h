/*
* syscall.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: System calls for interoperating with user-space processes
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_SYSCALL_H
#define AVERY_SYSCALL_H
#include "../../types.h"

extern "C" u64 syscall_handle(u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5);

namespace syscall {
    constexpr u64 Exit = 0x00;

    void exit(i32 code);
}

#endif //AVERY_SYSCALL_H
