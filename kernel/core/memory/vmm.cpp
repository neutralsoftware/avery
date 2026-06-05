/*
* vmm.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Virtual Memory Manager
* Copyright (c) 2026 Max Van den Eynde
*/

#include <kernel/memory/physicalMemory.h>
#include <kernel/memory/virtualMemory.h>

#include "kernel/debug.h"

#define PTE_PRESENT  (1ull << 0)
#define PTE_WRITABLE (1ull << 1)
#define PTE_USER     (1ull << 2)
#define PTE_NX       (1ull << 63)

#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ull

#define PML4_INDEX(v) (((v) >> 39) & 0x1FF)
#define PDPT_INDEX(v) (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)   (((v) >> 21) & 0x1FF)
#define PT_INDEX(v)   (((v) >> 12) & 0x1FF)

#define OUT_OF_MEMORY 0

static vmm::PageTable* kernelPML4;

vmm::PageTable* vmm::newTable() {
    physAddr phys = pmm::allocPage();

    ASSERT(phys != OUT_OF_MEMORY);

    auto* table = reinterpret_cast<PageTable*>(pmm::physicalToVirtual(phys));
    memory::set(reinterpret_cast<u8*>(table), static_cast<u8>(0), PAGE_SIZE);

    return table;
}

vmm::PageTable* vmm::getNextTable(PageTable* table, u64 index, bool create) {
    u64 entry = table->entries[index];

    if (entry & PTE_PRESENT) {
        physAddr phys = entry & PTE_ADDR_MASK;
        return static_cast<PageTable*>(pmm::physicalToVirtual(phys));
    }

    if (!create) {
        return nullptr;
    }

    PageTable* newlyCreated = newTable();
    physAddr newPhys = pmm::virtualToPhysicalHHDM(newlyCreated);

    table->entries[index] = newPhys | PTE_PRESENT | PTE_WRITABLE;

    return newlyCreated;
}

u64* vmm::getPte(PageTable* pml4, virtAddr virt, bool create) {
    PageTable* pdpt = getNextTable(pml4, PML4_INDEX(virt), create);

    if (!pdpt) {
        return nullptr;
    }

    PageTable* pd = getNextTable(pdpt, PDPT_INDEX(virt), create);

    if (!pd) {
        return nullptr;
    }

    PageTable* pt = getNextTable(pd, PT_INDEX(virt), create);

    if (!pt) {
        return nullptr;
    }

    return &pt->entries[PDPT_INDEX(virt)];
}

bool vmm::mapPage(PageTable* pml4, virtAddr virt, physAddr phys, u64 flags) {
    if ((virt % PAGE_SIZE) != 0 || (phys % PAGE_SIZE) != 0) {
        debug::error("Tried to map a page to a virtual or physical address that is not aligned.");
        return false;
    }

    u64* pte = getPte(pml4, virt, true);
    if (!pte) {
        return false;
    }

    if (*pte & PTE_PRESENT) {
        return false;
    }

    *pte = (phys & PTE_ADDR_MASK) | flags | PTE_PRESENT;

    return true;
}

bool vmm::unmapPage(PageTable* pml4, virtAddr virt) {
    if (virt % PAGE_SIZE != 0) {
        debug::error("Tried to unmap a page to a virtual address that is not aligned.");
        return false;
    }

    u64* pte = getPte(pml4, virt, false);
    if (!pte || !(*pte & PTE_PRESENT)) {
        return false;
    }

    *pte = 0;

    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");

    return true;
}

physAddr vmm::virtToPhysical(PageTable* pml4, virtAddr virt) {
    u64* pte = getPte(pml4, virt, false);

    if (!pte || !(*pte & PTE_PRESENT)) {
        return false;
    }

    physAddr pagePhys = *pte & PTE_ADDR_MASK;
    u64 offset = virt & 0xFFF;

    return pagePhys + offset;
}

bool vmm::mapRange(PageTable* pml4, virtAddr virt, physAddr phys, u64 size, u64 flags) {
    u64 pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (u64 i = 0; i < pages; i++) {
        bool ok = mapPage(pml4, virt + i * PAGE_SIZE, phys + i * PAGE_SIZE, flags);

        if (!ok) {
            return false;
        }
    }

    return true;
}

vmm::PageTable* vmm::createAddressSpace() {
    return newTable();
}

void vmm::switchAddressSpace(PageTable* table) {
    physAddr phys = pmm::virtualToPhysicalHHDM(table);

    asm volatile("mov %0, %%cr3" :: "r"(phys) : "memory");
}

void vmm::init() {
    physAddr cr3;

    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    kernelPML4 = static_cast<PageTable*>(pmm::physicalToVirtual(cr3 & PTE_ADDR_MASK));
}

vmm::PageTable* vmm::getKernelPml4() {
    return kernelPML4;
}
