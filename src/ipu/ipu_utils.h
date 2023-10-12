#ifndef __IPU_UTILS__
#define __IPU_UTILS__


#ifdef __IPU__
#define __SUPER__ __attribute__((target("supervisor")))
#else
#define __SUPER__
#endif

extern int tileID;


#endif // __IPU_UTILS__