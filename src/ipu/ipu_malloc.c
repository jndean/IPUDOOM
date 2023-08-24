#include <stdlib.h>

#include "ipu_malloc.h"
#include "print.h"

#define IPUMALLOC_MAXMAPSIZE 250000

#define ALIGN32(x) (((x) + 3) & (~3))

static unsigned char PU_LEVEL_pool[IPUMALLOC_MAXMAPSIZE];
static int PU_LEVEL_size = 0;


void* IPU_level_malloc(int size, const char* name) {
    void* ret = (void*)(&PU_LEVEL_pool[PU_LEVEL_size]);
    PU_LEVEL_size = ALIGN32(PU_LEVEL_size + size);

    if (0) { // Enable for debug
        printf("LEVEL_ALLOC: %s = %dK,  total = %dK\n",
                name, size / 1000, PU_LEVEL_size / 1000);
    }

    if (PU_LEVEL_size > IPUMALLOC_MAXMAPSIZE) {
        printf("ERROR: IPUMALLOC_MAXMAPSIZE is %d, but IPU_level_malloc wants %d\n",
            IPUMALLOC_MAXMAPSIZE, PU_LEVEL_size);
        // exit(1701);
    }
    return ret;
}

void IPU_level_free() {
    PU_LEVEL_size = 0;
}