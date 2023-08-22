#include "doomstat.h"
#include "r_defs.h"
#include "r_state.h"
// #include "w_wad.h"

#include "ipu_interface.h"
#include "ipu_transfer.h"
#include "ipu_print.h"

int gamelumpnum;
int requestedlumpnum;

void IPU_G_LoadLevel_UnpackMiscValues(G_LoadLevel_MiscValues_t* pack) {
  gameepisode = pack->gameepisode;
  gamemap = pack->gamemap;
  gamelumpnum = pack->lumpnum;
}

void IPU_G_Ticker_UnpackMiscValues(G_Ticker_MiscValues_t* pack) {
  gamestate = pack->gamestate;
  if (gamestate != GS_LEVEL)
    return;
  am_playerpos.x = pack->player_mobj.x;
  am_playerpos.y = pack->player_mobj.y;
  am_playerpos.z = pack->player_mobj.z;
  am_playerpos.angle = pack->player_mobj.angle;
  for (int i = 0; i < IPUMAPPEDLINEUPDATES; ++i) {
    int update = pack->mappedline_updates[i];
    if (update == -1) break;
    lines[update].flags |= ML_MAPPED;
  }
}

void IPU_Setup_UnpackMarkNums(const unsigned char* buf) {
  short* offsets = (short*) buf;
  const int offsetssize = 10 * sizeof(short);
  memcpy(markbuf, &offsets[10], IPUAMMARKBUFSIZE - offsetssize);
  for(int i = 0; i < 10; ++i) {
    marknums[i] = (patch_t*) &markbuf[offsets[i] - offsetssize];
  }
}


void IPU_R_RenderPlayerView_UnpackMiscValues(R_RenderPlayerView_MiscValues_t* pack) {
  displayplayer = pack->displayplayer;
  players[displayplayer] = pack->players_displayplayer;
  // TODO:  pack->displaypayer_mo
}