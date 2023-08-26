#ifndef __IPUDOOM__
#define __IPUDOOM__

#ifdef __cplusplus
extern "C" {
#endif

#include "ipu/ipu_interface.h"


void IPU_Init(void);

void IPU_AM_Drawer(void);
void IPU_R_RenderPlayerView(void);
void IPU_R_ExecuteSetViewSize(void);

void IPU_G_DoLoadLevel(void);
void IPU_G_Ticker(void);
void IPU_G_Responder(G_Responder_MiscValues_t*);


#ifdef __cplusplus
}
#endif

#endif // __IPUDOOM__ //