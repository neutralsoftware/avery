/*
* thread.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Thread interface for scheduler
* Copyright (c) 2026 Max Van den Eynde
*/

#include <kernel/exec/process.h>

#include "core/systems.h"

namespace thread {
    tid nextTid = 1;

    tid allocateTid() {
        return nextTid++;
    }
}

Thread* Thread::create(Process* process) {
    if (!process) {
        return nullptr;
    }

    Thread* thread = new Thread();

    thread->id = thread::allocateTid();
    thread->process = process;
    thread->state = ThreadState::Created;

    u8* stack = new u8[thread::KernelStackSize];

    thread->kernelStackBottom = reinterpret_cast<u64>(stack);
    thread->kernelStackTop = reinterpret_cast<u64>(stack + thread::KernelStackSize);

    thread->context = {};
    thread->context.rsp = thread->kernelStackTop;
    thread->context.rip = reinterpret_cast<u64>(threadStartTrampoline);
    return thread;
}

extern "C" [[noreturn]] void threadStartTrampoline() {
    Process* process = process::currentProcess;

    process->addressSpace.activate();

    core::enterUserMode(process->executable.entry, process->executable.userStackTop);
}
