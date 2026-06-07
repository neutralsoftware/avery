/*
* process.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Process definitions
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_PROCESS_H
#define AVERY_PROCESS_H
#include "elf.h"
#include "../../types.h"
#include "kernel/memory/virtualMemory.h"

struct Executable {
    u64 entry;
    u64 userStackTop;

    u64 imageStart;
    u64 imageEnd;

    static elf::Result fromElf(const elf::File& file, Executable* outExecutable);
    static elf::Result createAndLoadElf(const elf::File& file, memory::AddressSpace& addressSpace,
                                        Executable* outExecutable);
};

using pid = u64;

struct Process;

namespace process {
    static constexpr u64 UserStackTop = 0x00007FFFFFFFF000;
    static Process* currentProcess = nullptr;

    pid allocatePid();
}

enum class ProcessState : u8 {
    Created,
    Ready,
    Running,
    Blocked,
    Exited
};

struct Process {
    pid pid;
    ProcessState state;

    memory::AddressSpace addressSpace;
    Executable executable;

    i32 exitCode;

    static Process* createFromElf(const elf::File& file);

    void destroy();
    void markReady();
    void markRunning();
    void exit(i32 code);
};

#endif //AVERY_PROCESS_H
