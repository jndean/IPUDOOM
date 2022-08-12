#ifndef __IPUDOOM__
#define __IPUDOOM__

#ifdef __cplusplus
extern "C" {
#endif


void IPU_Init(void);

void IPU_AM_LevelInit(void);
void IPU_AM_Drawer(void);

void IPU_G_DoLoadLevel(void);


#ifdef __cplusplus
}
#endif

#endif // __IPUDOOM__ //