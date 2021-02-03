#include "slabs.h"
#include "knode.h"

#define MEM_DEBUG

#ifdef MEM_DEBUG
#define mem_assert(cond) assert(cond)
#define mem_log(format, ...) \
  printf("file:%s, line:%d: %s\n", __FILE__, __LINE__, format, ##__VA_ARGS__)
#else
#define mem_assert(cond)
#define DEBUG(format, ...)
#endif

SMalloc *g_smalloc = nullptr;

// divide optimization
static int32_t block_num_table[2][10] = {
    {256, 128, 64, 32, 16, 8, 4, 2, 1, 0},    // page: 8K
    {512, 256, 128, 64, 32, 16, 8, 4, 2, 1},  // page: 16K
};

static inline size_t get_next_block_size(size_t size) {
  const static int ALIGN = 32;
  const static size_t max = 1 << 14;  // max = 16K

  size_t n = ((size + ALIGN - 1) & ~(ALIGN - 1));
  return n >= max ? max : n + 1;
}

static inline size_t get_pos_by_block_size(size_t blockSize) {
  mem_assert(blockSize > 0);
  mem_assert(blockSize % 32 == 0);
  int n = 0;
  int bs = blockSize;
  bs = bs >> 6;  // bs = bs / 32
  while (bs > 0) {
    ++n;
    bs = bs >> 1;
  }
  return n;
}

// argument blockSize must be a multiply of 32
static inline int32_t blocks_per_page(int32_t blockSize) {
  mem_assert(blockSize > 0);
  mem_assert(blockSize % 32 == 0);

  int n = 0;
  int bs = blockSize;
  bs = bs >> 6;  // bs = bs / 32
  while (bs > 0) {
    ++n;
    bs = bs >> 1;
  }

  int32_t res = 0;
  switch (PAGE_SIZE) {
    case 8192:  // 8K
      res = block_num_table[0][n];
      break;
    case 16384:
      res = block_num_table[1][n];
      break;
  }
  // mem_log("blockSize:%d res: %d n:%d", blockSize, res, n);
  return res;
}

static inline void freePage(void *ptr, size_t size = PAGE_SIZE) {
  munmap(ptr, size);
}

static inline void *allocPage(size_t size = PAGE_SIZE,
                              bool freePageInList = false) {
  static void *freePageList[PRE_ALLOC_PAGE_NUM];  // page create by mmap, but
                                                  // now used now, every page's
                                                  // size equal to PAGE_SIZE
  static int32_t pos = -1;
  mem_assert(pos >= -1);

  if (freePageInList) {
    while (pos >= 0) {
      freePage(freePageList[pos--]);
    }
    return nullptr;
  }

  if (pos == -1) {
    void *p = mmap(NULL, size * PRE_ALLOC_PAGE_NUM, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // memset(p, 0, size*PRE_ALLOC_PAGE_NUM);
    char *p2 = static_cast<char *>(p);

    do {
      ++pos;
      freePageList[pos] = static_cast<void *>(p2 + pos * size);
    } while (pos < PRE_ALLOC_PAGE_NUM -
                       1);  // max value of pos is PRE_ALLOC_PAGE_NUM-1
  }

  return freePageList[pos--];
}

// you should promise the memory WHICH ptr point to is bigger than ALIGN(now is
// 8B)
static inline void *set_header_slot(void *ptr, Slot *s) {
  mem_assert(ptr != nullptr);
  mem_assert(s != nullptr);
  Header h = nullptr;
  h = static_cast<Header>(ptr);
  // mem_log("ptr%p, h:%p", ptr, h);
  *h = s;
  // real use
  return static_cast<void *>(h + 1);
  // for test
  // void* p = static_cast<void*>(h+1);
  // mem_log("header:%p user:%p", ptr, p);
  // mem_assert(reinterpret_cast<uint64_t>(p) - reinterpret_cast<uint64_t>(ptr)
  // == HEADER_ALIGN); return p;
}

static inline Slot *get_header_slot(void *ptr) {
  mem_assert(ptr != nullptr);
  static Header h = nullptr;
  h = static_cast<Header>(ptr) - 1;  // get header ptr
  mem_log("header:%p user:%p", h, ptr);
  mem_assert(reinterpret_cast<uint64_t>(ptr) - reinterpret_cast<uint64_t>(h) ==
             HEADER_ALIGN);
  mem_assert(*h != nullptr);
  return *h;
}

Slot::Slot(int32_t _blockSzie, Slab *_s) {
  mem_assert(_blockSzie > 0);
  blockSize = _blockSzie;
  userBlockSize = _blockSzie - HEADER_ALIGN;
  slabBelong = _s;
  blockCnt = blocks_per_page(blockSize);
  tailPos = blockCnt - 1;
  mem_log("tailPos:%d", tailPos);
  hasSpace = true;

  pagePtr = allocPage();
  char *block = static_cast<char *>(pagePtr);

  stack = static_cast<void **>(malloc(blockCnt * sizeof(void *)));
  for (int32_t i = 0; i < blockCnt; ++i) {
    void *p = static_cast<void *>(
        ((block + i * (blockSize))));  // todo..... maybe error
    stack[i] =
        set_header_slot(p, this);  // stack save the ptr with hidden header
  }
}

Slot::~Slot() {
  freePage(pagePtr);
  free(stack);
}

void *Slot::Alloc() {        // return null if all block was used out
  mem_assert(tailPos >= 0);  // shouldn't occur
  mem_assert(tailPos < blockCnt);

  void *ptr = stack[tailPos];

  if (--tailPos < 0) {  // there is no free block in current slot
    hasSpace = false;
    KNode *node = slabBelong->freeList.RemoveHead();
    slabBelong->fullList.AddTail(node);
    slabBelong->FindMinFreeSlot();
  }
  return ptr;
}

void Slot::Free(void *ptr) {
  tailPos++;
  hasSpace = true;
  mem_assert(tailPos >= 0);
  mem_assert(tailPos < blockCnt);  // can't return block more than slotSize
  stack[tailPos] = ptr;
  // memset(ptr, 0, userBlockSize);  // set this block memory to 0
}

Slab::Slab(int32_t _blockSize, int32_t initSlotCnt) {
  mem_assert(_blockSize > 0);
  mem_assert(initSlotCnt > 0);
  blockSize = _blockSize;
  slotCnt = initSlotCnt;
  blockCntperSlot = blocks_per_page(blockSize);
  CreateSlot(initSlotCnt);
  minFreeSlot =
      static_cast<Slot *>(static_cast<HostNode *>(freeList.GetHead())->m_host);
}

Slab::~Slab() {
  HostNode *node = nullptr;
  node = static_cast<HostNode *>(freeList.GetHead());
  while (node) {
    node->Remove();
    delete static_cast<Slot *>(node->m_host);
    delete node;
    node = static_cast<HostNode *>(freeList.GetHead());
  }

  node = static_cast<HostNode *>(fullList.GetHead());
  while (node) {
    node->Remove();
    delete static_cast<Slot *>(node->m_host);
    delete node;
    node = static_cast<HostNode *>(fullList.GetHead());
  }
}

bool Slab::CreateSlot(
    int n) {  // create new Slot when init or current Slots is run out
  for (int32_t i = 0; i < n; i++) {
    mem_assert(blockSize > 0);  // for test
    Slot *s = new Slot(blockSize, this);
    HostNode *node = new HostNode;
    node->m_host = static_cast<void *>(s);
    freeList.AddHead(node);
  }
  return true;
}

bool Slab::DestroySlot() {  // destroy Slots which are not used
  return true;
}

void Slab::FindMinFreeSlot() {
  if (!freeList.IsEmpty()) {
    minFreeSlot = static_cast<Slot *>(
        static_cast<HostNode *>(freeList.GetHead())->m_host);
  } else {
    HostNode *node = nullptr;
    HostNode *next = nullptr;
    node = static_cast<HostNode *>(fullList.GetHead());
    while (node) {
      next = static_cast<HostNode *>(node->GetNext());
      if (static_cast<Slot *>(node->m_host)->hasSpace) {
        node->Remove();
        freeList.AddHead(node);
      }
      node = next;
    }
    if (freeList.IsEmpty()) {
      CreateSlot(1);
    }
    mem_assert(!freeList.IsEmpty());
    minFreeSlot = static_cast<Slot *>(
        static_cast<HostNode *>(freeList.GetHead())->m_host);
  }
}

void *Slab::AllocMem() {  // allocate a block memory, if there is no free
                          // memory, call CreateSlot
  return minFreeSlot->Alloc();
}

void Slab::FreeMem(void *ptr) {  // free a block memory
}

SMalloc::SMalloc() {
  for (int32_t i = 0; i < slot_count; ++i) {
    mem_log("slot_size: %d", slot_size[i]);
    Slab *p = new Slab(slot_size[i] + 8, INIT_SLOT_NUM);
    slabArray[i] = p;
  }

  memset(blockCache, 0, sizeof(blockCache));
  g_smalloc = this;
}

SMalloc::~SMalloc() {
  for (auto slab : slabArray) delete slab;
  g_smalloc = nullptr;
  allocPage(0, true);  // free page in pre Alloc List
}

void *SMalloc::Alloc(const size_t size) {
  mem_assert(size >= 0);
  size_t sz = get_next_block_size(size);
  size_t pos = get_pos_by_block_size(sz);

  mem_assert(pos >= 0);
  mem_log("pos:%ld", pos);
  mem_log("slab*: %p", slabArray[0]);
  Slab *slab = slabArray[pos];
  void *ptr = slab->AllocMem();
  mem_assert(ptr != nullptr);
  return ptr;
}

void SMalloc::Free(void *ptr) {
  mem_assert(ptr != nullptr);
  Slot *s = get_header_slot(ptr);
  s->Free(ptr);
  return;
}

void *SMalloc::Realloc(void *ptr, size_t size) {
  mem_assert(ptr != nullptr);
  mem_assert(size >= 0);
  Slot *s = get_header_slot(ptr);
  if (size <= static_cast<size_t>(s->userBlockSize)) {  // reduce
    return ptr;
  } else {  // expand
    void *newPtr = Alloc(size);
    memcpy(newPtr, ptr, s->userBlockSize);
    s->Free(ptr);
    return newPtr;
  }
}

void *smem_lalloc(void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud;
  const static int32_t maxAllocMem = MAX_ALLOC_SIZE;
  static void *p = nullptr;

  if (nsize == 0) {  // free
    if (ptr == nullptr) return nullptr;

    if (osize > maxAllocMem)
      free(ptr);
    else
      g_smalloc->Free(ptr);
    return nullptr;
  }

  if (ptr == nullptr) {  // malloc
    if (nsize <= maxAllocMem)
      return g_smalloc->Alloc(nsize);
    else
      return malloc(nsize);
  }

  // realloc
  if (osize <= maxAllocMem && nsize <= maxAllocMem) {
    return g_smalloc->Realloc(ptr, nsize);
  } else if (osize <= maxAllocMem && nsize > maxAllocMem) {
    p = malloc(nsize);
    memcpy(p, ptr, osize);
    g_smalloc->Free(ptr);
    return p;
  } else if (osize > maxAllocMem && nsize <= maxAllocMem) {
    p = g_smalloc->Alloc(nsize);
    memcpy(p, ptr, nsize);
    free(ptr);
    return p;
  } else {  // osize > maxMgrMem && nsize > maxMgrMem
    return realloc(ptr, nsize);
  }

  mem_assert(0);
}