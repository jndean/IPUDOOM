#include "doomstat.h"
// #include "w_wad.h"

#include "ipu_interface.h"
#include "ipu_transfer.h"

int gamelumpnum;
int requestedlumpnum;

void IPU_G_LoadLevel_UnpackMiscValues(G_LoadLevel_MiscValues_t* pack) {
  gameepisode = pack->gameepisode;
  gamemap = pack->gamemap;
  gamelumpnum = pack->lumpnum;
}

void IPU_G_Ticker_UnpackMiscValues(G_Ticker_MiscValues_t* pack) {
  gamestate = pack->gamestate;
  if (gamestate == GS_LEVEL) {
    am_playerpos.x = pack->player_mobj.x;
    am_playerpos.y = pack->player_mobj.y;
    am_playerpos.z = pack->player_mobj.z;
    am_playerpos.angle = pack->player_mobj.angle;
  }
}


/*
void IPU_UnpackVertexes(const unsigned char* buf) {
    IpuPackedLevel_t* pack = (IpuPackedLevel_t*) buf;
    uint32_t pos = 0;

    numvertexes = *(uint32_t*)buf;
    
    mapvertex_t* ml = (mapvertex_t*)(&buf[4]);
    vertex_t *li not done here;
    for (int i = 0; i < numvertex)
    vertexes = (vertex_t*) &pack->data[pos];
    pos += numvertexes * sizeof(vertex_t);

    numlines = pack->numlines;
    lines = (line_t*) &pack->data[pos];
    pos += numlines * sizeof(line_t);

    // for (int i=0; i <)
    // LATER: repoint other contents of lines


}
 */