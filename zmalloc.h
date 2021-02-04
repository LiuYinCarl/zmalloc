#pragma once

#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>


#define ZDEBUG

#ifdef ZDEBUG
#define zassert(cond) \
  assert(cond)
#define zlog(format, ...) \
  printf("file:%s, line:%d: %s\n", __FILE__, __LINE__, format, ##__VA_ARGS__)
#else
#define zassert(cond)
#define zlog(format, ...)
#endif

#define zcast(t, exp)	((t)(exp))

#define max_alloc_size  1024 - 8
#define init_page_num   10
#define alloc_page_num  10
#define page_size       1024
#define header_size     8
#define ptr_size        8

const static int32_t slot_count = 32;
const static int32_t slot_size[32] = {
  24,  56,  88,  120, 152, 184, 216, 248,
  280, 312, 344, 376, 408, 440, 472, 504,
  536, 568, 600, 632, 664, 696, 728, 760,
  792, 824, 856, 888, 920, 952, 984, 1016 };

// -------------- Znode -------------------------
typedef struct Znode {
  struct Znode* prev;
  struct Znode* next;
  void* val;
} Znode;

typedef struct Zlist {
  Znode* head;
  Znode* tail;
  int32_t len;
} Zlist;

static inline int32_t node_is_linked(Znode* node) {
  zassert(node);
  return (node->prev || node->prev);
}

static inline void node_remove_self(Znode* node) {
  zassert(node);
  if (node->prev) node->prev->next = node->next;
  if (node->next) node->next->prev = node->prev;
  node->next = NULL;
  node->prev = NULL;
}

static inline Znode* node_get_next(Znode* node) {
  zassert(node);
  if (node->next)
    return node->next;
  else
    return NULL;
}

static inline void list_init(Zlist* list) {
  list->head = NULL;
  list->tail = NULL;
  list->len = 0;
}

static inline void list_clear(Zlist* list) {
  zassert(list);
  Znode* node = list->head;
  Znode* next = NULL;
  while (node) {
    next = node->next;
    free(node);
    node = next;
  }
  list->head = NULL;
  list->tail = NULL;
  list->len = 0;
}

static inline void list_add_head(Zlist* list, Znode* node) {
  zassert(list);
  zassert(node);
  if (!list->head) { // empty list
    list->head = node;
    list->tail = node;
  } else {
    node->next = list->head;
    list->head->prev = node;
    list->head = node;
  }
}

static inline void list_add_tail(Zlist* list, Znode* node) {
  zassert(list);
  zassert(node);
  if (!list->tail) { // empty list
    list->head = node;
    list->tail = node;
  } else {
    list->tail->next = node;
    node->prev = list->tail;
    list->tail = node;
  }
}

static inline void* list_remove_head(Zlist* list) {
  zassert(list);
  zassert(list->head);  // must at least one node on list
  Znode* node = list->head;
  if (node->next) {  // more than one node on list
    list->head = node->next;
    node->next->prev = NULL;
  } else { // only one node on list
    list->head = NULL;  // todo
    list->tail = NULL;
  }
  --list->len;
  node->prev = node->next = NULL;
  return node;
}

static inline void list_remove_tail(Zlist* list) {
  zassert(list);
}

static inline void list_remove_node(Zlist* list, Znode* node) {
  zassert(list);
  zassert(node);
  node_remove_self(node);
  --list->len;
}

void* free_page_list[alloc_page_num];
size_t free_page_pos = -1;

#define allocmem() \
  mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)

#define freemem(ptr) \
  munmap(ptr, page_size)

#define blocks_per_page(page_size, block_size) \
  (page_size / block_size)  // todo: 优化除法

static inline size_t next_blocksize(size_t size) {
  const static size_t ALIGN = 32;
  const static size_t max = 1 << 14;  // max = 16K
  size_t n = ((size + ALIGN - 1) & ~(ALIGN - 1));
  return n >= max ? max : n;
}

static inline size_t get_pos_by_blocksize(size_t blockSize) {
  zassert(blockSize > 0);
  zassert(blockSize % 32 == 0);
  int n = 0;
  int bs = blockSize;
  bs = bs >> 6;  // bs = bs / 32
  while (bs > 0) {
    ++n;
    bs = bs >> 1;
  }
  return n;
}

typedef struct Zslab {
  Zlist* pagelist;
  size_t pagenum;
  Zpage* minfreepage;
  Zlist* fulllist;
  Zlist* freelist;
} Zslab;

typedef struct Zpage {
  Znode* selfnode;
  void* ppage;
  void** stack;
  size_t blocksize;
  size_t usersize;  // blocksize - header(8)
  size_t blocknum;
  size_t tailpos;
  size_t hasspace;
  Zslab* slab;
} Zpage;

typedef struct Zpool {
  Zslab slabs[slot_count];
  void* blockcaches[slot_count];
} Zpool;

Zpool pool;

static inline void* set_block_header(void* pblock, void* ppage) {
  void* h = pblock;
  *h = ppage;
  return pblock + 1;
}

static inline void* get_block_header(void* pblock) {
  return pblock - 1;
}

static inline void page_init(Zpage* page, Zslab* slab, size_t blocksize) {
  zassert(page);
  zassert(slab);
  zassert(blocksize % 32 == 0);
  page->ppage = allocmem();
  page->blocksize = blocksize;
  page->blocknum = blocks_per_page(page_size, blocksize);
  page->slab = slab;

  char* block = zcast(char*, page->ppage);
  page->stack = zcast(void**, malloc(page->blocknum * sizeof(void*)));
  for (size_t i = 0; i < page->blocknum; ++i) {
    void* p = (block + i * blocksize);
    page->stack[i] = set_block_header(p, page);
  }
  page->tailpos = page->blocknum - 1;
  page->hasspace = 1;
}

static inline void page_uninit(Zpage* page) {
  zassert(page);
  freemem(page->ppage);
  free(page->stack);
}

static inline void* page_alloc(Zpage* page) {
  zassert(page->tailpos >= 0);
  zassert(page->tailpos < page->blocknum);
  void* ptr = page->stack[page->tailpos];

  if (--page->tailpos < 0) {
    page->hasspace = false;
    Znode* node = page->slab->freelist.removehead();
    page->slab->fulllist.addtail();
    slab_find_min_freepage(page->slab);
  }
  return ptr;
}

static inline void page_free(Zpage* page, void* ptr) {
  ++page->tailpos;
  page->hasspace = true;
  zassert(page->tailpos >= 0);
  zassert(page->tailpos < page->blocknum);
  page->stack[page->tailpos] = ptr;
  // todo memset the block
}


static inline void slab_init(Zslab* slab, size_t blocksize) {
  slab->pagelist = malloc(sizeof(Zlist));
  list_init(slab->pagelist);
  for (size_t i = 0; i < init_page_num; ++i) // alloc pages
    slab_new_page(slab, blocksize);
}

static inline void slab_uninit(Zslab* slab) {
  Znode* node = slab->pagelist->head->next;
  Zpage* page = NULL;
  while (node) { // free pages
    page = zcast(Zpage*, node->val);
    page_uninit(page);
    node = node->next;
  }
  list_clear(slab->pagelist);
  free(slab->pagelist);
}

static inline void slab_new_page(Zslab* slab) {
  Zpage* page = zcast(Zpage*, malloc(sizeof(Zpage)));
  page_init(page, slab, slab->blocksize);
  Znode* node = zcast(Znode*, malloc(sizeof(Znode)));
  node->prev = NULL;
  node->next = NULL;
  node->val = page;
  list_add_head(slab->pagelist, node);
  page->selfnode = node;
}

static inline void slab_del_page(Zpage* node) {
  page_uninit(page);
}

static inline void* slab_alloc(Zslab* slab) {
  
}

static inline void slab_find_min_freepage(Zslab* slab) {
  zassert(slab);
  Zlist* freelist = slab->freelist;
  Zlist* fulllist = slab->fulllist;
  Zpage* minfreepage = NULL;
  if (freelist->len > 0) {
    minfreepage = zcast(Zpage*, freelist->head->val);
  } else {
    Znode* node = NULL;
    Znode* next = NULL;
    node = fulllist->head;
    while (node) {
      next = node_get_next(node);
      if (zcast(Zpage*, node->val)->hasspace) {
        node_remove_self(node);
        list_add_head(freelist, node);
      }
      node = next;
    }
    if (freelist->len == 0) {  // can't get freepage from fulllist
      slab_new_page(slab);
    }
    zassert(freelist->len > 0);
    slab->minfreepage = zcast(Zpage*, freelist->head->val);
  }
}

static inline void zinit() {
  for (size_t i = 0; i < slot_count; ++i) {
    slab_init(&(pool.slabs[i]), slot_size[i]);
  }
}

static inline void zuninit() {
  for (size_t i = 0; i < slot_count; ++i) {
    slab_uninit(&(pool.slabs[i]), slot_size[i]);
  }
}

static inline void* zalloc(size_t size) {
  zassert(size >= 0);
  size_t sz = next_blocksize(size);
  size_t pos = get_pos_by_blocksize(sz);

  zassert(pos > 0);
  zlog("pos:%ld", pos);
  zlog("slab*:%p", pool.slabs[pos]);
  Zslab* slab = pool.slabs[pos];
  void* ptr = slab_alloc();
  zassert(ptr);
  return ptr;
}

static inline void zfree(void* ptr) {
  zassert(ptr);
  Zpage* page = get_block_header(ptr);
  page_free(page, ptr);
}

static inline void* zrealloc(void* ptr, size_t size) {
  zassert(ptr);
  zassert(size >= 0);
  Zpage* page = zcast(Zpage*, get_block_header(ptr));
  if (size <= page->usersize) {
    return ptr;
  } else { // expand
    void* newptr = zalloc(size);
    memcpy(newptr, ptr, page->usersize);
    page_free(page, ptr);
    return newptr;
  }
}

static inline void* z_lalloc(void* ud, void* ptr, size_t osize, size_t nsize) {
  (void)ud;
  static void* p = NULL;

  if (nsize == 0) { // free
    if (ptr == NULL) return NULL;
    
    if (osize > max_alloc_size) free(ptr);
    else zfree(ptr);
    return NULL;
  }

  if (ptr == NULL) { // malloc
    if (nsize <= max_alloc_size) return zalloc(nsize);
    else return malloc(nsize);
  }

  // realloc
  if (osize <= max_alloc_size && nsize <= max_alloc_size) {
    return zrealloc(ptr, nsize);
  } else if (osize <= max_alloc_size && nsize > max_alloc_size) {
    p = malloc(nsize);
    memcpy(p, ptr, osize);
    zfree(ptr);
    return p;
  } else if (osize > max_alloc_size && nsize <= max_alloc_size) {
    p = zalloc(nsize);
    memcpy(p, ptr, nsize);
    free(ptr);
    return p;
  } else { // osize > max_alloc_size && nsize > max_alloc_zize
    return realloc(ptr, nsize);
  }
  zassert(0);
}