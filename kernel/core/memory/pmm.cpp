/*
* pmm.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Physical Memory Manager
* Copyright (c) 2026 Max Van den Eynde
*/

#include <limine.h>
#include <kernel/memory/physicalMemory.h>

#include "kernel/debug.h"

pmm::PhysicalMemory pmm::physicalMemory;
#define OUT_OF_MEMORY 0

void pmm::init(volatile limine_memmap_request& memmapRequest) {
    ASSERT(memmapRequest.response != nullptr);
    limine_memmap_response* memmap = memmapRequest.response;
    ASSERT(memmap->entries != nullptr);

    physicalMemory.highestAddress = 0;

    for (u64 i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];

        u64 end = entry->base + entry->length;
        if (end > physicalMemory.highestAddress) {
            physicalMemory.highestAddress = end;
        }
    }

    physicalMemory.totalPages = alignUp(physicalMemory.highestAddress, static_cast<physAddr>(PAGE_SIZE)) / PAGE_SIZE;
    physicalMemory.bitmapSize = alignUp(physicalMemory.totalPages, static_cast<u64>(8)) / 8;
    physicalMemory.bitmapSize = alignUp(physicalMemory.bitmapSize, static_cast<u64>(PAGE_SIZE));

    physicalMemory.bitmapPhysicalAddress = 0;

    for (u64 i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        u64 start = alignUp(entry->base, static_cast<physAddr>(PAGE_SIZE));
        u64 end = alignDown(entry->base + entry->length, static_cast<physAddr>(PAGE_SIZE));

        if (end <= start) {
            continue;
        }

        if (end - start >= physicalMemory.bitmapSize) {
            physicalMemory.bitmapPhysicalAddress = start;
            break;
        }
    }

    ASSERT(physicalMemory.bitmapPhysicalAddress != 0);

    physicalMemory.bitmap = reinterpret_cast<u8*>(physicalToVirtual(physicalMemory.bitmapPhysicalAddress));

    memory::set(physicalMemory.bitmap, static_cast<u8>(0xFF), static_cast<int>(physicalMemory.bitmapSize));

    physicalMemory.freePages = 0;
    physicalMemory.usedPages = physicalMemory.totalPages;

    for (u64 i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        u64 start = alignUp(entry->base, static_cast<physAddr>(PAGE_SIZE));
        u64 end = alignDown(entry->base + entry->length, static_cast<physAddr>(PAGE_SIZE));

        if (end <= start) {
            continue;
        }

        markFree(start, (end - start) / PAGE_SIZE);
    }

    markUsed(physicalMemory.bitmapPhysicalAddress, physicalMemory.bitmapSize / PAGE_SIZE);

    markUsed(0, 1);

    physicalMemory.lastAllocIndex = 0;
}

void pmm::markUsed(physAddr addr, u64 pageCount) {
    u64 startPage = physicalToPage(addr);

    for (u64 i = 0; i < pageCount; i++) {
        u64 page = startPage + i;

        if (page >= physicalMemory.totalPages) {
            break;
        }

        if (!bitmapTest(page)) {
            bitmapSet(page);
            physicalMemory.freePages--;
            physicalMemory.usedPages++;
        }
    }
}

void pmm::markFree(physAddr addr, u64 pageCount) {
    uint64_t start_page = physicalToPage(addr);

    for (uint64_t i = 0; i < pageCount; i++) {
        uint64_t page = start_page + i;

        if (page >= physicalMemory.totalPages) {
            break;
        }

        if (bitmapTest(page)) {
            bitmapClear(page);
            physicalMemory.freePages++;
            physicalMemory.usedPages--;
        }
    }
}

physAddr pmm::allocPage() {
    for (u64 i = physicalMemory.lastAllocIndex; i < physicalMemory.totalPages; i++) {
        if (!bitmapTest(i)) {
            bitmapSet(i);

            physicalMemory.usedPages++;
            physicalMemory.freePages--;
            physicalMemory.lastAllocIndex = i + 1;

            return pageToPhysical(i);
        }
    }

    for (u64 i = 0; i < physicalMemory.lastAllocIndex; i++) {
        if (!bitmapTest(i)) {
            bitmapSet(i);

            physicalMemory.usedPages++;
            physicalMemory.freePages--;
            physicalMemory.lastAllocIndex = i + 1;

            return pageToPhysical(i);
        }
    }

    return OUT_OF_MEMORY;
}

void pmm::freePage(physAddr addr) {
    ASSERT(addr % PAGE_SIZE == 0);

    u64 page = physicalToPage(addr);

    ASSERT(page <= physicalMemory.totalPages);
    ASSERT(bitmapTest(page));

    bitmapClear(page);
    physicalMemory.freePages++;
    physicalMemory.usedPages--;

    if (page < physicalMemory.lastAllocIndex) {
        physicalMemory.lastAllocIndex = page;
    }
}

physAddr pmm::allocPages(u64 count) {
    if (count == 0) {
        debug::warn("Cannot allocate 0 pages");
        return 0;
    }

    u64 runStart = 0;
    u64 runLength = 0;

    for (u64 i = 0; i < physicalMemory.totalPages; i++) {
        if (!bitmapTest(i)) {
            if (runLength == 0) {
                runStart = i;
            }

            runLength++;

            if (runLength == count) {
                for (u64 j = 0; j < count; j++) {
                    bitmapSet(runStart + j);
                }

                physicalMemory.freePages -= count;
                physicalMemory.usedPages += count;
                physicalMemory.lastAllocIndex = runStart + count;

                return pageToPhysical(runStart);
            }
        }
        else {
            runLength = 0;
        }
    }

    return OUT_OF_MEMORY;
}

void pmm::freePages(physAddr addr, u64 count) {
    ASSERT(addr % PAGE_SIZE == 0);

    for (u64 i = 0; i < count; i++) {
        freePage(addr + i * PAGE_SIZE);
    }
}

bool pmm::isFree(physAddr addr) {
    ASSERT(addr % PAGE_SIZE == 0);

    u64 page = physicalToPage(addr);

    if (page >= physicalMemory.totalPages) {
        debug::warn("Page fell out of the total pages that the memory has.");
        return false;
    }

    return !bitmapTest(page);
}

u64 pmm::getFreePages() {
    return physicalMemory.freePages;
}

u64 pmm::getTotalPages() {
    return physicalMemory.totalPages;
}

u64 pmm::getFreeMemory() {
    return physicalMemory.freePages * PAGE_SIZE;
}

u64 pmm::getUsedPages() {
    return physicalMemory.usedPages;
}
