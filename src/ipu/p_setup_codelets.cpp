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
    __SUPER__ void P_LoadSectorPics(const unsigned char *buf);
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
    
    // Switch statements are bonkers
    switch (step++) {
      *lumpNum = gamelumpnum;                      case  0: P_SetupLevel_pt0(&lumpBuf[0]);
      *lumpNum = gamelumpnum + ML_BLOCKMAP; break; case  1: P_LoadBlockMap  (&lumpBuf[0]);
      *lumpNum = gamelumpnum + ML_VERTEXES; break; case  2: P_LoadVertexes  (&lumpBuf[0]);
      *lumpNum = gamelumpnum + ML_SECTORS;  break; case  3: P_LoadSectors   (&lumpBuf[0]);
      *lumpNum = IPULUMPBUFFLAG_FLATPICS;   break; case  4: P_LoadSectorPics(&lumpBuf[0]);
      *lumpNum = gamelumpnum + ML_SIDEDEFS; break; case  5: P_LoadSideDefs  (&lumpBuf[0]);
      *lumpNum = gamelumpnum + ML_LINEDEFS; break; case  6: P_LoadLineDefs  (&lumpBuf[0]);
      *lumpNum = gamelumpnum + ML_SSECTORS; break; case  7: P_LoadSubsectors(&lumpBuf[0]);
      *lumpNum = gamelumpnum + ML_NODES;    break; case  8: P_LoadNodes     (&lumpBuf[0]);
      *lumpNum = gamelumpnum + ML_SEGS;     break; case  9: P_LoadSegs      (&lumpBuf[0]);
      *lumpNum = gamelumpnum;               break; case 10: P_GroupLines    (&lumpBuf[0]);
                                                         // P_LoadReject     // TODO
      *lumpNum = gamelumpnum + ML_THINGS;   break; case 11: P_LoadThings    (&lumpBuf[0]);
                                                         // P_SpawnSpecials  // TODO
                                                         // R_PrecacheLevel  // TODO
      *lumpNum = gamelumpnum; step = 0;     break;
    }

    // TODO: add e.g. P_SetupLevel_pt2 to do remaining var setup
    // (Could put more in P_SetupLevel_pt0?)
    return true;
  }
};
