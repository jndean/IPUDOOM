
#ifndef __IPU_TEXTURETILES__
#define __IPU_TEXTURETILES__

#ifdef __cplusplus
extern "C" {
#endif


#include "doomtype.h"
#include "ipu_utils.h"



__SUPER__ void IPU_R_InitColumnRequester(void);

__SUPER__ byte* IPU_R_RequestColumn(int texture, int column);



#ifdef __cplusplus
}
#endif

#endif // __IPU_TEXTURETILES__
