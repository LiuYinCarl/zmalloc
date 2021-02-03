#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "testcase.h"
// #include "lualloc.h"
#include "slabs.h"

extern int STACK, N;
extern struct test_item ITEM[];

// static void
// fill(char *p, int sz) {
// 	char dummy[8];
// 	uintptr_t x = (uintptr_t)p;
// 	int i;
// 	for (i=0;i<8;i++) {
// 		dummy[i] = x & 0xff;
// 		x >>= 8;
// 	}
// 	for (i=0;i<sz;i++) {
// 		p[i] = (dummy[i % sizeof(uintptr_t)] ^ sz) & 0xff;
// 	}
// }


// static int
// check(char *p, int sz) {
// 	char dummy[8];
// 	uintptr_t x = (uintptr_t)p;
// 	int i;
// 	for (i=0;i<8;i++) {
// 		dummy[i] = x & 0xff;
// 		x >>= 8;
// 	}
// 	for (i=0;i<sz;i++) {
// 		char c = (dummy[i % sizeof(uintptr_t)] ^ sz) & 0xff;
// 		if (c != p[i])
// 			return 1;
// 	}
// 	return 0;
// }


static struct timespec startTime;
static struct timespec endTime;
long long int s_malloc = 0;
long long int us_malloc = 0;
long long int s_free = 0;
long long int us_free = 0;
long long int s_realloc = 0;
long long int us_realloc = 0;


static void
test_nocheck(void *ud, void * (*lalloc)(void *, void *, size_t, size_t)) {
	int i;
	void ** ptr = (void **)malloc(STACK * sizeof(void *));
	for (i=0;i<N;i++) {
		struct test_item * item = &ITEM[i];
		int index = item->index - 1;
		switch(item->action) {
		case TEST_ALLOC:
			clock_gettime(CLOCK_REALTIME, &startTime);
			ptr[index] = lalloc(ud, NULL, 0, item->size);
			clock_gettime(CLOCK_REALTIME, &endTime);
			s_malloc += endTime.tv_sec - startTime.tv_sec;
			us_malloc += endTime.tv_nsec - startTime.tv_nsec;

			break;
		case TEST_FREE: {
            void* tmpp = ptr[index];
            clock_gettime(CLOCK_REALTIME, &startTime);
            lalloc(ud, tmpp, item->size, 0);
			clock_gettime(CLOCK_REALTIME, &endTime);
			s_free += endTime.tv_sec - startTime.tv_sec;
			us_free += endTime.tv_nsec - startTime.tv_nsec;
			break;
        }
		case TEST_REALLOC: {
			void * oldptr = ptr[index];
            clock_gettime(CLOCK_REALTIME, &startTime);
			ptr[index] = lalloc(ud, oldptr, item->osize, item->size);
			clock_gettime(CLOCK_REALTIME, &endTime);
			s_realloc += endTime.tv_sec - startTime.tv_sec;
			us_realloc += endTime.tv_nsec - startTime.tv_nsec;
			break;
		}
		}
	}
	free(ptr);
}

// ------------------- normal malloc -------------------------------------------

static void *
malloc_lalloc(void *ud, void *ptr, size_t osize, size_t nsize) {
	if (nsize == 0) {
		free(ptr);
		return NULL;
	} else {
		return realloc(ptr, nsize);
	}
}


int
main(int argc, char *argv[]) {   
    void* notuse = nullptr;
    if (argc == 1 || strcmp(argv[1],"smem") == 0) {
		printf("use smem.\n");
		g_smalloc = new SMalloc;
        test_nocheck(notuse, smem_lalloc);
		delete(g_smalloc);
    } else if (strcmp(argv[1], "malloc") == 0) {
		printf("use malloc.\n");
        test_nocheck(notuse, malloc_lalloc);
    } else {
        printf("error cmd\n");
    }

	long long int t1 = 0, t2 = 0, t3 = 0;
	t1 = s_malloc*1000000 + us_malloc/1000;
	t2 = s_free*1000000 + us_free/1000;
	t3 = s_realloc*1000000 + us_realloc/1000;

	printf("malloc use time:%lldus\n", t1);
	printf("free use time:%lldus\n", t2);
	printf("realloc use time:%lldus\n", t3);
	printf("total time: %lldus\n", t1 + t2 + t3);
	return 0;
}
