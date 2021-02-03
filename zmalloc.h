#pragma once

#include <assert.h>


#define ZDEBUG

#ifdef ZDEBUG
#define zassert(cond) assert(cond)
#define zlog(format, ...) \
  printf("file:%s, line:%d: %s\n", __FILE__, __LINE__, format, ##__VA_ARGS__)
#else
#define zassert(cond)
#define zlog(format, ...)
#endif

// -------------- Znode -------------------------

typedef struct Znode {
  struct Znode* prev;
  struct Znode* next;
  void* val;
} Znode;

inline int32_t is_linked(Znode* node) {
  assert(node);
  return (node->pprev || node->pprev);
}

inline void remove_self(Znode* node) {
  zassert(node);
  if (node->prev) node->prev->next = node->next;
  if (node->next) node->next->pprev = node->prev;
  node->next = NULL;
  node->prev = NULL;
}

// -------------- Zlist -------------------------

typedef struct Zlist {
  Znode* head;
  Znode* tail;
  int32_t len;
} Zlist;

static inline void list_init(Zlist* list) {
  Znode* fakenode;
  fakenode->prev = NULL;
  fakenode->next = NULL;
  fakenode->val = NULL;

  list->head = fakenode;
  list->tail = fakenode;
  list->len = 0;  // fakenode is not a node
}

static inline void list_clear(Zlist* list) {
  zassert(list);
  Znode* node = list->head->next;  // list's first node is fakenode
  Znode* next = NULL;
  while (node) {
    next = node->next;
    free(node);
    node = next;
  }
  list->len = 0;
}

static inline void list_head_insert(Zlist* list, Znode* node) {
  zassert(list);
  zassert(node);
  if (!list->head->next) { // empty list
    list->head->next = node;
    list->tail = node;
  } else {
    node->next = list->head->next;
    list->head->next->prev = node;
    list->head->next = node;
  }
}

static inline void list_tail_insert(Zlist* list, Znode* node) {
  zaseert(list);
  zassert(node);
  if (!list->tail->prev) { // empty list
    list->head->next = node;
    list->tail = node;
  } else {
    list->tail->next = node;
    node->prev = list->tail;
    list->tail = node;
  }
}

static inline void list_remove_node(Zlist* list, Znode* node) {
  zassert(list);
  zassert(node);
  remove_self(node);
  --list->len;
}

// ---------------page alloc/free --------------------------
void* free_page_list[alloc_page_num];
size_t free_page_pos = -1;

#define allocmem() /
  mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)

#define freemem(ptr) /
  munmap(ptr, page_size)

// ---------------------------------------------
#define blocks_per_page(page_size, block_size) \
  (page_size / block_size)  // todo: 优化除法

static inline size_t next_block_size(size_t size) {
  const static size_t ALIGN = 32;
  const static size_t max = 1 << 14;  // max = 16K
  size_t n = ((size + ALIGN - 1) & ~(ALIGN - 1));
  return n >= max ? max : n;
}

// --------------------------------------------------------------------
#define max_alloc_size 1024 - 8
#define init_page_num 10
#define alloc_page_num 10
#define page_size 1024
#define header_size 8
#define ptr_size 8

const static int32_t slot_count = 32;
const static int32_t slot_size[] = {
  24,  56,  88,  120, 152, 184, 216, 248,
  280, 312, 344, 376, 408, 440, 472, 504,
  536, 568, 600, 632, 664, 696, 728, 760,
  792, 824, 856, 888, 920, 952, 984, 1016 };


// -------------------------------------------------------------------
typedef struct Zpage {
  Znode* selfnode;
  void* ppage;
  void** stack;
  size_t blocksize;
  size_t blocknum;
  size_t tailpos;
  size_t hasspace;
  Zslab* slab;
} Zpage;

static inline void* set_block_header(void* pblock, void* ppage) {
  void* h = pblock;
  *h = ppage;
  return pblock + 1;
}

static inline void* get_block_header(void* pblock) {
  return pblock - 1;
}

static inline void page_init(Zpage* page, Zslab* slab, size_t block_size) {
  zassert(page);
  zassert(slab);
  zassert(block_size % 32 == 0);
  page->ppage = allocmem();
  page->blocksize = block_size;
  page->blocknum = blocks_per_page(page_size, block_size);
  page->slab = slab;

  char* block = page->ppage;
  page->stack = malloc(page->blocknum * sizeof(void*));
  for (size_t i = 0; i < page->blocknum; ++i) {
    void* p = (block + i * block_size);
    page->stack[i] = set_block_header(p, page);
  }

  page->tailpos = page->blocknum - 1;
  page->hasspace = 1;
}

static inline page_uninit(Zpage* page) {
  zassert(page);
  freemem(page->ppage);
  free(page->stack)
}

// ----------------------------------------------------------------
typedef struct Zslab {
  Zlist* pagelist;
  size_t pagenum;
} Zslab;

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
    page = node->val;
    page_uninit(page);
    node = node->next;
  }
  list_clear(slab->pagelist);
  free(slab->pagelist);
}

static inline void slab_new_page(Zslab* slab, size_t blocksize) {
  Zpage* page = malloc(sizeof(Zpage));
  page_init(&page, slab, blocksize);
  Znode* node = malloc(sizeof(Znode));
  node->prev = NULL;
  node->next = NULL;
  node->val = page;
  list_head_insert(slab->pagelist, node);
  page->selfnode = node;
}

static inline void slab_del_page(Zpage* node) {
  page_uninit(page);
}



// ----------------------------------------------------------------
typedef struct Zpool {
  Zslabs slabs[slot_count];
  void* blockcaches[slot_count];
} Zpool;

Zpool pool;


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


static inline void* zalloc() {

}

static inline void zfree(void* ptr) {

}

static inline void zrealloc(void* ptr, size_t size) {

}





// ---------------------------------------------------------------
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
  } else if (osize > max_alloc_size && nsizs <= max_alloc_size) {
    p = zalloc(nsize);
    memcpy(p, ptr, nsize);
    free(ptr);
    return p;
  } else { // osize > max_alloc_size && nsize > max_alloc_zize
    return realloc(ptr, nsize);
  }
  zassert(0);
}