#include "doomstat.h"
#include "r_defs.h"
#include "r_state.h"
// #include "w_wad.h"

#include "ipu_interface.h"
#include "ipu_transfer.h"
#include "print.h"

extern int setblocks; // r_main.c
extern int setdetail; // r_main.c



int gamelumpnum;

__SUPER__
void IPU_G_LoadLevel_UnpackMiscValues(G_LoadLevel_MiscValues_t* pack) {
  gameepisode = pack->gameepisode;
  gamemap = pack->gamemap;
  gamelumpnum = pack->lumpnum;
  gamemode = pack->gamemode;
  deathmatch = pack->deathmatch;
  skyflatnum = pack->skyflatnum;
  for (int i = 0; i < MAXPLAYERS; ++i) {
    playeringame[i] = pack->playeringame[i];
  }
}

__SUPER__
void IPU_G_Ticker_UnpackMiscValues(G_Ticker_MiscValues_t* pack) {
  gamestate = pack->gamestate;
  if (gamestate != GS_LEVEL)
    return;
  
  consoleplayer = displayplayer = pack->consoleplayer; // Asserted true on host
  players[consoleplayer].mo->x = pack->player.x;
  players[consoleplayer].mo->y = pack->player.y;
  players[consoleplayer].mo->z = pack->player.z;
  players[consoleplayer].mo->angle = pack->player.angle;
  players[consoleplayer].extralight = pack->player.extralight;
  players[consoleplayer].viewz = pack->player.viewz;
  players[consoleplayer].fixedcolormap = pack->player.fixedcolormap;

  am_playerpos.x = pack->player.x;
  am_playerpos.y = pack->player.y;
  am_playerpos.z = pack->player.z;
  am_playerpos.angle = pack->player.angle;
  for (int i = 0; i < IPUMAPPEDLINEUPDATES; ++i) {
    int update = pack->mappedline_updates[i];
    if (update == -1) break;
    lines[update].flags |= ML_MAPPED;
  }


}

__SUPER__
void IPU_Setup_UnpackMarkNums(const unsigned char* buf) {
  short* offsets = (short*) buf;
  const int offsetssize = 10 * sizeof(short);
  memcpy(markbuf, &offsets[10], IPUAMMARKBUFSIZE - offsetssize);
  for(int i = 0; i < 10; ++i) {
    marknums[i] = (patch_t*) &markbuf[offsets[i] - offsetssize];
  }
}


__SUPER__ 
void IPU_R_RenderPlayerView_UnpackMiscValues(R_RenderPlayerView_MiscValues_t* pack) {
  // Nothing to unpack
}

__SUPER__
void IPU_R_ExecuteSetViewSize_UnpackMiscValues(R_ExecuteSetViewSize_MiscValues_t* pack) {
  setblocks = pack->setblocks;
  setdetail = pack->setdetail;
}