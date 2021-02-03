#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <memory.h>
#include <assert.h>
#include<sys/time.h>
#include <array>

#include "klist.h"

#define MAX_ALLOC_SIZE 1016
// #define SLOT_MAX_POS 9  // 8KB
#define INIT_SLOT_NUM 10
#define PRE_ALLOC_PAGE_NUM 10  // don't set too big, otherwise mmap() maybe failed
#define PAGE_SIZE  1024  // 1KB
#define ALIGN_ADDR 0XFFFFFFFFFFFF4000
#define HEADER_ALIGN 8

const static int32_t slot_count = 32;
const static int32_t slot_size[] = {
    24,  56,  88,  120, 152, 184, 216, 248,
    280, 312, 344, 376, 408, 440, 472, 504,
    536, 568, 600, 632, 664, 696, 728, 760,
    792, 824, 856, 888, 920, 952, 984, 1016 };


class Slot;
class Slab;
class SMalloc;

typedef Slot** Header;
typedef void* PagePtr;

extern SMalloc* g_smalloc;

class Slot {
public:
    Slot(int32_t _blockSzie, Slab* _s);
    ~Slot();
    void* Alloc();
    void Free(void* ptr);

public:
    void* pagePtr;
    int32_t blockSize;
    int32_t userBlockSize;  // userBlockSize = blockSize - Header(8 Byte)
    bool hasSpace;

private:
    void** stack;
    int32_t blockCnt;  // how many items per Slot
    int32_t tailPos;
    Slab* slabBelong;
};


class Slab {
public:
    Slab(int32_t _blockSize, int32_t initSlotCnt);
    ~Slab();
    bool CreateSlot(int n=1);
    bool DestroySlot();
    void FindMinFreeSlot();
    void* AllocMem();
    void FreeMem(void* ptr);

public:
    KList fullList;
    KList freeList;
    
private:
    Slot* minFreeSlot;
    int32_t blockSize;  // size of item block
    int32_t slotCnt;  // size of slab
    int32_t blockCntperSlot;  // how many item per slot  
};


// small memory malloc
class SMalloc {
public:
    SMalloc();
    ~SMalloc();
    void* Alloc(const size_t size);
    void Free(void* ptr);
    void* Realloc(void*ptr, size_t size);

private:
    std::array<Slab*, slot_count> slabArray;
    void* blockCache[slot_count];
};

void* smem_lalloc(void* ud, void* ptr, size_t osize, size_t nsize);