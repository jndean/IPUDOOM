
#ifndef __IPU_TEXTURETILES__
#define __IPU_TEXTURETILES__

#ifdef __cplusplus
extern "C" {
#endif


#include "doomtype.h"
#include "ipu_interface.h"
#include "ipu_utils.h"


#define IPUTEXREQUESTISSPAN (0x800000)
#define IPUMAXSPANREQUESTBATCHSIZE (5)


typedef struct {
    unsigned columnOffset, lightNum, lightScale;
} IPUColRequest_t;

typedef struct {
    unsigned position, step;
    unsigned char count, lightNum;
} IPUSpanRequest_t;

typedef struct {
    unsigned texture;
    union {
        IPUColRequest_t colRequest;
        IPUSpanRequest_t spanRequests[IPUMAXSPANREQUESTBATCHSIZE];
    };
    unsigned short numSpanRequests;
} IPUTextureRequest_t;

#ifdef __cplusplus
static_assert((IPUTEXTURECACHELINESIZE * sizeof(int)) >= sizeof(IPUTextureRequest_t), 
              "Texture cache buf is used to store the request at one point");
#endif

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
