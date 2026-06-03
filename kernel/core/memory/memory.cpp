/*
* memory.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Memory functions implementation
* Copyright (c) 2026 Max Van den Eynde
*/

#include <limine.h>
#include <kernel/memory.h>

#include "io/serial.h"
#include "kernel/debug.h"
#include "kernel/memory/physicalMemory.h"

u64 HHDMOffset = 0;

void memory::copy(u8* dest, const u8* src, int count) {
    for (int i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}

void memory::setHHDM(volatile limine_hhdm_request& request) {
    ASSERT(request.response != nullptr);
    ASSERT(request.response->offset != 0);
    HHDMOffset = request.response->offset;
}

u64 memory::getHHDMOffset() {
    EXPECT(HHDMOffset != 0);
    return HHDMOffset;
}

void memory::initMemoryServices(volatile struct limine_memmap_request& request) {
    ASSERT(request.response != nullptr);
    pmm::init(request);
    debug::log("Initialized Physical Memory");
    io::serialWrite("Available pages: ");
    io::serialWriteNumber(pmm::physicalMemory.totalPages);
    io::serialWrite("\n");
}
