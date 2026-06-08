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

    Process* currentProcess = nullptr;

    pid allocatePid() {
        pid allocatedPid = nextPid++;
        debug::log("Allocated process pid ", allocatedPid);
        return allocatedPid;
    }
}

Process* Process::createFromElf(const elf::File& file) {
    debug::log("Creating process from ELF file");

    Process* process = new Process();

    ASSERT(process != nullptr);

    process->pid = process::allocatePid();
    process->state = ProcessState::Created;
    process->exitCode = 0;

    debug::log("Process ", process->pid, " allocated and marked created");
    auto addressResult = memory::AddressSpace::create(&process->addressSpace);

    if (addressResult != memory::MapResult::Ok) {
        debug::error("Process ", process->pid, " address space creation failed: result ",
                     static_cast<u32>(addressResult));
        delete process;
        return nullptr;
    }

    debug::log("Process ", process->pid, " address space created: pml4 ", process->addressSpace.pml4);
    auto elfResult = Executable::createAndLoadElf(file, process->addressSpace, &process->executable);

    if (elfResult != elf::Result::Ok) {
        debug::error("Process ", process->pid, " ELF load failed: result ", static_cast<u32>(elfResult));
        process->addressSpace.destroy();
        delete process;
        return nullptr;
    }

    process->state = ProcessState::Ready;
    debug::log("Process ", process->pid, " ready: entry ", process->executable.entry, " stack top ",
               process->executable.userStackTop);
    process->nextMmapBase = 0x0000004000000000;

    process->mainThread = Thread::create(process);

    if (!process->mainThread) {
        debug::error("Process ", process->pid, " could not create main thread");
        delete process;
        return nullptr;
    }

    process->mainThread->state = ThreadState::Ready;

    return process;
}

void Process::destroy() {
    debug::log("Destroying process ", pid, " address space ", addressSpace.pml4);
    addressSpace.destroy();
    state = ProcessState::Exited;
    debug::log("Process ", pid, " destroyed and marked exited");
}

void Process::markReady() {
    state = ProcessState::Ready;
    debug::log("Process ", pid, " marked ready");
}

void Process::markRunning() {
    state = ProcessState::Running;
    debug::log("Process ", pid, " marked running");
}

void Process::exit(i32 code) {
    exitCode = code;
    state = ProcessState::Exited;
    debug::log("Process ", pid, " exited with code ", code);
}
