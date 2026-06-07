/*
* systems.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: ${FILE_DESCRIPTION}
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_SYSTEMS_H
#define AVERY_SYSTEMS_H

struct limine_memmap_request;

extern "C" void syscall_int80_entry();

namespace core {
    void initSystems(volatile struct limine_memmap_request& request);

    [[noreturn]] void enterUserMode(u64 entry, u64 userStackTop);
}

#endif //AVERY_SYSTEMS_H
