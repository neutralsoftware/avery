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

class FileHandle;

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
using tid = u64;

struct Process;

namespace process {
    static constexpr u64 UserStackTop = 0x00007FFFFFFFF000;
    extern Process* currentProcess;

    pid allocatePid();
}

namespace memory {
    constexpr int ProtectionRead = 1 << 0;
    constexpr int ProtectionWrite = 1 << 1;
    constexpr int ProtectionExecute = 1 << 2;

    constexpr int MapAnonymous = 1 << 0;
}

enum class ProcessState : u8 {
    Created,
    Ready,
    Running,
    Blocked,
    Exited
};

struct FileDescriptor {
    int id;
    FileHandle* handle;
    bool readable;
    bool writable;
    usize pos;
};

struct Thread;

struct Process {
    pid pid;
    ProcessState state;
    u64 nextMmapBase;

    Vector<FileDescriptor> fileDescriptors;

    Thread* mainThread;

    memory::AddressSpace addressSpace;
    Executable executable;

    i32 exitCode;

    static Process* createFromElf(const elf::File& file);

    void destroy();
    void markReady();
    void markRunning();
    void exit(i32 code);
};

enum class ThreadState : u8 {
    Created,
    Ready,
    Running,
    Blocked,
    Dead
};

struct CpuContext {
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 rbx;
    u64 rbp;
    u64 rsp;
    u64 rip;
};

struct Thread {
    tid id;
    Process* process;
    ThreadState state;

    CpuContext context;

    u64 kernelStackBottom;
    u64 kernelStackTop;

    static Thread* create(Process* process);
    void destroy();
};

namespace thread {
    static constexpr usize KernelStackSize = 16 * 1024;
    tid allocateTid();

    extern "C" void switchContext(CpuContext* oldContext, CpuContext* newContext);
}

namespace scheduler {
    extern Queue<Thread*>* readyQueue;
    extern Thread* current;
    extern u64 ticks;

    inline Thread* currentThread() {
        return current;
    }

    void addThread(Thread* thread);
    Thread* pickNext();
    void yield();
    void init();
    void onTimerTick();
}

extern "C" [[noreturn]] void threadStartTrampoline();


#endif //AVERY_PROCESS_H
