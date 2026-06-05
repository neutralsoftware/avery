/*
* virtualMemory.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Virtual Memory Manager definitions
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_VIRTUALMEMORY_H
#define AVERY_VIRTUALMEMORY_H
#include "../../types.h"

#define PAGE_ENTRIES 512


namespace vmm {
    struct PageTable {
        u64 entries[PAGE_ENTRIES];
    };

    constexpr u64 AddressMask = 0x000FFFFFFFFFF000ull;

    constexpr u64 FlagPresent = 1ull << 0;
    constexpr u64 FlagWritable = 1ull << 1;
    constexpr u64 FlagUser = 1ull << 2;
    constexpr u64 FlagWriteThrough = 1ull << 3;
    constexpr u64 FlagCacheDisable = 1ull << 4;
    constexpr u64 FlagGlobal = 1ull << 8;
    constexpr u64 FlagNX = 1ull << 63;

    void init();

    PageTable* createAddressSpace();
    void switchAddressSpace(PageTable* table);

    bool mapPage(PageTable* pml4, virtAddr virt, physAddr phys, u64 flags);
    bool unmapPage(PageTable* pml4, virtAddr virt);
    bool mapRange(PageTable* pml4, virtAddr virt, physAddr phys, u64 size, u64 flags);

    physAddr virtToPhysical(PageTable* pml4, virtAddr virt);

    PageTable* newTable();
    PageTable* getNextTable(PageTable* table, u64 index, bool create);
    u64* getPte(PageTable* pml4, virtAddr virt, bool create);

    PageTable* getKernelPml4();
}

#endif //AVERY_VIRTUALMEMORY_H
