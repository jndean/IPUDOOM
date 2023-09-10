#ifndef __IPU_INTERFACE__
#define __IPU_INTERFACE__
#ifdef __cplusplus
extern "C" {
#endif

#include "d_event.h"
#include "d_player.h"
#include "d_mode.h"
#include "doomdef.h"
#include "doomtype.h"
#include "m_fixed.h"
#include "tables.h"


#define IPUMAXLUMPBYTES (32000)
#define IPUMISCVALUESSIZE (116)
#define IPUMAXEVENTSPERTIC (5)
#define IPUAMMARKBUFSIZE (544)
#define IPUMAPPEDLINEUPDATES (2)

#define IPUFIRSTRENDERTILE (0)
#define IPUNUMRENDERTILES (8)
#define IPUCOLSPERRENDERTILE (SCREENWIDTH / IPUNUMRENDERTILES)



typedef struct {
    int gameepisode;
    int gamemap;
    int lumpnum;
    GameMode_t gamemode;
    int deathmatch;
    boolean playeringame[MAXPLAYERS];
} G_LoadLevel_MiscValues_t;

typedef struct {
    fixed_t x, y, z;
    angle_t angle;
    int extralight;
    fixed_t viewz;
    int fixedcolormap;
} IPUTransfer_playerstate_t;

typedef struct {
    int consoleplayer;
    gamestate_t gamestate;
    IPUTransfer_playerstate_t player;

    int mappedline_updates[IPUMAPPEDLINEUPDATES];
} G_Ticker_MiscValues_t;


typedef struct {
    event_t events[IPUMAXEVENTSPERTIC];
    unsigned char num_ev;
    // Other things for responder here
} G_Responder_MiscValues_t;

typedef struct {
    // nothing
    int dummy; // just so struct is not empty
} R_RenderPlayerView_MiscValues_t;


typedef struct {
    int setblocks;
    int setdetail;
} R_ExecuteSetViewSize_MiscValues_t;


#ifdef __cplusplus
}
#endif
#endif // __IPU_INTERFACE__ //