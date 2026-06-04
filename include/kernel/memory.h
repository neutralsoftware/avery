/*
* memory.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Memory related functions
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_MEMORY_H
#define AVERY_MEMORY_H
#include "../types.h"

struct limine_hhdm_request;

namespace memory {
    void copy(u8* dest, const u8* src, int count);

    template <typename T>
    void set(T* dest, T val, int count) {
        for (int i = 0; i < count; i++) {
            dest[i] = val;
        }
    }

    void setHHDM(volatile struct limine_hhdm_request& request);
    u64 getHHDMOffset();
    void initMemoryServices(volatile struct limine_memmap_request& request);

    namespace heap {
        void init();

        void* allocate(u64 size);
        void free(void* ptr);
        void* realloc(void* ptr, u64 newSize);

        u64 getUsedMemory();
        u64 getFreeMemory();

        struct HeapBlock {
            u64 magic;
            u64 size;
            bool free;
            HeapBlock* next;
            HeapBlock* prev;
        };

        void splitBlock(HeapBlock* block, u64 size);
        void mergeWithNext(HeapBlock* block);
        HeapBlock* findFreeBlock(u64 size);
        bool expandHeap(u64 size);
    }

    template <typename T>
    T* allocElem(u64 count) {
        return heap::allocate(sizeof(T) * count);
    }

    inline void* alloc(u64 size) {
        return heap::allocate(size);
    }

    inline void free(void* ptr) {
        heap::free(ptr);
    }

    inline void* calloc(u64 count, u64 size) {
        return heap::allocate(count * size);
    }

    inline void* realloc(void* ptr, u64 newSize) {
        return heap::realloc(ptr, newSize);
    }
}

void* operator new(usize size);
void* operator new[](usize size);

void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;

void operator delete(void* ptr, usize size) noexcept;
void operator delete[](void* ptr, usize size) noexcept;


#endif //AVERY_MEMORY_H
