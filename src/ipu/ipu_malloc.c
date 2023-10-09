#include <stdlib.h>

#include "print.h"
#include "ipu_malloc.h"
#include "ipu_utils.h"

#define ALIGN32(x) (((x) + 3) & (~3))

// #define IPUMALLOC_DEBUGPRINT 0

enum {
    PU_STATIC_max_size = 10000,
    PU_LEVEL_max_size = 200000,
    PU_TMP_max_size = 512  // This is a category I made up, for fleeting allocations
};

static unsigned PU_STATIC_size = 0;
static unsigned PU_LEVEL_size = 0;
static unsigned PU_TMP_size = 0;
static unsigned char PU_STATIC_pool[PU_STATIC_max_size];
static unsigned char PU_LEVEL_pool[PU_LEVEL_max_size];
static unsigned char PU_TMP_pool[PU_TMP_max_size];


void* IPU_level_malloc(int size, const char* name) {
    void* ret = (void*)(&PU_LEVEL_pool[PU_LEVEL_size]);
    PU_LEVEL_size = ALIGN32(PU_LEVEL_size + size);

#ifdef IPUMALLOC_DEBUGPRINT
    if (tileID == IPUMALLOC_DEBUGPRINT)
        printf("LEVEL_ALLOC: %s = %dK, total = %dK\n", name, size / 1000, PU_LEVEL_size / 1000);
#endif

    if (PU_LEVEL_size > PU_LEVEL_max_size) {
        printf("### ERROR ###: PU_LEVEL_max_size is %d, but IPU_level_malloc wants %d\n",
            PU_LEVEL_max_size, PU_LEVEL_size);
        // exit(1701);
    }
    return ret;
}

void IPU_level_free() {
    PU_LEVEL_size = 0;
}

void* IPU_static_malloc(int size, const char* name) {
    void* ret = (void*)(&PU_STATIC_pool[PU_STATIC_size]);
    PU_STATIC_size = ALIGN32(PU_STATIC_size + size);

#ifdef IPUMALLOC_DEBUGPRINT
    if (tileID == IPUMALLOC_DEBUGPRINT)
        printf("STATIC_ALLOC: %s = %d,  total = %dK\n", name, size, PU_STATIC_size / 1000);
#endif

    if (PU_STATIC_size > PU_STATIC_max_size) {
        printf("### ERROR ###: PU_STATIC_max_size is %d, but IPU_static_malloc wants %d\n",
            PU_STATIC_max_size, PU_STATIC_size);
    }
    return ret;
}
 // There is no IPU_static_free :)


static unsigned PU_TMP_count = 0;
void* IPU_tmp_malloc(int size, const char* name) {
    void* ret = (void*)(&PU_TMP_pool[PU_TMP_size]);
    PU_TMP_size = ALIGN32(PU_TMP_size + size);

#ifdef IPUMALLOC_DEBUGPRINT
    if (tileID == IPUMALLOC_DEBUGPRINT)
        printf("TMP_ALLOC: %s = %dK,  total = %dK\n", name, size / 1000, PU_TMP_size / 1000);
#endif

    if (PU_TMP_size > PU_TMP_max_size) {
        printf("### ERROR ###: PU_TMP_max_size is %d, but IPU_tmp_malloc wants %d\n",
            PU_TMP_max_size, PU_TMP_size);
    }

    PU_TMP_count += 1;
    return ret;
}


void IPU_tmp_free(void* ptr) {
    (void) ptr;
    PU_TMP_count -= 1;
    if (PU_TMP_count == 0) {
        PU_TMP_size = 0;
    }
}


void IPU_summarise_malloc() {
    if (tileID != 0) 
        return;

    printf(
        "IPU Dynamic Allocation Summary:\n"
        "Static: %dK/%dK, Level: %dK/%dK, Tmp: %d/%d\n",
        PU_STATIC_size / 1000, PU_STATIC_max_size / 1000,
        PU_LEVEL_size / 1000, PU_LEVEL_size/ 1000,
        PU_TMP_size / 1000, PU_TMP_size/ 1000
    );
}
