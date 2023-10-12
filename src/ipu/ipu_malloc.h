#ifndef __IPU_MALLOC_H__
#define __IPU_MALLOC_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ipu_utils.h"


__SUPER__ void* IPU_static_malloc(int size, const char* name);
// There is no IPU_static_free()  :D

__SUPER__ void* IPU_level_malloc(int size, const char* name);
__SUPER__ void IPU_level_free(void);

__SUPER__ void* IPU_tmp_malloc(int size, const char* name);
__SUPER__ void IPU_tmp_free(void* ptr);

__SUPER__ void IPU_summarise_malloc(void);


#ifdef __cplusplus
}
#endif 
#endif // __IPU_MALLOC_H__