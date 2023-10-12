#include <poplar/Vertex.hpp>
#include <poplar/StackSizeDefs.hpp>

#include <print.h>

#include "doomdata.h"
#include "i_video.h"

#include "ipu_transfer.h"


extern "C" {
    __SUPER__ void P_SetupLevel_pt0(const unsigned char *buf);
    __SUPER__ void P_LoadBlockMap(const unsigned char *buf);
    __SUPER__ void P_LoadVertexes(const unsigned char *buf);
    __SUPER__ void P_LoadSectors(const unsigned char *buf);
    __SUPER__ void P_LoadSideDefs(const unsigned char *buf);
    __SUPER__ void P_LoadLineDefs(const unsigned char *buf);
    __SUPER__ void P_LoadSubsectors(const unsigned char *buf);
    __SUPER__ void P_LoadNodes(const unsigned char *buf);
    __SUPER__ void P_LoadSegs(const unsigned char *buf);
    __SUPER__ void P_LoadThings(const unsigned char *buf);
    __SUPER__ void P_GroupLines(const unsigned char *buf);
};



// --------------- P_Setup ----------------- //


class P_SetupLevel_Vertex : public poplar::SupervisorVertex {
  poplar::Input<poplar::Vector<unsigned char>> lumpBuf;
  poplar::Output<int> lumpNum;
 public:
  __SUPER__ bool compute() {
    static int step = 0;
    int next;
    // Switch statements are bonkers
    switch (step++) {
      next = 0;                  case  0: P_SetupLevel_pt0(&lumpBuf[0]);
      next = ML_BLOCKMAP; break; case  1: P_LoadBlockMap  (&lumpBuf[0]);
      next = ML_VERTEXES; break; case  2: P_LoadVertexes  (&lumpBuf[0]);
      next = ML_SECTORS;  break; case  3: P_LoadSectors   (&lumpBuf[0]);
      next = ML_SIDEDEFS; break; case  4: P_LoadSideDefs  (&lumpBuf[0]);
      next = ML_LINEDEFS; break; case  5: P_LoadLineDefs  (&lumpBuf[0]);
      next = ML_SSECTORS; break; case  6: P_LoadSubsectors(&lumpBuf[0]);
      next = ML_NODES;    break; case  7: P_LoadNodes     (&lumpBuf[0]);
      next = ML_SEGS;     break; case  8: P_LoadSegs      (&lumpBuf[0]);
      next = 0;           break; case  9: P_GroupLines    (&lumpBuf[0]);
                                       // P_LoadReject     // TODO
      next = ML_THINGS;   break; case 10: P_LoadThings    (&lumpBuf[0]);
                                       // P_SpawnSpecials  // TODO
                                       // R_PrecacheLevel  // TODO
      next = -1;          break;
    }
    *lumpNum = gamelumpnum + next;
    if (next == -1) {
      step = 0;
    }
    // TODO: add e.g. P_SetupLevel_pt2 to do remaining var setup
    // (Could put more in P_SetupLevel_pt0?)
    return true;
  }
};
