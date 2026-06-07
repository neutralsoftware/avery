/*
* process.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Process creation and management functions
* Copyright (c) 2026 Max Van den Eynde
*/

#include "../../include/kernel/exec/process.h"

#include "../../include/kernel/debug.h"
#include "../../include/kernel/memory/virtualMemory.h"

namespace process {
    static pid nextPid = 1;

    pid allocatePid() {
        return nextPid++;
    }
}

Process* Process::createFromElf(const elf::File& file) {
    Process* process = new Process();

    ASSERT(process != nullptr);

    process->pid = process::allocatePid();
    process->state = ProcessState::Created;
    process->exitCode = 0;

    auto addressResult = memory::AddressSpace::create(&process->addressSpace);

    if (addressResult != memory::MapResult::Ok) {
        delete process;
        return nullptr;
    }

    auto elfResult = Executable::createAndLoadElf(file, process->addressSpace, &process->executable);

    if (elfResult != elf::Result::Ok) {
        process->addressSpace.destroy();
        delete process;
        return nullptr;
    }

    process->state = ProcessState::Ready;
    return process;
}

void Process::destroy() {
    addressSpace.destroy();
    state = ProcessState::Exited;
}

void Process::markReady() {
    state = ProcessState::Ready;
}

void Process::markRunning() {
    state = ProcessState::Running;
}

void Process::exit(i32 code) {
    exitCode = code;
    state = ProcessState::Exited;
}
