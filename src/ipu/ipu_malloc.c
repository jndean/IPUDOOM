#include <stdlib.h>

// #include "ipu_malloc.h"
#include "ipu_print.h"


#define IPUMALLOC_MAXMAPSIZE 150000

#define ALIGN32(x) (((x) + 3) & (~3))

static unsigned char PU_LEVEL_pool[IPUMALLOC_MAXMAPSIZE];
static int PU_LEVEL_size = 0;


void* IPU_level_malloc(int size) {
    void* ret = (void*)(&PU_LEVEL_pool[PU_LEVEL_size]);
    PU_LEVEL_size = ALIGN32(PU_LEVEL_size + size);

    if (PU_LEVEL_size > IPUMALLOC_MAXMAPSIZE) {
        ipuprint("ERROR: IPUMALLOC_MAXMAPSIZE is ");
        ipuprintnum(IPUMALLOC_MAXMAPSIZE);
        ipuprint(", but IPU_level_malloc wants ");
        ipuprintnum(PU_LEVEL_size);
        ipuprint("\n");
        // exit(1701);
    }
    return ret;
}

void IPU_level_free() {
    PU_LEVEL_size = 0;
}