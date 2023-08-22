#ifndef __IPU_TRANSFER__
#define __IPU_TRANSFER__
#ifdef __cplusplus
extern "C" {
#endif


#include "doomtype.h"
#include "r_defs.h"


void IPU_G_LoadLevel_PackMiscValues(void* buf);
void IPU_G_Ticker_PackMiscValues(void* buf);
void IPU_G_Responder_PackMiscValues(void* src_buf, void* dst_buf);
void IPU_R_RenderPlayerView_PackMiscValues(void* buf);
void IPU_LoadLumpForTransfer(int lumpnum, byte* buf);
void IPU_Setup_PackMarkNums(void* buf);
void IPU_NotifyLineMapped(line_t *line);
void IPU_CheckAlreadyMappedLines(void);



#ifdef __cplusplus
}
#endif
#endif // __IPU_TRANSFER__ //