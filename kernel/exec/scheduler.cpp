/*
* scheduler.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Scheduler for the Avery Kernel
* Copyright (c) 2026 Max Van den Eynde
*/

#include <kernel/exec/process.h>

#include "core/gdt.h"
#include "kernel/debug.h"

Queue<Thread*>* scheduler::readyQueue = nullptr;
Thread* scheduler::current = nullptr;
u64 scheduler::ticks = 0;

void scheduler::init() {
    readyQueue = new Queue<Thread*>();
}

void scheduler::addThread(Thread* thread) {
    thread->state = ThreadState::Ready;
    readyQueue->push(thread);
}

Thread* scheduler::pickNext() {
    while (!readyQueue->empty()) {
        Thread* thread = readyQueue->pop().value();

        if (thread->state == ThreadState::Ready) {
            return thread;
        }
    }

    return nullptr;
}

void scheduler::yield() {
    Thread* old = current;
    Thread* next = pickNext();

    if (!next) {
        return;
    }

    if (old && old->state == ThreadState::Running) {
        old->state = ThreadState::Ready;
        readyQueue->push(old);
    }

    next->state = ThreadState::Running;
    current = next;
    process::currentProcess = next->process;

    next->process->addressSpace.activate();
    gdt::setTssRsp0(next->kernelStackTop);

    if (old) {
        debug::log("Switching from process ", old->process->pid, " to ", next->process->pid);

        thread::switchContext(&old->context, &next->context);
    } else {
        debug::log("Switching to process ", next->process->pid);

        CpuContext dummy{};
        thread::switchContext(&dummy, &next->context);
    }
}

static constexpr usize quantumTicks = 5;

void scheduler::onTimerTick() {
    ticks++;

    if (ticks % quantumTicks == 0) {
        yield();
    }
}
