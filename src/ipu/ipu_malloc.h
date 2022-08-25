#ifndef __IPU_MALLOC_H__
#define __IPU_MALLOC_H__
#ifdef __cplusplus
extern "C" {
#endif

void* IPU_level_malloc(int size);
void IPU_level_free(void);


#ifdef __cplusplus
}
#endif 
#endif // __IPU_MALLOC_H__