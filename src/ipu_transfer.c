#include <assert.h>
#include <stdlib.h>
#include <string.h>

// #include "r_defs.h"
// #include "r_state.h"
#include "doomdata.h"
#include "doomstat.h"
#include "i_system.h"
#include "m_misc.h"
#include "r_defs.h"
#include "r_state.h"
#include "r_sky.h"
#include "v_patch.h"
#include "w_wad.h"
#include "z_zone.h"

#include "ipu/ipu_interface.h"

extern int setblocks; // r_main.c
extern int setdetail; // r_main.c



void IPU_G_LoadLevel_PackMiscValues(void* buf) {
  assert(sizeof(G_LoadLevel_MiscValues_t) <= IPUMISCVALUESSIZE);

  G_LoadLevel_MiscValues_t pack;
  pack.gameepisode = gameepisode;
  pack.gamemap = gamemap; 
  pack.gamemode = gamemode;
  pack.deathmatch = deathmatch;
  pack.skyflatnum = skyflatnum;
  pack.skytexture = skytexture;
  for (int i = 0; i < MAXPLAYERS; ++i)
    pack.playeringame[i] = playeringame[i];
  
  char lumpname[9];
  lumpname[0] = 'E';
  lumpname[1] = '0' + gameepisode;
  lumpname[2] = 'M';
  lumpname[3] = '0' + gamemap;
  lumpname[4] = '\0';
  pack.lumpnum = W_GetNumForName(lumpname);

  memcpy(buf, &pack, sizeof(pack));
}


#define MAX_BUFFERED_MAPPED_LINES 1000
static int mapped_line_buf[MAX_BUFFERED_MAPPED_LINES];
static int mapped_line_count = 0;

void IPU_NotifyLineMapped(line_t *line) {
  int index = line - lines;
  mapped_line_buf[mapped_line_count++] = index;
  if (mapped_line_count == MAX_BUFFERED_MAPPED_LINES) {
    I_Error("\nERROR: mapped_line_buf circular buffer is overflowing\n");
  }
}

void IPU_CheckAlreadyMappedLines(void) {
  for (int i = 0; i < numlines; ++i) {
    if (lines[i].flags & ML_MAPPED) {
      IPU_NotifyLineMapped(&lines[i]);
   }
  }
}

void IPU_G_Ticker_PackMiscValues(void* buf) {
  assert(sizeof(G_Ticker_MiscValues_t) <= IPUMISCVALUESSIZE);

  G_Ticker_MiscValues_t* pack = (G_Ticker_MiscValues_t*) buf;
  pack->gamestate = gamestate;
  if (gamestate != GS_LEVEL) 
    return;

  assert(consoleplayer == displayplayer);
  pack->consoleplayer = consoleplayer;
  pack->player.x = players[consoleplayer].mo->x;
  pack->player.y = players[consoleplayer].mo->y;
  pack->player.z = players[consoleplayer].mo->z;
  pack->player.angle = players[consoleplayer].mo->angle;
  pack->player.extralight = players[consoleplayer].extralight;
  pack->player.viewz = players[consoleplayer].viewz;
  pack->player.fixedcolormap = players[consoleplayer].fixedcolormap;

  for (int i = 0; i < IPUMAPPEDLINEUPDATES; ++i) {
    if (!mapped_line_count) {
      pack->mappedline_updates[i] = -1;
      break;  
    }
    pack->mappedline_updates[i] = mapped_line_buf[--mapped_line_count];
  }
}

void IPU_G_Responder_PackMiscValues(void* src_buf, void* dst_buf) {
  assert(sizeof(G_Responder_MiscValues_t) <= IPUMISCVALUESSIZE);
  memcpy(dst_buf, src_buf, sizeof(G_Responder_MiscValues_t));
}

void IPU_R_RenderPlayerView_PackMiscValues(void* buf) {
  // R_RenderPlayerView_MiscValues_t* pack = (R_RenderPlayerView_MiscValues_t*) buf;
  // Nothing to pack
}

void IPU_R_ExecuteSetViewSize_PackMiscValues(void* buf) {
  assert(sizeof(R_ExecuteSetViewSize_MiscValues_t) <= IPUMISCVALUESSIZE);
  R_ExecuteSetViewSize_MiscValues_t* pack = buf;
  pack->setblocks = setblocks;
  pack->setdetail = setdetail;
}

void IPU_R_Init_PackMiscValues(void* buf) {
  assert(sizeof(R_Init_MiscValues_t) <= IPUMISCVALUESSIZE);
  R_Init_MiscValues_t* pack = buf;
  pack->TEXTURE1_lumpnum = W_GetNumForName("TEXTURE1");
  pack->PNAMES_lumpnum = W_GetNumForName("PNAMES");
}

void IPU_Setup_PackMarkNums(void* buf) {
  int bufpos = 10 * sizeof(short);
  char namebuf[9] = "AMMNUM0\0";

  for (int i = 0; i < 10; ++i, ++namebuf[6]) {
    lumpindex_t lumpnum = W_GetNumForName(namebuf);
    int size = W_LumpLength(lumpnum);
    if (bufpos + size > IPUAMMARKBUFSIZE) {
      I_Error("\nERROR: not enough space for AM mark patches\n");
    }
    patch_t* lump = W_CacheLumpNum(lumpnum, PU_STATIC);
    memcpy(((char*) buf) + bufpos, lump, size);
    W_ReleaseLumpNum(lumpnum);
    ((short*)buf)[i] = bufpos;
    bufpos += size;
  }
  if (bufpos != IPUAMMARKBUFSIZE) {
    I_Error("\nERROR: marknum patches don't fill buffer...?\n");
  }
}

int IPU_LoadLumpNum(int lumpnum, byte* buf, int maxSize) {
    const int size = W_LumpLength(lumpnum);
    const int required = size + sizeof(int); // Space for size field
    if (required > maxSize) {
        I_Error("\nERROR: Need %d bytes to transfer lump %d to IPU, only have %d\n",
        required, lumpnum, maxSize);
    }
    ((int*)buf)[0] = size;
    byte* data = W_CacheLumpNum(lumpnum, PU_STATIC);
    memcpy(buf + sizeof(int), data, size);
    W_ReleaseLumpNum(lumpnum);
    return size;
}


int IPU_LoadLumpName(const char* name, byte* buf, int maxSize) {
  return IPU_LoadLumpNum(W_GetNumForName(name), buf, maxSize);
}


void IPU_LoadSectorPicNums(byte* buf, int maxSize) {
    int required = numsectors * 2 * sizeof(short);
    if (required > maxSize)
        I_Error("\nERROR: Need %d bytes to transfer SectorPicNums to IPU, only have %d\n", required, maxSize);
        
    short* data = (short*) buf;
    for (int i = 0; i < numsectors; ++i) {
      data[2 * i] = sectors[i].floorpic;
      data[2 * i + 1] = sectors[i].ceilingpic;
    }
}