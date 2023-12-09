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


#define IPUMAXLUMPBYTES (40000)
#define IPUMISCVALUESSIZE (116)
#define IPUMAXEVENTSPERTIC (5)
#define IPUAMMARKBUFSIZE (544)
#define IPUMAPPEDLINEUPDATES (2)
#define IPUSECTORHEIGHTUPDATES (4)
#define IPUPROGBUFSIZE (128)


#define IPUFIRSTRENDERTILE (0)
#define IPUNUMRENDERTILES (32)
#define IPUCOLSPERRENDERTILE (SCREENWIDTH / IPUNUMRENDERTILES)

#define IPUTEXTURETILESPERRENDERTILE (5)
#define IPUTEXTURETILEBUFSIZE (1024 * 400)
#define IPUFIRSTTEXTURETILE (IPUFIRSTRENDERTILE + IPUNUMRENDERTILES)
#define IPUNUMTEXTURETILES (IPUTEXTURETILESPERRENDERTILE * IPUNUMRENDERTILES)
#define IPUNUMTEXTURECACHELINES (1)
#define IPUTEXTURECACHELINESIZE (128 / sizeof(int))
#define IPUMAXNUMTEXTURES (200)
#define IPUCOMMSBUFSIZE (IPUNUMRENDERTILES)

#define COLOURMAPSIZE (8704)

// Flags for special cases of the lump-loading mechanism
#define IPULUMPBUFFLAG_FLATPICS (-100)


typedef struct {
    int gameepisode;
    int gamemap;
    int lumpnum;
    GameMode_t gamemode;
    int deathmatch;
    int skyflatnum;
    int skytexture;
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
    fixed_t ceilingheight, floorheight;
    int sectornum;
} IPUSectorHeightUpdate_t;

typedef struct {
    int consoleplayer;
    gamestate_t gamestate;
    IPUTransfer_playerstate_t player;

    int mappedline_updates[IPUMAPPEDLINEUPDATES];
    IPUSectorHeightUpdate_t sectorheight_updates[IPUSECTORHEIGHTUPDATES];
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

typedef struct {
    int TEXTURE1_lumpnum;
    int PNAMES_lumpnum;
} R_Init_MiscValues_t;


#ifdef __cplusplus
}
#endif
#endif // __IPU_INTERFACE__ //