/*
* addressSpace.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Implementation for Address Spaces
* Copyright (c) 2026 Max Van den Eynde
*/

#include <kernel/memory/virtualMemory.h>

#include "drivers/driver.h"
#include "kernel/memory/physicalMemory.h"

namespace memory {
    static constexpr u64 EntryCount = 512;
    static constexpr u64 AddressMask = 0x000FFFFFFFFFF000;
    static constexpr u64 OwnedPhysical = 1ull << 9;

    static constexpr u64 UserTop = 0x0000800000000000;

    static usize pml4Index(u64 virtualAddress) {
        return (virtualAddress >> 39) & 0x1FF;
    }

    static usize pdptIndex(u64 virtualAddress) {
        return (virtualAddress >> 30) & 0x1FF;
    }

    static usize pdIndex(u64 virtualAddress) {
        return (virtualAddress >> 21) & 0x1FF;
    }

    static usize ptIndex(u64 virtualAddress) {
        return (virtualAddress >> 12) & 0x1FF;
    }

    static void zeroPage(u64* page) {
        for (usize i = 0; i < EntryCount; i++) {
            page[i] = 0;
        }
    }

    static u64* allocPageTable() {
        u64 physical = pmm::allocPage();

        if (!physical) {
            return nullptr;
        }

        u64* table = reinterpret_cast<u64*>(pmm::physicalToVirtual(physical));
        zeroPage(table);

        return table;
    }

    static u64 tablePhysical(u64* table) {
        return pmm::virtualToPhysicalHHDM(table);
    }

    static bool present(u64 entry) {
        return (entry & vmm::FlagPresent) != 0;
    }

    static u64* tableFromEntry(u64 entry) {
        return static_cast<u64*>(pmm::physicalToVirtual(entry & AddressMask));
    }

    static bool isActive(u64* pml4) {
        u64 currentCR3;
        asm volatile("mov %%cr3, %0" : "=r"(currentCR3));

        return (currentCR3 & AddressMask) == tablePhysical(pml4);
    }

    static void invalidateIfActive(u64* pml4, u64 virtualAddress) {
        if (isActive(pml4)) {
            asm volatile("invlpg (%0)" :: "r"(virtualAddress) : "memory");
        }
    }

    static bool rangeBounds(u64 virtualAddress, usize size, u64* outStart, u64* outEnd) {
        if (!outStart || !outEnd || size == 0) {
            return false;
        }

        if (static_cast<u64>(size) > U64_MAX - virtualAddress) {
            return false;
        }

        u64 endAddress = virtualAddress + size;

        if (endAddress > U64_MAX - (mmio::PageSize - 1)) {
            return false;
        }

        *outStart = alignDown(virtualAddress, mmio::PageSize);
        *outEnd = alignUp(endAddress, mmio::PageSize);

        return *outEnd >= *outStart;
    }

    static bool isUserAddressRange(u64 start, u64 end) {
        return start < UserTop && end <= UserTop;
    }

    static MapResult ensureNextTable(u64* table, usize index, u64 flags, u64** outNext) {
        if (!outNext) {
            return MapResult::InvalidArgument;
        }

        if (!present(table[index])) {
            u64* next = allocPageTable();

            if (!next) {
                return MapResult::OutOfMemory;
            }

            table[index] =
                tablePhysical(next)
                | vmm::FlagPresent
                | vmm::FlagWritable;
        }

        if ((flags & vmm::FlagUser) != 0) {
            table[index] |= vmm::FlagUser;
        }

        *outNext = tableFromEntry(table[index]);
        return MapResult::Ok;
    }

    static MapResult getLeafTable(u64* pml4, u64 virtualAddress, bool create, u64 flags, u64** outPt) {
        if (!outPt) {
            return MapResult::InvalidArgument;
        }

        u64 pml4e = pml4[pml4Index(virtualAddress)];

        if (!present(pml4e)) {
            if (!create) {
                return MapResult::NotMapped;
            }

            MapResult result = ensureNextTable(pml4, pml4Index(virtualAddress), flags, outPt);
            if (result != MapResult::Ok) {
                return result;
            }

            pml4e = pml4[pml4Index(virtualAddress)];
        }
        else if ((flags & vmm::FlagUser) != 0) {
            pml4[pml4Index(virtualAddress)] |= vmm::FlagUser;
        }

        u64* pdpt = tableFromEntry(pml4e);
        u64 pdpte = pdpt[pdptIndex(virtualAddress)];

        if (!present(pdpte)) {
            if (!create) {
                return MapResult::NotMapped;
            }

            MapResult result = ensureNextTable(pdpt, pdptIndex(virtualAddress), flags, outPt);
            if (result != MapResult::Ok) {
                return result;
            }

            pdpte = pdpt[pdptIndex(virtualAddress)];
        }
        else if ((flags & vmm::FlagUser) != 0) {
            pdpt[pdptIndex(virtualAddress)] |= vmm::FlagUser;
        }

        u64* pd = tableFromEntry(pdpte);
        u64 pde = pd[pdIndex(virtualAddress)];

        if (!present(pde)) {
            if (!create) {
                return MapResult::NotMapped;
            }

            MapResult result = ensureNextTable(pd, pdIndex(virtualAddress), flags, outPt);
            if (result != MapResult::Ok) {
                return result;
            }

            pde = pd[pdIndex(virtualAddress)];
        }
        else if ((flags & vmm::FlagUser) != 0) {
            pd[pdIndex(virtualAddress)] |= vmm::FlagUser;
        }

        *outPt = tableFromEntry(pde);
        return MapResult::Ok;
    }

    static MapResult mapPageInternal(
        u64* pml4,
        u64 virtualAddress,
        u64 physicalAddress,
        u64 flags,
        bool owned
    ) {
        if (!pml4) {
            return MapResult::InvalidArgument;
        }

        if ((virtualAddress % mmio::PageSize) != 0 || (physicalAddress % mmio::PageSize) != 0) {
            return MapResult::InvalidArgument;
        }

        if ((flags & vmm::FlagUser) != 0 && virtualAddress >= UserTop) {
            return MapResult::InvalidArgument;
        }

        u64* pt;
        MapResult result = getLeafTable(pml4, virtualAddress, true, flags, &pt);

        if (result != MapResult::Ok) {
            return result;
        }

        usize index = ptIndex(virtualAddress);

        if (present(pt[index])) {
            return MapResult::AlreadyMapped;
        }

        pt[index] =
            (physicalAddress & AddressMask)
            | flags
            | vmm::FlagPresent;

        if (owned) {
            pt[index] |= OwnedPhysical;
        }

        invalidateIfActive(pml4, virtualAddress);

        return MapResult::Ok;
    }

    static void freeLeafIfOwned(u64 entry) {
        if ((entry & OwnedPhysical) != 0) {
            pmm::freePage(entry & AddressMask);
        }
    }

    static void destroyPageTable(u64* table, u32 level) {
        for (usize i = 0; i < EntryCount; i++) {
            u64 entry = table[i];

            if (!present(entry)) {
                continue;
            }

            if (level == 1) {
                freeLeafIfOwned(entry);
            }
            else {
                destroyPageTable(tableFromEntry(entry), level - 1);
            }
        }

        pmm::freePage(tablePhysical(table));
    }

    MapResult AddressSpace::create(AddressSpace* outSpace) {
        if (!outSpace) {
            return MapResult::InvalidArgument;
        }

        u64* newPML4 = allocPageTable();

        if (!newPML4) {
            return MapResult::OutOfMemory;
        }

        u64* kernelPML4 = reinterpret_cast<u64*>(vmm::getKernelPml4());

        if (!kernelPML4) {
            pmm::freePage(tablePhysical(newPML4));
            return MapResult::InvalidArgument;
        }

        for (usize i = 256; i < EntryCount; i++) {
            newPML4[i] = kernelPML4[i];
        }

        outSpace->pml4 = newPML4;
        return MapResult::Ok;
    }

    void AddressSpace::activate() const {
        if (!pml4) {
            return;
        }

        u64 physical = tablePhysical(pml4);
        asm volatile("mov %0, %%cr3" :: "r"(physical) : "memory");
    }

    MapResult AddressSpace::mapPage(
        u64 virtualAddress,
        u64 physicalAddress,
        u64 flags
    ) {
        if (!pml4) {
            return MapResult::InvalidArgument;
        }

        return mapPageInternal(pml4, virtualAddress, physicalAddress, flags, false);
    }

    MapResult AddressSpace::mapRange(
        u64 virtualAddress,
        u64 physicalAddress,
        usize size,
        u64 flags
    ) {
        u64 start;
        u64 end;

        if (!rangeBounds(virtualAddress, size, &start, &end)) {
            return MapResult::InvalidArgument;
        }

        if ((physicalAddress % mmio::PageSize) != 0) {
            return MapResult::InvalidArgument;
        }

        u64 mappedSize = end - start;

        if (physicalAddress > U64_MAX - (mappedSize - mmio::PageSize)) {
            return MapResult::InvalidArgument;
        }

        for (u64 address = start; address < end; address += mmio::PageSize) {
            MapResult result = mapPage(
                address,
                physicalAddress + (address - start),
                flags
            );

            if (result != MapResult::Ok) {
                unmapRange(start, static_cast<usize>(address - start));
                return result;
            }
        }

        return MapResult::Ok;
    }

    MapResult AddressSpace::mapNewUserRange(
        u64 virtualAddress,
        usize size,
        u64 flags
    ) {
        u64 start;
        u64 end;

        if (!rangeBounds(virtualAddress, size, &start, &end)) {
            return MapResult::InvalidArgument;
        }

        if (!isUserAddressRange(start, end)) {
            return MapResult::InvalidArgument;
        }

        for (u64 address = start; address < end; address += mmio::PageSize) {
            u64 physical = pmm::allocPage();

            if (!physical) {
                return MapResult::OutOfMemory;
            }

            zeroPage(reinterpret_cast<u64*>(pmm::physicalToVirtual(physical)));

            MapResult result = mapPageInternal(
                pml4,
                address,
                physical,
                flags | vmm::FlagUser,
                true
            );

            if (result != MapResult::Ok) {
                pmm::freePage(physical);
                unmapRange(start, static_cast<usize>(address - start));
                return result;
            }
        }

        return MapResult::Ok;
    }

    MapResult AddressSpace::mapNewUserPage(
        u64 virtualAddress,
        usize size,
        u64 flags
    ) {
        if (size > static_cast<usize>(mmio::PageSize)) {
            return MapResult::InvalidArgument;
        }

        return mapNewUserRange(virtualAddress, size, flags);
    }

    bool AddressSpace::isMapped(u64 virtualAddress) const {
        if (!pml4) {
            return false;
        }

        if ((virtualAddress % mmio::PageSize) != 0) {
            return false;
        }

        u64* pt;
        MapResult result = getLeafTable(pml4, virtualAddress, false, 0, &pt);

        if (result != MapResult::Ok) {
            return false;
        }

        u64 pte = pt[ptIndex(virtualAddress)];

        return present(pte);
    }

    MapResult AddressSpace::unmapPage(u64 virtualAddress) const {
        if (!pml4) {
            return MapResult::InvalidArgument;
        }

        if ((virtualAddress % mmio::PageSize) != 0) {
            return MapResult::InvalidArgument;
        }

        u64* pt;
        MapResult result = getLeafTable(pml4, virtualAddress, false, 0, &pt);

        if (result != MapResult::Ok) {
            return result;
        }

        usize index = ptIndex(virtualAddress);
        u64 entry = pt[index];

        if (!present(entry)) {
            return MapResult::NotMapped;
        }

        freeLeafIfOwned(entry);
        pt[index] = 0;

        invalidateIfActive(pml4, virtualAddress);

        return MapResult::Ok;
    }

    MapResult AddressSpace::unmapRange(u64 virtualAddress, usize size) {
        u64 start;
        u64 end;

        if (!rangeBounds(virtualAddress, size, &start, &end)) {
            return MapResult::InvalidArgument;
        }

        for (u64 address = start; address < end; address += mmio::PageSize) {
            MapResult result = unmapPage(address);

            if (result != MapResult::Ok) {
                return result;
            }
        }

        return MapResult::Ok;
    }

    void AddressSpace::destroy() {
        if (!pml4) {
            return;
        }

        for (usize i = 0; i < 256; i++) {
            u64 entry = pml4[i];

            if (!present(entry)) {
                continue;
            }

            destroyPageTable(tableFromEntry(entry), 3);
            pml4[i] = 0;
        }

        pmm::freePage(tablePhysical(pml4));
        pml4 = nullptr;
    }
}
