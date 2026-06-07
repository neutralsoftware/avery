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

extern "C" u64 syscall_handler(u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5);

namespace syscall {
    constexpr u64 Exit = 0x00;
    constexpr u64 Write = 0x01;
    constexpr u64 Read = 0x02;
    constexpr u64 MemoryMap = 0x03;
    constexpr u64 MemoryUnmap = 0x04;
    constexpr u64 GetPID = 0x05;
    constexpr u64 Yield = 0x06;
    constexpr u64 Execute = 0x07;
    constexpr u64 Open = 0x08;
    constexpr u64 Close = 0x09;
    constexpr u64 Seek = 0x0A;
    constexpr u64 DebugLog = 0x0B;

    void exit(i32 code);
    usize write(i32 fd, const void* buffer, usize count);
    usize read(i32 fd, void* buffer, usize count);
    u64 mmap(void* address, usize length, int protection, int flags);
    u64 memunmap(void* address, usize length);
    u64 getPid();
    void yield();
    u64 exec(cstring path, cstring argv[]);
    u64 open(cstring path, int flags);
    u64 close(int fd);
    u64 seek(int fd, u64 offset, int whence);
    void debugLog(cstring str);

    constexpr u64 FlagAccessMode = 0x3;
    constexpr u64 FlagReadOnly = 0x0;
    constexpr u64 FlagWriteOnly = 0x1;
    constexpr u64 FlagReadWrite = 0x2;

    constexpr u64 FlagCreate = 0x100;
    constexpr u64 FlagExclusive = 0x200;
    constexpr u64 FlagTruncate = 0x1000;
    constexpr u64 FlagAppend = 0x2000;

    constexpr u64 SeekStart = 0;
    constexpr u64 SeekCurrent = 1;
    constexpr u64 SeekEnd = 2;
}

#endif //AVERY_SYSCALL_H
