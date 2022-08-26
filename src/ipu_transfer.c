#include <assert.h>
#include <stdlib.h>
#include <string.h>

// #include "r_defs.h"
// #include "r_state.h"
#include "doomdata.h"
#include "doomstat.h"
#include "m_misc.h"
#include "z_zone.h"
#include "w_wad.h"

#include "ipu/ipu_interface.h"

void IPU_G_LoadLevel_PackMiscValues(void* buf) {
  assert(sizeof(G_LoadLevel_MiscValues_t) <= IPUMISCVALUESSIZE);

  G_LoadLevel_MiscValues_t pack;
  pack.gameepisode = gameepisode;
  pack.gamemap = gamemap;
  
  char lumpname[9];
  lumpname[0] = 'E';
  lumpname[1] = '0' + gameepisode;
  lumpname[2] = 'M';
  lumpname[3] = '0' + gamemap;
  lumpname[4] = '\0';
  pack.lumpnum = W_GetNumForName(lumpname);

  memcpy(buf, &pack, sizeof(pack));
}


void IPU_LoadLumpForTransfer(int lumpnum, byte* buf) {
    int size = W_LumpLength(lumpnum);
    int required = size + sizeof(int); // Space for size field
    if (required > IPUMAXLUMPBYTES) {
        printf("\nNeed %d bytes to transfer lump %d to IPU, only have %d\n",
        required, lumpnum, IPUMAXLUMPBYTES);
        exit(1);
    }
    ((int*)buf)[0] = size;
    byte* data = W_CacheLumpNum(lumpnum, PU_STATIC);
    memcpy(buf + sizeof(int), data, size);
    W_ReleaseLumpNum(lumpnum);
}