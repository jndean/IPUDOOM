#ifndef __IPU_INTERFACE__
#define __IPU_INTERFACE__
#ifdef __cplusplus
extern "C" {
#endif

#include "d_event.h"
#include "d_player.h"
#include "doomdef.h"
#include "doomtype.h"
#include "m_fixed.h"
#include "tables.h"


#define IPUMAXLUMPBYTES 32000
#define IPUMISCVALUESSIZE 116
#define IPUPRINTBUFSIZE 2048
#define IPUMAXEVENTSPERTIC 5
#define IPUAMMARKBUFSIZE 544
#define IPUMAPPEDLINEUPDATES 2


typedef struct {
    int gameepisode;
    int gamemap;
    int lumpnum;
} G_LoadLevel_MiscValues_t;

typedef struct {
    fixed_t x, y, z;
    angle_t angle;
} IPUPlayerPos_t;

typedef struct {
    gamestate_t gamestate;
    IPUPlayerPos_t player_mobj;
    int mappedline_updates[IPUMAPPEDLINEUPDATES];
} G_Ticker_MiscValues_t;


typedef struct {
    event_t events[IPUMAXEVENTSPERTIC];
    unsigned char num_ev;
    // Other things for responder here
} G_Responder_MiscValues_t;

typedef struct {
    int displayplayer;
    player_t players_displayplayer;
    mobj_t displayplayer_mo;
} R_RenderPlayerView_MiscValues_t;



#ifdef __cplusplus
}
#endif
#endif // __IPU_INTERFACE__ //