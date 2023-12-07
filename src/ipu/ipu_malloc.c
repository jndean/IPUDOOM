#include <stdlib.h>

#include "print.h"
#include "ipu_malloc.h"
#include "ipu_utils.h"

#define ALIGN32(x) (((x) + 3) & (~3))

// #define IPUMALLOC_DEBUGPRINT 0

__attribute__ ((aligned (4)))
static unsigned char IPUMALLOC_STATIC_pool[8000];
__attribute__ ((aligned (4)))
static unsigned char IPUMALLOC_LEVEL_pool[250000];
__attribute__ ((aligned (4)))
static unsigned char IPUMALLOC_TMP_pool[512]; // This is a category I made up, for fleeting allocations

static unsigned char* pools[IPUMALLOC_NUMPOOLS] = {
    IPUMALLOC_STATIC_pool,
    IPUMALLOC_LEVEL_pool,
    IPUMALLOC_TMP_pool,
};
static int poolSizes[IPUMALLOC_NUMPOOLS] = {
    sizeof(IPUMALLOC_STATIC_pool),
    sizeof(IPUMALLOC_LEVEL_pool),
    sizeof(IPUMALLOC_TMP_pool)
};
static int poolPositions[IPUMALLOC_NUMPOOLS] = {0};

static char* poolNames[IPUMALLOC_NUMPOOLS] = {
    "STATIC pool",
    "LEVEL pool",
    "TMP pool"
};

static unsigned tmpStackHeight = 0;

__SUPER__
void* IPU_malloc(int size, IPUMallocPool_t pool, const char* name) {
    char* errStr;

    if (name == NULL) {
        name = "Unnamed";
    }

    if (pool >= IPUMALLOC_NUMPOOLS || pool < 0) {
        errStr = "Bad Pool Index";
        goto ERROR;
    }

    void* ret = (void*) &pools[pool][poolPositions[pool]];
    poolPositions[pool] = ALIGN32(poolPositions[pool] + size);

#ifdef IPUMALLOC_DEBUGPRINT
    if (tileID == IPUMALLOC_DEBUGPRINT)
        printf("IPUMALLOC: %s = %dK (%s @ %dK)\n", 
            name, size / 1000, poolNames[pool], poolPositions[pool] / 1000);
#endif

    if (poolPositions[pool] > poolSizes[pool]) {
        errStr = "No Space Left";
        goto ERROR;
    }

    if (pool == IPUMALLOC_TMP) {
        tmpStackHeight += 1;
    }

    return ret;

ERROR:
    printf("\n# # # # IPUMALLOC FAILED # # # \nReason: %s\n Request: size=%d, pool=%d, name=%s\n",
        errStr, size, pool, name);
    asm volatile(
        "setzi $m0, 0xa110c\n"
        "setzi $m1, 0xa110c\n"
        "setzi $m2, 0xa110c\n"
        "setzi $m3, 0xa110c\n"
        "setzi $m4, 0xa110c\n"
        "trap 0\n"
    );
    __builtin_unreachable();
}


__SUPER__
void IPU_free(IPUMallocPool_t pool) {
    switch (pool) {
        case IPUMALLOC_LEVEL:
            poolPositions[IPUMALLOC_LEVEL] = 0;
        break;
        case IPUMALLOC_TMP:
            tmpStackHeight -= 1;
            if (tmpStackHeight == 0) {
                poolPositions[IPUMALLOC_TMP] = 0;
            }
        break;
        default:
            printf("IPUMALLOC ERROR: tried to free pool %d\n", pool);
    }
}


__SUPER__
void IPU_summarise_malloc() {
    if (tileID != 0) 
        return;

    printf(
        "IPU Dynamic Allocation Summary:\n"
        "Static: %dK/%dK, Level: %dK/%dK, Tmp: %d/%d\n",
        poolPositions[IPUMALLOC_STATIC] / 1000, poolSizes[IPUMALLOC_STATIC] / 1000,
        poolPositions[IPUMALLOC_LEVEL] / 1000, poolSizes[IPUMALLOC_LEVEL] / 1000,
        poolPositions[IPUMALLOC_TMP], poolSizes[IPUMALLOC_TMP]
    );
}


