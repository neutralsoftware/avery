/*
* executable.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Loading and validation of executable types
* Copyright (c) 2026 Max Van den Eynde
*/

#include <kernel/exec/process.h>

#include "drivers/driver.h"
#include "kernel/memory.h"

elf::Result Executable::fromElf(const elf::File& file, Executable* outExecutable) {
    if (!outExecutable) {
        return elf::Result::InvalidArgument;
    }

    elf::Result result = file.validateExecutable();

    if (result != elf::Result::Ok) {
        return result;
    }

    bool foundLoadableSegment = false;

    u64 imageStart = U64_MAX;
    u64 imageEnd = 0;

    for (usize i = 0; i < file.header->programHeaderCount; i++) {
        const elf::ProgramHeader* programHeader = file.getProgramHeader(i);

        if (!programHeader) {
            return elf::Result::OutOfBounds;
        }

        if (!programHeader->isLoadable()) {
            continue;
        }

        foundLoadableSegment = true;

        u64 segmentStart = alignDown(programHeader->virtualAddress, mmio::PageSize);
        u64 segmentEnd = alignUp(programHeader->virtualAddress + programHeader->memorySize, mmio::PageSize);

        if (segmentEnd < segmentStart) {
            return elf::Result::Malformed;
        }

        if (segmentStart < imageStart) {
            imageStart = segmentStart;
        }

        if (segmentEnd > imageEnd) {
            imageEnd = segmentEnd;
        }
    }

    if (!foundLoadableSegment) {
        return elf::Result::Malformed;
    }

    if (file.header->entry < imageStart || file.header->entry > imageEnd) {
        return elf::Result::Malformed;
    }

    outExecutable->entry = file.header->entry;
    outExecutable->userStackTop = process::UserStackTop;
    outExecutable->imageStart = imageStart;
    outExecutable->imageEnd = imageEnd;


    return elf::Result::Ok;
}

elf::Result Executable::createAndLoadElf(const elf::File& file, memory::AddressSpace& addressSpace,
                                         Executable* outExecutable) {
    auto result = fromElf(file, outExecutable);

    if (result != elf::Result::Ok) {
        return result;
    }

    addressSpace.activate();

    for (usize i = 0; i < file.header->programHeaderCount; i++) {
        const elf::ProgramHeader* ph = file.getProgramHeader(i);

        if (!ph) {
            return elf::Result::OutOfBounds;
        }

        if (!ph->isLoadable()) {
            continue;
        }

        u64 segmentStart = alignDown(ph->virtualAddress, mmio::PageSize);
        u64 segmentEnd = alignUp(ph->virtualAddress + ph->memorySize, mmio::PageSize);

        usize segmentSize = segmentEnd - segmentStart;

        u64 flags = vmm::FlagUser;

        if (ph->isWritable()) {
            flags |= vmm::FlagWritable;
        }

        if (!ph->isExecutable()) {
            flags |= vmm::FlagNX;
        }

        if (ph->virtualAddress + ph->memorySize < ph->virtualAddress) {
            return elf::Result::Malformed;
        }

        auto mapResult = addressSpace.mapNewUserRange(segmentStart, segmentSize, flags);

        if (mapResult != memory::MapResult::Ok) {
            return elf::Result::Malformed;
        }

        memory::copy(reinterpret_cast<u8*>(ph->virtualAddress), file.data + ph->offset, static_cast<int>(ph->fileSize));

        memory::set(reinterpret_cast<u8*>(ph->virtualAddress + ph->fileSize), static_cast<u8>(0),
                    static_cast<int>(ph->memorySize - ph->fileSize));
    }

    constexpr usize UserStackSize = 1024 * 1024;

    auto stackResult = addressSpace.mapNewUserRange(outExecutable->userStackTop - UserStackSize, UserStackSize,
                                                    vmm::FlagUser | vmm::FlagWritable | vmm::FlagNX);

    if (stackResult != memory::MapResult::Ok) {
        return elf::Result::Malformed;
    }

    return elf::Result::Ok;
}
