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
#include "kernel/debug.h"
#include "kernel/memory.h"

elf::Result Executable::fromElf(const elf::File& file, Executable* outExecutable) {
    debug::log("Creating executable metadata from ELF");

    if (!outExecutable) {
        debug::error("Executable metadata creation failed: missing output pointer");
        return elf::Result::InvalidArgument;
    }

    elf::Result result = file.validateExecutable();

    if (result != elf::Result::Ok) {
        debug::error("Executable metadata creation failed: ELF validation result ", static_cast<u32>(result));
        return result;
    }

    bool foundLoadableSegment = false;

    u64 imageStart = U64_MAX;
    u64 imageEnd = 0;

    for (usize i = 0; i < file.header->programHeaderCount; i++) {
        const elf::ProgramHeader* programHeader = file.getProgramHeader(i);

        if (!programHeader) {
            debug::error("Executable metadata creation failed: program header ", i, " out of bounds");
            return elf::Result::OutOfBounds;
        }

        if (!programHeader->isLoadable()) {
            debug::log("Skipping non-loadable ELF segment ", i, " type ", programHeader->type);
            continue;
        }

        foundLoadableSegment = true;

        u64 segmentStart = alignDown(programHeader->virtualAddress, mmio::PageSize);
        u64 segmentEnd = alignUp(programHeader->virtualAddress + programHeader->memorySize, mmio::PageSize);

        if (segmentEnd < segmentStart) {
            debug::error("Executable metadata creation failed: segment ", i, " range overflow");
            return elf::Result::Malformed;
        }

        debug::log("Executable segment ", i, " covers ", segmentStart, " through ", segmentEnd);

        if (segmentStart < imageStart) {
            imageStart = segmentStart;
        }

        if (segmentEnd > imageEnd) {
            imageEnd = segmentEnd;
        }
    }

    if (!foundLoadableSegment) {
        debug::error("Executable metadata creation failed: no loadable segments");
        return elf::Result::Malformed;
    }

    if (file.header->entry < imageStart || file.header->entry > imageEnd) {
        debug::error("Executable metadata creation failed: entry ", file.header->entry, " outside image range ",
                     imageStart, " through ", imageEnd);
        return elf::Result::Malformed;
    }

    outExecutable->entry = file.header->entry;
    outExecutable->userStackTop = process::UserStackTop;
    outExecutable->imageStart = imageStart;
    outExecutable->imageEnd = imageEnd;


    debug::log("Executable metadata ready: entry ", outExecutable->entry, " image start ",
               outExecutable->imageStart, " image end ", outExecutable->imageEnd, " user stack top ",
               outExecutable->userStackTop);
    return elf::Result::Ok;
}

elf::Result Executable::createAndLoadElf(const elf::File& file, memory::AddressSpace& addressSpace,
                                         Executable* outExecutable) {
    debug::log("Creating and loading ELF executable into address space ", addressSpace.pml4);

    auto result = fromElf(file, outExecutable);

    if (result != elf::Result::Ok) {
        debug::error("ELF load failed while creating metadata: result ", static_cast<u32>(result));
        return result;
    }

    debug::log("Activating process address space for ELF load: pml4 ", addressSpace.pml4);
    addressSpace.activate();

    for (usize i = 0; i < file.header->programHeaderCount; i++) {
        const elf::ProgramHeader* ph = file.getProgramHeader(i);

        if (!ph) {
            debug::error("ELF load failed: program header ", i, " out of bounds");
            return elf::Result::OutOfBounds;
        }

        if (!ph->isLoadable()) {
            debug::log("ELF load skipping non-loadable segment ", i, " type ", ph->type);
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
            debug::error("ELF load failed: segment ", i, " virtual range overflow");
            return elf::Result::Malformed;
        }

        debug::log("ELF load mapping segment ", i, " start ", segmentStart, " end ", segmentEnd, " size ",
                   segmentSize, " flags ", flags);
        auto mapResult = addressSpace.mapNewUserRange(segmentStart, segmentSize, flags);

        if (mapResult != memory::MapResult::Ok) {
            debug::error("ELF load failed: could not map segment ", i, " map result ",
                         static_cast<u32>(mapResult));
            return elf::Result::Malformed;
        }

        debug::log("ELF load copying segment ", i, " from file offset ", ph->offset, " to virtual address ",
                   ph->virtualAddress, " bytes ", ph->fileSize);
        memory::copy(reinterpret_cast<u8*>(ph->virtualAddress), file.data + ph->offset, static_cast<int>(ph->fileSize));

        debug::log("ELF load zeroing segment ", i, " at virtual address ", ph->virtualAddress + ph->fileSize,
                   " bytes ", ph->memorySize - ph->fileSize);
        memory::set(reinterpret_cast<u8*>(ph->virtualAddress + ph->fileSize), static_cast<u8>(0),
                    static_cast<int>(ph->memorySize - ph->fileSize));
    }

    constexpr usize UserStackSize = 1024 * 1024;

    debug::log("Mapping user stack from ", outExecutable->userStackTop - UserStackSize, " to ",
               outExecutable->userStackTop);
    auto stackResult = addressSpace.mapNewUserRange(outExecutable->userStackTop - UserStackSize, UserStackSize,
                                                    vmm::FlagUser | vmm::FlagWritable | vmm::FlagNX);

    if (stackResult != memory::MapResult::Ok) {
        debug::error("ELF load failed: could not map user stack result ", static_cast<u32>(stackResult));
        return elf::Result::Malformed;
    }

    debug::log("ELF executable loaded: entry ", outExecutable->entry, " stack top ",
               outExecutable->userStackTop);
    return elf::Result::Ok;
}
