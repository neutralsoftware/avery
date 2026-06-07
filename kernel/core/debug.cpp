/*
* debug.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Debugging functions and more
* Copyright (c) 2026 Max Van den Eynde
*/

#include "../../include/kernel/debug.h"

#include "core/isr.h"
#include "io/serial.h"
#include "kernel/console.h"
#include "kernel/memory/virtualMemory.h"

namespace {
    struct LiveRegisters {
        u64 r15;
        u64 r14;
        u64 r13;
        u64 r12;
        u64 r11;
        u64 r10;
        u64 r9;
        u64 r8;
        u64 rsi;
        u64 rdi;
        u64 rbp;
        u64 rdx;
        u64 rcx;
        u64 rbx;
        u64 rax;
        u64 rip;
        u64 cs;
        u64 rflags;
        u64 rsp;
        u64 ss;
    };

    u64 readCr0() {
        u64 value;
        asm volatile("mov %%cr0, %0" : "=r"(value));
        return value;
    }

    u64 readCr2() {
        u64 value;
        asm volatile("mov %%cr2, %0" : "=r"(value));
        return value;
    }

    u64 readCr3() {
        u64 value;
        asm volatile("mov %%cr3, %0" : "=r"(value));
        return value;
    }

    u64 readCr4() {
        u64 value;
        asm volatile("mov %%cr4, %0" : "=r"(value));
        return value;
    }

    u64 readRsp() {
        u64 value;
        asm volatile("mov %%rsp, %0" : "=r"(value));
        return value;
    }

    u64 readRbp() {
        u64 value;
        asm volatile("mov %%rbp, %0" : "=r"(value));
        return value;
    }

    u64 readCs() {
        u16 value;
        asm volatile("mov %%cs, %0" : "=r"(value));
        return value;
    }

    u64 readSs() {
        u16 value;
        asm volatile("mov %%ss, %0" : "=r"(value));
        return value;
    }

    u64 readRflags() {
        u64 value;
        asm volatile("pushfq; pop %0" : "=r"(value));
        return value;
    }

    LiveRegisters captureLiveRegisters() {
        LiveRegisters registers{};

        asm volatile("mov %%r15, %0" : "=r"(registers.r15));
        asm volatile("mov %%r14, %0" : "=r"(registers.r14));
        asm volatile("mov %%r13, %0" : "=r"(registers.r13));
        asm volatile("mov %%r12, %0" : "=r"(registers.r12));
        asm volatile("mov %%r11, %0" : "=r"(registers.r11));
        asm volatile("mov %%r10, %0" : "=r"(registers.r10));
        asm volatile("mov %%r9, %0" : "=r"(registers.r9));
        asm volatile("mov %%r8, %0" : "=r"(registers.r8));
        asm volatile("mov %%rsi, %0" : "=r"(registers.rsi));
        asm volatile("mov %%rdi, %0" : "=r"(registers.rdi));
        asm volatile("mov %%rdx, %0" : "=r"(registers.rdx));
        asm volatile("mov %%rcx, %0" : "=r"(registers.rcx));
        asm volatile("mov %%rbx, %0" : "=r"(registers.rbx));
        asm volatile("mov %%rax, %0" : "=r"(registers.rax));

        registers.rbp = readRbp();
        registers.rsp = readRsp();
        registers.rip = reinterpret_cast<u64>(__builtin_return_address(0));
        registers.cs = readCs();
        registers.ss = readSs();
        registers.rflags = readRflags();

        return registers;
    }

    void writeHex(LogType logType, u64 value) {
        if (logType == LogType::Serial) {
            io::serialWriteHex(value);
            return;
        }

        out::printHex(value);
    }

    void writeRegister(LogType logType, cstring name, u64 value) {
        debug::writeValue(logType, name);
        debug::writeValue(logType, ": ");
        writeHex(logType, value);
        debug::writeLineEnd(logType);
    }

    void writePanicHeader(LogType logType, const char* message, const char* file, int line, const char* function) {
        debug::writeValue(logType, "");
        debug::writeLineEnd(logType);
        debug::writeValue(logType, "The Avery Kernel panicked!");
        debug::writeLineEnd(logType);
        debug::writeValue(logType, "Avery entered a bad execution state that resulted in an unrecoverable state.");
        debug::writeLineEnd(logType);
        debug::writeValue(logType, "Restart your computer and contact the developers if the problem keeps happening");
        debug::writeLineEnd(logType);
        debug::writeValue(logType, "=================");
        debug::writeLineEnd(logType);
        debug::writeValue(logType, "Crash Information:");
        debug::writeLineEnd(logType);
        debug::writeValue(logType, message);
        debug::writeLineEnd(logType);
        debug::writeLineEnd(logType);
        debug::writeValue(logType, "At file: ");
        debug::writeValue(logType, file);
        debug::writeLineEnd(logType);
        debug::writeValue(logType, "At line: ");
        debug::writeValue(logType, static_cast<u64>(line));
        debug::writeLineEnd(logType);
        debug::writeValue(logType, "At function: ");
        debug::writeValue(logType, function);
        debug::writeLineEnd(logType);
    }

    void writeControlRegisters(LogType logType) {
        writeRegister(logType, "CR0", readCr0());
        writeRegister(logType, "CR2", readCr2());
        writeRegister(logType, "CR3", readCr3());
        writeRegister(logType, "CR4", readCr4());
    }

    void writeLiveRegisters(LogType logType) {
        LiveRegisters registers = captureLiveRegisters();

        debug::writeValue(logType, "Registers:");
        debug::writeLineEnd(logType);
        writeRegister(logType, "RAX", registers.rax);
        writeRegister(logType, "RBX", registers.rbx);
        writeRegister(logType, "RCX", registers.rcx);
        writeRegister(logType, "RDX", registers.rdx);
        writeRegister(logType, "RSI", registers.rsi);
        writeRegister(logType, "RDI", registers.rdi);
        writeRegister(logType, "RBP", registers.rbp);
        writeRegister(logType, "RSP", registers.rsp);
        writeRegister(logType, "R8", registers.r8);
        writeRegister(logType, "R9", registers.r9);
        writeRegister(logType, "R10", registers.r10);
        writeRegister(logType, "R11", registers.r11);
        writeRegister(logType, "R12", registers.r12);
        writeRegister(logType, "R13", registers.r13);
        writeRegister(logType, "R14", registers.r14);
        writeRegister(logType, "R15", registers.r15);
        writeRegister(logType, "RIP", registers.rip);
        writeRegister(logType, "CS", registers.cs);
        writeRegister(logType, "RFLAGS", registers.rflags);
        writeRegister(logType, "SS", registers.ss);
        writeControlRegisters(logType);
    }

    void writeSavedRegisters(LogType logType, const isr::Registers* registers) {
        debug::writeValue(logType, "Registers:");
        debug::writeLineEnd(logType);
        writeRegister(logType, "RAX", registers->rax);
        writeRegister(logType, "RBX", registers->rbx);
        writeRegister(logType, "RCX", registers->rcx);
        writeRegister(logType, "RDX", registers->rdx);
        writeRegister(logType, "RSI", registers->rsi);
        writeRegister(logType, "RDI", registers->rdi);
        writeRegister(logType, "RBP", registers->rbp);
        writeRegister(logType, "RSP", registers->rsp);
        writeRegister(logType, "R8", registers->r8);
        writeRegister(logType, "R9", registers->r9);
        writeRegister(logType, "R10", registers->r10);
        writeRegister(logType, "R11", registers->r11);
        writeRegister(logType, "R12", registers->r12);
        writeRegister(logType, "R13", registers->r13);
        writeRegister(logType, "R14", registers->r14);
        writeRegister(logType, "R15", registers->r15);
        writeRegister(logType, "RIP", registers->rip);
        writeRegister(logType, "CS", registers->cs);
        writeRegister(logType, "RFLAGS", registers->rflags);
        writeRegister(logType, "SS", registers->ss);
        writeRegister(logType, "INT", registers->int_no);
        writeRegister(logType, "ERR", registers->err_code);
        writeControlRegisters(logType);
    }

    void finishPanic(bool hlt) {
        asm volatile("cli");

        if (hlt) {
            while (true) {
                asm volatile("hlt");
            }
        }
    }
}

void debug::kernelPanic(const char* message, const char* file,
                        int line, const char* function, bool hlt) {
    writePanicHeader(LogType::Serial, message, file, line, function);
    writeLiveRegisters(LogType::Serial);
    vmm::switchToKernelAddressSpace();

    out::switchTo(ConsoleOutputMode::Framebuffer);
    out::setColor(Color::white, Color::red);
    out::clear();

    writePanicHeader(LogType::Console, message, file, line, function);
    writeLiveRegisters(LogType::Console);

    finishPanic(hlt);
}

void debug::kernelPanic(const char* message, const char* file,
                        int line, const char* function, const isr::Registers* registers, bool hlt) {
    writePanicHeader(LogType::Serial, message, file, line, function);
    if (registers) {
        writeSavedRegisters(LogType::Serial, registers);
    }
    else {
        writeLiveRegisters(LogType::Serial);
    }
    vmm::switchToKernelAddressSpace();

    out::switchTo(ConsoleOutputMode::Framebuffer);
    out::setColor(Color::white, Color::red);
    out::clear();

    writePanicHeader(LogType::Console, message, file, line, function);
    if (registers) {
        writeSavedRegisters(LogType::Console, registers);
    }
    else {
        writeLiveRegisters(LogType::Console);
    }

    finishPanic(hlt);
}

void debug::log(const char* message, LogType logType) {
    log(logType, message);
}


void debug::warn(const char* message, LogType logType) {
    warn(logType, message);
}


void debug::error(const char* message, LogType logType) {
    error(logType, message);
}

void debug::writePrefix(LogType logType, cstring prefix) {
    if (logType == LogType::Serial) {
        io::serialWrite(prefix);
        return;
    }

    if (prefix[1] == 'L') {
        out::setColor(Color::green, Color::black);
    }
    else if (prefix[1] == 'W') {
        out::setColor(Color::yellow, Color::black);
    }
    else {
        out::setColor(Color::red, Color::black);
    }

    out::print(prefix);
}

void debug::writeValue(LogType logType, cstring value) {
    if (logType == LogType::Serial) {
        io::serialWrite(value);
        return;
    }

    out::print(value);
}

void debug::writeValue(LogType logType, const string& value) {
    writeValue(logType, value.cStr());
}

void debug::writeValue(LogType logType, char value) {
    if (logType == LogType::Serial) {
        io::serialWriteChar(value);
        return;
    }

    out::putChar(value);
}

void debug::writeValue(LogType logType, bool value) {
    writeValue(logType, value ? "true" : "false");
}

void debug::writeValue(LogType logType, u8 value) {
    writeValue(logType, static_cast<u64>(value));
}

void debug::writeValue(LogType logType, u16 value) {
    writeValue(logType, static_cast<u64>(value));
}

void debug::writeValue(LogType logType, u32 value) {
    writeValue(logType, static_cast<u64>(value));
}

void debug::writeValue(LogType logType, u64 value) {
    if (logType == LogType::Serial) {
        io::serialWriteNumber(value);
        return;
    }

    out::printNumber(value);
}

void debug::writeValue(LogType logType, i16 value) {
    writeValue(logType, static_cast<i64>(value));
}

void debug::writeValue(LogType logType, i32 value) {
    writeValue(logType, static_cast<i64>(value));
}

void debug::writeValue(LogType logType, i64 value) {
    if (value < 0) {
        writeValue(logType, '-');
        writeValue(logType, static_cast<u64>(-(value + 1)) + 1);
        return;
    }

    writeValue(logType, static_cast<u64>(value));
}

void debug::writeValue(LogType logType, const void* value) {
    if (logType == LogType::Serial) {
        io::serialWriteHex(reinterpret_cast<u64>(value));
        return;
    }

    out::printHex(reinterpret_cast<u64>(value));
}

void debug::writeLineEnd(LogType logType) {
    writeValue(logType, '\n');
}

void debug::stackTrace(LogType type) {
    StackFrame* frame;

    asm volatile("mov %%rbp, %0" : "=r"(frame));

    if (type == LogType::Serial) {
        io::serialWrite("Stack Trace: \n");
    }
    else {
        out::println("Stack Trace:");
    }

    for (int i = 0; frame && i < 32; i++) {
        if (type == LogType::Serial) {
            io::serialWrite("[STACK ");
            io::serialWriteNumber(static_cast<u32>(i));
            io::serialWrite("]: ");
            io::serialWriteHex(frame->rip);
            io::serialWrite("\n");
        }
        else {
            out::print("[STACK ");
            out::printNumber(static_cast<u32>(i));
            out::print("]: ");
            out::printHex(frame->rip);
            out::print("\n");
        }

        frame = frame->rbp;
    }
}
