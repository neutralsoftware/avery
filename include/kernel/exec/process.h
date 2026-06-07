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

struct Executable {
    u64 entry;
    u64 userStackTop;

    u64 imageStart;
    u64 imageEnd;

    static elf::Result fromElf(const elf::File& file, Executable* outExecutable);
};

namespace process {
    static constexpr u64 UserStackTop = 0x0000800000000000;
}

#endif //AVERY_PROCESS_H
