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
