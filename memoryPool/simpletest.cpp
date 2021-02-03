#include "slabs.h"

#include <time.h>
#include <stdio.h>

#define NUM 10000000
#define BLOCK_SIZE 200

// static struct timespec startTime;
// static struct timespec endTime;
// long long int s_malloc = 0;
// long long int ns_malloc = 0;
// long long int s_smem = 0;
// long long int ns_smem = 0;



// todo static 与非static
static void *
malloc_lalloc(void *ud, void *ptr, size_t osize, size_t nsize) {
	if (nsize == 0) {
		free(ptr);
		return NULL;
	} else {
		return realloc(ptr, nsize);
	}
}


int main() {
    void* ptr = nullptr;
    g_smalloc = new SMalloc;
    for (int i = 0; i < NUM; ++i) {
			// clock_gettime(CLOCK_REALTIME, &startTime);
			ptr = smem_lalloc(NULL, NULL, 0, BLOCK_SIZE);
            smem_lalloc(NULL, ptr, BLOCK_SIZE, 0);
			// clock_gettime(CLOCK_REALTIME, &endTime);
			// s_smem += endTime.tv_sec - startTime.tv_sec;
			// ns_smem += endTime.tv_nsec - startTime.tv_nsec;
    }

    for (int i = 0; i < NUM; ++i) {
			// clock_gettime(CLOCK_REALTIME, &startTime);
			ptr = malloc_lalloc(NULL, NULL, 0, BLOCK_SIZE);
            malloc_lalloc(NULL, ptr, BLOCK_SIZE, 0);
			// clock_gettime(CLOCK_REALTIME, &endTime);
			// s_malloc += endTime.tv_sec - startTime.tv_sec;
			// ns_malloc += endTime.tv_nsec - startTime.tv_nsec;   
    }

    // printf("smem total time: %lldus\n", s_smem*1000000 + ns_smem/1000);
    // printf("malloc total time: %lldus\n", s_malloc*1000000 + ns_malloc/1000);

    return 0;
}