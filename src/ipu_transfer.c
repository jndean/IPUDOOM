#include <stdlib.h>
#include <string.h>

// #include "r_defs.h"
// #include "r_state.h"
#include "doomdata.h"
#include "doomstat.h"
#include "m_misc.h"
#include "w_wad.h"

#include "ipu/ipu_interface.h"

/*
void IPU_P_SetupLevel(void* void_buf) {
  printf("DBG: Packing level!!\n");

  // find map name
  char lumpname[9];
  if (gamemode == commercial) {
    if (gamemap < 10)
      M_snprintf(lumpname, 9, "map0%i", gamemap);
    else
      M_snprintf(lumpname, 9, "map%i", gamemap);
  } else {
    lumpname[0] = 'E';
    lumpname[1] = '0' + gameepisode;
    lumpname[2] = 'M';
    lumpname[3] = '0' + gamemap;
    lumpname[4] = 0;
  }
  int baselump = W_GetNumForName(lumpname);
  

}*/


void IPU_LoadLumpForTransfer(int lumpnum, byte* buf) {
    /*
    int size = W_LumpLength(lumpnum);
    if (size >= IPUMAXLUMPBYTES) {
        printf("\nNeed %d bytes to transfer lump %d to IPU, only have %d\n",
        size, lumpnum, IPUMAXLUMPBYTES);
        exit(1);
    }
    byte* data = W_CacheLumpNum(lump, PU_STATIC);
    memcpy(buf, data, size);
    W_ReleaseLumpNum(lumpnum);
    */
}