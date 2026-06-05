/*
* heap.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Heap allocator for the kernel
* Copyright (c) 2026 Max Van den Eynde
*/

#define KERNEL_HEAP_START 0xFFFFA00000000000ull
#define KERNEL_HEAP_INITIAL_SIZE (16 * 4096)
#define KERNEL_HEAP_MAX_SIZE (64 * 1024 * 1024)

#define HEAP_MAGIC 0xC0FFEE1234567890ull

#include "kernel/debug.h"
#include "kernel/memory.h"
#include "kernel/memory/physicalMemory.h"
#include "kernel/memory/virtualMemory.h"

memory::heap::HeapBlock* head = nullptr;
virtAddr heapStart = KERNEL_HEAP_START;
virtAddr heapEnd = KERNEL_HEAP_START;
virtAddr heapMax = KERNEL_HEAP_START + KERNEL_HEAP_MAX_SIZE;

u64 usedMemory = 0;
u64 freeMemory = 0;

void memory::heap::splitBlock(HeapBlock* block, u64 size) {
    u64 remaining = block->size - size;

    if (remaining <= sizeof(HeapBlock) + 16) {
        return;
    }

    auto* newBlock = reinterpret_cast<HeapBlock*>(reinterpret_cast<u8*>(block + 1) + size);

    newBlock->magic = HEAP_MAGIC;
    newBlock->size = remaining - sizeof(HeapBlock);
    newBlock->free = true;
    newBlock->next = block->next;
    newBlock->prev = block;

    if (newBlock->next) {
        newBlock->next->prev = newBlock;
    }

    block->next = newBlock;
    block->size = size;
}

void memory::heap::mergeWithNext(HeapBlock* block) {
    HeapBlock* next = block->next;

    if (!next || !next->free) {
        return;
    }

    block->size += sizeof(HeapBlock) + next->size;
    block->next = next->next;

    if (block->next) {
        block->next->prev = block;
    }
}

memory::heap::HeapBlock* memory::heap::findFreeBlock(u64 size) {
    HeapBlock* current = head;

    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }

        current = current->next;
    }

    return nullptr;
}

bool memory::heap::expandHeap(u64 size) {
    size = alignUp(size, 4096ull);

    if (heapEnd + size > heapMax) {
        return false;
    }

    for (u64 offset = 0; offset < size; offset += 4096) {
        physAddr phys = pmm::allocPage();

        if (phys == 0) {
            return false;
        }

        bool ok = vmm::mapPage(vmm::getKernelPml4(), heapEnd + offset, phys, vmm::FlagWritable);

        if (!ok) {
            pmm::freePage(phys);
            return false;
        }
    }

    auto* newBlock = reinterpret_cast<HeapBlock*>(heapEnd);
    newBlock->magic = HEAP_MAGIC;
    newBlock->size = size - sizeof(HeapBlock);
    newBlock->next = nullptr;
    newBlock->prev = nullptr;
    newBlock->free = true;

    if (!head) {
        head = newBlock;
    }
    else {
        HeapBlock* last = head;

        while (last->next) {
            last = last->next;
        }

        last->next = newBlock;
        newBlock->prev = last;

        if (last->free) {
            mergeWithNext(last);
        }
    }

    heapEnd += size;
    freeMemory += size - sizeof(HeapBlock);

    return true;
}

void memory::heap::init() {
    head = nullptr;
    heapStart = KERNEL_HEAP_START;
    heapEnd = KERNEL_HEAP_START;
    heapMax = KERNEL_HEAP_START + KERNEL_HEAP_MAX_SIZE;

    usedMemory = 0;
    freeMemory = 0;

    bool ok = expandHeap(KERNEL_HEAP_INITIAL_SIZE);

    ASSERT(ok);
}

void* memory::heap::allocate(u64 size) {
    if (size == 0) {
        return nullptr;
    }

    size = alignUp(size, 16ull);

    HeapBlock* block = findFreeBlock(size);

    if (!block) {
        u64 expandSize = size + sizeof(HeapBlock);

        if (expandSize < 4096) {
            expandSize = 4096;
        }

        bool ok = expandHeap(expandSize);

        if (!ok) {
            return nullptr;
        }

        block = findFreeBlock(size);

        if (!block) {
            return nullptr;
        }
    }

    splitBlock(block, size);

    block->free = false;

    usedMemory += block->size;
    freeMemory -= block->size;

    return block + 1;
}

void memory::heap::free(void* ptr) {
    if (!ptr) {
        debug::error("Tried to free a pointer that was null");
        return;
    }

    HeapBlock* block = static_cast<HeapBlock*>(ptr) - 1;

    ASSERT(block->magic == HEAP_MAGIC);
    ASSERT(!block->free);

    block->free = true;

    usedMemory -= block->size;
    freeMemory += block->size;

    if (block->next && block->next->free) {
        mergeWithNext(block);
    }

    if (block->prev && block->prev->free) {
        mergeWithNext(block->prev);
    }
}

void* memory::heap::realloc(void* ptr, u64 newSize) {
    if (!ptr) {
        debug::warn("Tried to reallocate a null pointer");
        return allocate(newSize);
    }

    ASSERT(newSize > 0);

    HeapBlock* block = static_cast<HeapBlock*>(ptr) - 1;

    ASSERT(block->magic == HEAP_MAGIC);

    if (block->size >= newSize) {
        return ptr;
    }

    void* newPtr = allocate(newSize);

    if (!newPtr) {
        return nullptr;
    }

    copy(static_cast<u8*>(newPtr), static_cast<u8*>(ptr), static_cast<int>(block->size));
    free(ptr);

    return newPtr;
}

void* operator new(usize size) {
    return memory::alloc(size);
}

void* operator new[](usize size) {
    return memory::alloc(size);
}

void operator delete(void* ptr) noexcept {
    memory::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    memory::free(ptr);
}

void operator delete(void* ptr, usize) noexcept {
    memory::free(ptr);
}

void operator delete[](void* ptr, usize) noexcept {
    memory::free(ptr);
}


u64 memory::heap::getFreeMemory() {
    return freeMemory;
}

u64 memory::heap::getUsedMemory() {
    return usedMemory;
}

