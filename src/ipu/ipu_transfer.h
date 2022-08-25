#ifndef __IPU_TRANSFER_D__
#define __IPU_TRANSFER_D__
#ifdef __cplusplus
extern "C" {
#endif

#include "ipu_interface.h"


extern int gamelumpnum;
extern int requestedlumpnum;
void IPU_G_LoadLevel_UnpackMiscValues(G_LoadLevel_MiscValues_t* pack);



#ifdef __cplusplus
}
#endif
#endif // __IPU_TRANSFER_D__ //