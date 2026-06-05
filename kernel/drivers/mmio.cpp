/*
* mmio.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Memory-Mapped Input & Output functions
* Copyright (c) 2026 Max Van den Eynde
*/

#include <types.h>
#include <drivers/driver.h>

#include "kernel/debug.h"
#include "kernel/memory/virtualMemory.h"

static virtAddr nextMMIOVirt = 0xFFFF900000000000ull;

mmio::Interface::Interface(physAddr physical, usize size) {
    physAddr alignedPhys = alignDown(physical, static_cast<physAddr>(PageSize));
    u64 offset = physical - alignedPhys;

    mappedSize = alignUp(size + offset, PageSize);

    mappingBase = nextMMIOVirt;
    nextMMIOVirt += mappedSize;

    u64 flags =
        vmm::FlagPresent |
        vmm::FlagWritable |
        vmm::FlagCacheDisable |
        vmm::FlagGlobal;

    bool ok = vmm::mapRange(
        vmm::getKernelPml4(),
        mappingBase,
        alignedPhys,
        mappedSize,
        flags
    );

    ASSERT(ok);

    base = mappingBase + offset;
}

mmio::Interface::~Interface() {
    if (!mappingBase || !mappedSize)
        return;

    for (u64 off = 0; off < mappedSize; off += PageSize) {
        vmm::unmapPage(vmm::getKernelPml4(), mappingBase + off);
    }
}
