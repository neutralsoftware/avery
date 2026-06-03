/*
* physicalMemory.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: ${FILE_DESCRIPTION}
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_PHYSICALMEMORY_H
#define AVERY_PHYSICALMEMORY_H

#define PAGE_SIZE 4096
#include "../../types.h"
#include "kernel/memory.h"

struct limine_memmap_request;

namespace pmm {
    struct MemoryRegion {
        physAddr base;
        u64 length;
        u64 type;
    };

    struct MemoryInfo {
        u64 totalMemory;
        u64 usableMemory;
        u64 reservedMemory;
        u64 pageCount;
    };

    struct PhysicalMemory {
        u8* bitmap;
        physAddr bitmapPhysicalAddress;
        u64 bitmapSize;

        u64 totalPages;
        u64 usedPages;
        u64 freePages;

        physAddr highestAddress;
        u64 lastAllocIndex;
    };

    extern PhysicalMemory physicalMemory;

    void init(volatile limine_memmap_request& memmap);
    void markUsed(physAddr addr, u64 pageCount);
    void markFree(physAddr addr, u64 pageCount);

    physAddr allocPage();
    physAddr allocPages(u64 count);

    void freePage(physAddr addr);
    void freePages(physAddr addr, u64 count);

    bool isFree(physAddr addr);
    u64 getFreePages();
    u64 getTotalPages();
    u64 getUsedPages();
    u64 getFreeMemory();

    inline void* physicalToVirtual(physAddr p) {
        return (void*)(p + memory::getHHDMOffset());
    }

    inline physAddr pageToPhysical(u64 page) {
        return page * PAGE_SIZE;
    }

    inline u64 physicalToPage(physAddr addr) {
        return addr / PAGE_SIZE;
    }

    inline void bitmapSet(u64 bit) {
        physicalMemory.bitmap[bit / 8] |= (1 << (bit % 8));
    }

    inline void bitmapClear(u64 bit) {
        physicalMemory.bitmap[bit / 8] &= ~(1 << (bit % 8));
    }

    inline bool bitmapTest(u64 bit) {
        return physicalMemory.bitmap[bit / 8] & (1 << (bit % 8));
    }
}

#endif //AVERY_PHYSICALMEMORY_H
