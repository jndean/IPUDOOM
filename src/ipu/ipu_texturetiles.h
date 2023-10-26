
#ifndef __IPU_TEXTURETILES__
#define __IPU_TEXTURETILES__

#ifdef __cplusplus
extern "C" {
#endif


#include "doomtype.h"
#include "ipu_utils.h"


typedef struct {
    unsigned texture, columnOffset;
} IPUColRequest_t;


extern unsigned* tileLocalProgBuf;
extern unsigned* tileLocalCommsBuf;
extern unsigned* tileLocalTextureBuf;
extern const int* tileLocalTextureRange;
extern const int* tileLocalTextureOffsets;



__SUPER__ void IPU_R_InitTextureTile(unsigned* progBuf, int progBufSize);
__SUPER__ void IPU_R_InitColumnRequester(unsigned* progBuf, int progBufSize);

__SUPER__ byte* IPU_R_RequestColumn(int texture, int column);
__SUPER__ void IPU_R_FulfilColumnRequest(unsigned* progBuf, unsigned char* textureBuf, unsigned* commsBuf);
__SUPER__ void IPU_R_Sans(unsigned* progBuf, unsigned* commsBuf);
__SUPER__ void IPU_R_RenderTileDone(void);


#ifdef __cplusplus
}
#endif

#endif // __IPU_TEXTURETILES__
