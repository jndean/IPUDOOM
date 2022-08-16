#include <assert.h>
#include <stdlib.h>
#include <string.h>

// #include "r_defs.h"
// #include "r_state.h"
#include "doomdata.h"
#include "doomstat.h"
#include "m_misc.h"
#include "w_wad.h"

#include "ipu/ipu_interface.h"

void IPU_G_LoadLevel_PackMiscValues(void* buf) {
  assert(sizeof(G_LoadLevel_MiscValues_t) <= IPUMISCVALUESSIZE);
  
  G_LoadLevel_MiscValues_t pack;
  pack.gameepisode = gameepisode;
  pack.gamemap = gamemap;
  memcpy(buf, &pack, sizeof(pack));
}


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