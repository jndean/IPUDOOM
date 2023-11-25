
#ifndef __IPU_TEXTURETILES__
#define __IPU_TEXTURETILES__

#ifdef __cplusplus
extern "C" {
#endif


#include "doomtype.h"
#include "ipu_utils.h"


struct IPUColRequest_t {
    unsigned columnOffset, lightNum, lightScale;
};
struct IPUSpanRequest_t {
    unsigned position, step;
    short ds_x1, ds_x2;
    unsigned char y, light;
};

typedef struct {
    unsigned texture;
    union {
        struct IPUColRequest_t colRequest;
        struct IPUSpanRequest_t spanRequest;
    };
} IPUTextureRequest_t;


extern unsigned* tileLocalProgBuf;
extern unsigned* tileLocalCommsBuf;
extern unsigned* tileLocalTextureBuf;
extern const int* tileLocalTextureRange;
extern const int* tileLocalTextureOffsets;



__SUPER__ void IPU_R_InitTextureTile(unsigned* progBuf, int progBufSize, const pixel_t* colourMap);
__SUPER__ void IPU_R_InitColumnRequester(unsigned* progBuf, int progBufSize);
__SUPER__ void IPU_R_InitSansTile(unsigned* progBuf, int progBufSize);

__SUPER__ byte* IPU_R_RequestColumn(int texture, int column);
__SUPER__ void IPU_R_FulfilColumnRequest(unsigned* progBuf, unsigned char* textureBuf, unsigned* commsBuf);
__SUPER__ void IPU_R_Sans(unsigned* progBuf, unsigned* commsBuf);
__SUPER__ void IPU_R_RenderTileDone(void);


#ifdef __cplusplus
}
#endif

#endif // __IPU_TEXTURETILES__
