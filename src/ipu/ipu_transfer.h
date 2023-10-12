#ifndef __IPU_TRANSFER_D__
#define __IPU_TRANSFER_D__
#ifdef __cplusplus
extern "C" {
#endif

#include "v_patch.h"
#include "ipu_utils.h"
#include "ipu_interface.h"


extern int gamelumpnum;
extern IPUTransfer_playerstate_t am_playerpos;
extern patch_t* marknums[10];
extern unsigned char markbuf[IPUAMMARKBUFSIZE];

__SUPER__ void IPU_G_LoadLevel_UnpackMiscValues(G_LoadLevel_MiscValues_t* pack);
__SUPER__ void IPU_G_Ticker_UnpackMiscValues(G_Ticker_MiscValues_t* pack);
__SUPER__ void IPU_R_RenderPlayerView_UnpackMiscValues(R_RenderPlayerView_MiscValues_t* pack);
__SUPER__ void IPU_R_ExecuteSetViewSize_UnpackMiscValues(R_ExecuteSetViewSize_MiscValues_t* pack);

#ifdef __cplusplus
}
#endif
#endif // __IPU_TRANSFER_D__ //