#ifndef __IPU_MALLOC_H__
#define __IPU_MALLOC_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ipu_utils.h"


typedef enum IPUMallocPool {
    IPUMALLOC_STATIC,
    IPUMALLOC_LEVEL,
    IPUMALLOC_TMP, // This is a category I made up, for fleeting allocations

    IPUMALLOC_NUMPOOLS
} IPUMallocPool_t;


__SUPER__ void* IPU_malloc(int size, IPUMallocPool_t pool, const char* name);
__SUPER__ void IPU_free(IPUMallocPool_t pool);

__SUPER__ void IPU_summarise_malloc(void);



#ifdef __cplusplus
}
#endif 
#endif // __IPU_MALLOC_H__