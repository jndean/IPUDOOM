#include <poplar/Vertex.hpp>
#include "poplar/StackSizeDefs.hpp" 

#include <print.h>

#include "doomdata.h"

#include "ipu_transfer.h"


extern "C" {
    void P_SetupLevel_pt0(const unsigned char *buf);
    void P_LoadBlockMap(const unsigned char *buf);
    void P_LoadVertexes(const unsigned char *buf);
    void P_LoadSectors(const unsigned char *buf);
    void P_LoadSideDefs(const unsigned char *buf);
    void P_LoadLineDefs(const unsigned char *buf);
    void P_LoadSubsectors(const unsigned char *buf);
    void P_LoadNodes(const unsigned char *buf);
    void P_LoadSegs(const unsigned char *buf);
    void P_LoadThings(const unsigned char *buf);
    void IPU_Setup_UnpackMarkNums(const unsigned char* buf);
};

// DEF_STACK_USAGE(400, "__runCodelet_P_SetupLevel_Vertex");


// --------------- P_Setup ----------------- //

// struct P_SetupLevel_SubFunc {
//   void (*func)(const unsigned char*);
//   int lump_num;
// };
// static P_SetupLevel_SubFunc setupLevelSubfuncs[14] = {
//   {P_SetupLevel_pt0, 0},
//   {P_LoadBlockMap, ML_BLOCKMAP},
//   {P_LoadVertexes, ML_VERTEXES},
//   {P_LoadSectors, ML_SECTORS},
//   {P_LoadSideDefs, ML_SIDEDEFS},
//   {P_LoadLineDefs, ML_LINEDEFS},
//   {P_LoadSubsectors, ML_SSECTORS},
//   {P_LoadNodes, ML_NODES},
//   {P_LoadSegs, ML_SEGS},
//   // {P_GroupLines, ML_SEGS},
//   // {P_LoadReject, ML_REJECT},
//   {P_LoadThings, ML_THINGS},
//   // {P_SpawnSpecials, 0}
//   {NULL, 0},  /* SENTINEL */
// };

// DEF_FUNC_CALL_PTRS("__runCodelet_P_SetupLevel_Vertex",
// "P_LoadBlockMap,P_LoadVertexes,P_LoadSectors,P_LoadSideDefs,P_LoadLineDefs,P_LoadSubsectors");

class P_SetupLevel_Vertex : public poplar::Vertex {
  poplar::Input<poplar::Vector<unsigned char>> lumpBuf;
  poplar::Output<int> lumpNum;
 public:
  bool compute() {
    static int step = 0;
    int next;
    // Switch statements are bonkers
    switch (step++) {
      next = 0;                  case 0: P_SetupLevel_pt0(&lumpBuf[0]);
      next = ML_BLOCKMAP; break; case 1: P_LoadBlockMap(&lumpBuf[0]);
      next = ML_VERTEXES; break; case 2: P_LoadVertexes(&lumpBuf[0]);
      next = ML_SECTORS;  break; case 3: P_LoadSectors(&lumpBuf[0]);
      next = ML_SIDEDEFS; break; case 4: P_LoadSideDefs(&lumpBuf[0]);
      next = ML_LINEDEFS; break; case 5: P_LoadLineDefs(&lumpBuf[0]);
      next = ML_SSECTORS; break; case 6: P_LoadSubsectors(&lumpBuf[0]);
      next = ML_NODES;    break; case 7: P_LoadNodes(&lumpBuf[0]);
      next = ML_SEGS;     break; case 8: P_LoadSegs(&lumpBuf[0]);
      next = ML_THINGS;   break; case 9: P_LoadThings(&lumpBuf[0]);
      next = -1;          break;    
    }
    *lumpNum = gamelumpnum + next;
    if (next == -1) {
      step = 0;
    }
    return true;
  }
};


// ------------ IPU_Setup ------------ //

class IPU_Setup_UnpackMarknumSprites_Vertex : public poplar::Vertex {
  poplar::Input<poplar::Vector<unsigned char>> buf;
 public:
  bool compute() {
    IPU_Setup_UnpackMarkNums(&buf[0]);     
    return true;
  }
};