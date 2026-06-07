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

#include "fs/vfs.h"
#include "kernel/debug.h"
#include "kernel/exec/process.h"

extern "C" u64 syscall_handler(u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64, u64) {
    ASSERT(process::currentProcess != nullptr);
    debug::log("Dispatching system call with: ", number, ", ", arg0, ", ", arg1, ", ", arg2, ", ", arg3);
    switch (number) {
    case syscall::Exit:
        syscall::exit(static_cast<i32>(arg0));
        return 0;
    case syscall::Write:
        return syscall::write(static_cast<i32>(arg0), reinterpret_cast<void*>(arg1), arg2);
    case syscall::Read:
        return syscall::read(static_cast<i32>(arg0), reinterpret_cast<void*>(arg1), arg2);
    case syscall::MemoryMap:
        return syscall::mmap(reinterpret_cast<void*>(arg0), arg1, static_cast<int>(arg2), static_cast<int>(arg3));
    case syscall::MemoryUnmap:
        return syscall::memunmap(reinterpret_cast<void*>(arg0), arg1);
    case syscall::GetPID:
        return syscall::getPid();
    case syscall::Yield:
        syscall::yield();
        return 0;
    case syscall::Execute:
        return syscall::exec(reinterpret_cast<cstring>(arg0), reinterpret_cast<cstring*>(arg1));
    case syscall::Open:
        return syscall::open(reinterpret_cast<cstring>(arg0), static_cast<int>(arg1));
    case syscall::Close:
        return syscall::close(static_cast<i32>(arg0));
    case syscall::Seek:
        return syscall::seek(static_cast<i32>(arg0), arg1, static_cast<i32>(arg2));
    case syscall::DebugLog:
        syscall::debugLog(reinterpret_cast<cstring>(arg0));
        return 0;
    default:
        return static_cast<u64>(-1);
    }
}

void syscall::exit(i32 code) {
    process::currentProcess->exit(code);

    while (true) {
        asm volatile("cli; hlt");
    }
}

usize syscall::write(i32 fd, const void* buffer, usize count) {
    if (!process::currentProcess->addressSpace.userRangeReadable(reinterpret_cast<u64>(buffer), count)) {
        return static_cast<u64>(-1);
    }
    if (fd == 1) {
        // stdout
        const char* charBuff = static_cast<const char*>(buffer);

        for (usize i = 0; i < count; i++) {
            out::putChar(charBuff[i]);
        }

        return count;
    }
    if (fd == 2) {
        // stderr
    } else {
        Option<FileDescriptor&> descriptor = process::currentProcess->fileDescriptors[static_cast<u64>(fd)];
        if (descriptor.hasValue()) {
            if (descriptor.value().writable) {
                descriptor.value().handle->seek(descriptor.value().pos);
                return descriptor.value().handle->write(buffer, count);
            } else {
                debug::error("Tried to write to file descriptor ", fd, " but it was not writable.");
                return static_cast<u64>(-1);
            }
        } else {
            debug::error("Tried to access file descriptor ", fd, " but it did not exist");
            return static_cast<u64>(-1);
        }
    }
    return static_cast<u64>(-1);
}

usize syscall::read(i32 fd, void* buffer, usize count) {
    if (!process::currentProcess->addressSpace.userRangeWritable(reinterpret_cast<u64>(buffer), count)) {
        PANIC("Cannot write to the specified buffer in the read function");
    }
    auto charBuff = static_cast<char*>(buffer);
    if (fd == 0) {
        // stdin
        string cin = in::getLine();
        for (usize i = 0; i < (count - 1) && i < cin.length(); i++) {
            charBuff[i] = cin[i].valueOr('?');
        }
        charBuff[count] = '\0';
        return static_cast<u64>(count);
    } else {
        Option<FileDescriptor&> descriptor = process::currentProcess->fileDescriptors[static_cast<u64>(fd)];
        if (descriptor.hasValue()) {
            if (descriptor.value().readable) {
                descriptor.value().handle->seek(descriptor.value().pos);
                return descriptor.value().handle->read(buffer, count);
            } else {
                debug::error("Tried to write to file descriptor ", fd, " but it was not writable.");
                return static_cast<u64>(-1);
            }
        } else {
            debug::error("Tried to access file descriptor ", fd, " but it did not exist");
            return static_cast<u64>(-1);
        }
    }
}

u64 syscall::mmap(void* address, usize length, int protection, int flags) {
    u64 requestedAddress = reinterpret_cast<u64>(address);

    if (length == 0) {
        return 0;
    }

    if (requestedAddress != 0) {
        return 0;
    }

    if ((flags & memory::MapAnonymous) == 0) {
        return 0;
    }

    usize size = alignUp(length, static_cast<usize>(mmio::PageSize));

    u64 mapAddress = process::currentProcess->nextMmapBase;
    process::currentProcess->nextMmapBase += size;

    u64 pageFlags = vmm::FlagUser;

    if (protection & memory::ProtectionWrite) {
        pageFlags |= vmm::FlagWritable;
    }

    if ((protection & memory::ProtectionExecute) == 0) {
        pageFlags |= vmm::FlagNX;
    }

    auto result = process::currentProcess->addressSpace.mapNewUserRange(mapAddress, size, pageFlags);

    if (result != memory::MapResult::Ok) {
        return 0;
    }

    return mapAddress;
}

u64 syscall::memunmap(void* address, usize length) {
    u64 mapAddress = reinterpret_cast<u64>(address);

    if (length == 0) {
        return static_cast<u64>(-1);
    }

    if (mapAddress % mmio::PageSize != 0) {
        return static_cast<u64>(-1);
    }

    usize size = alignUp(length, static_cast<usize>(mmio::PageSize));

    auto result = process::currentProcess->addressSpace.unmapRange(mapAddress, size);

    if (result != memory::MapResult::Ok) {
        return static_cast<u64>(-1);
    }

    return 0;
}

u64 syscall::getPid() {
    return process::currentProcess->pid;
}

void syscall::yield() {
    PANIC("Yield is still in implementation");
}

u64 syscall::exec(cstring, cstring []) {
    PANIC("Execute is still in implementation");
    return static_cast<u64>(-1);
}

u64 syscall::open(cstring path, int flags) {
    if (!vfs::exists(path)) {
        return static_cast<u64>(-1);
    }

    int fdId = process::currentProcess->fileDescriptors.last().hasValue()
                   ? process::currentProcess->fileDescriptors.last().value().id + 1
                   : 1;
    FileHandle* handle = vfs::open(path, static_cast<VFSOpenMode>(flags));
    FileDescriptor fd{};
    fd.handle = handle;
    fd.id = fdId;
    fd.readable = true;
    fd.writable = true;
    fd.pos = 0;

    process::currentProcess->fileDescriptors.push(fd);
    return static_cast<u64>(fdId);
}

u64 syscall::close(int fd) {
    for (usize i = 0; i < process::currentProcess->fileDescriptors.size(); i++) {
        if (process::currentProcess->fileDescriptors[i].value().id == fd) {
            process::currentProcess->fileDescriptors.remove(i);
            return i;
        }
    }

    return static_cast<u64>(-1);
}

u64 syscall::seek(int fd, u64 offset, int whence) {
    for (usize i = 0; i < process::currentProcess->fileDescriptors.size(); i++) {
        FileDescriptor fileDescriptor = process::currentProcess->fileDescriptors[i].value();

        if (fileDescriptor.id == fd) {
            if (whence == SeekStart) {
                fileDescriptor.pos = offset;
            } else if (whence == SeekCurrent) {
                fileDescriptor.pos += offset;
            } else if (whence == SeekEnd) {
                fileDescriptor.pos = fileDescriptor.handle->size() - offset;
            }

            if (fileDescriptor.pos > fileDescriptor.handle->size()) {
                debug::error("Tried to access out of bounds in a file descriptor");
                return static_cast<u64>(-1);
            }
            return fileDescriptor.pos;
        }
    }

    return static_cast<u64>(-1);
}

void syscall::debugLog(cstring str) {
    debug::log(str);
}
