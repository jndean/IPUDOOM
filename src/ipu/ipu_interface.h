#ifndef __IPU_INTERFACE__
#define __IPU_INTERFACE__
#ifdef __cplusplus
extern "C" {
#endif


#define IPUMAXLUMPBYTES 32000
#define IPUMISCVALUESSIZE 128
#define IPUPRINTBUFSIZE 2048


typedef struct {
    int gameepisode;
    int gamemap;
} G_LoadLevel_MiscValues_t;



#ifdef __cplusplus
}
#endif
#endif // __IPU_INTERFACE__ //