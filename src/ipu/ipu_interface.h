#ifndef __IPU_INTERFACE__
#define __IPU_INTERFACE__
#ifdef __cplusplus
extern "C" {
#endif

#include "doomdef.h"


#define IPUMAXLUMPBYTES 32000
#define IPUMISCVALUESSIZE 16
#define IPUPRINTBUFSIZE 2048


typedef struct {
    int gameepisode;
    int gamemap;
    int lumpnum;
} G_LoadLevel_MiscValues_t;

typedef struct {
    gamestate_t gamestate;
} G_Ticker_MiscValues_t;





#ifdef __cplusplus
}
#endif
#endif // __IPU_INTERFACE__ //