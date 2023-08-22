#include <Vertex.hpp>

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
    void P_LoadThings(const unsigned char *buf);
    void IPU_Setup_UnpackMarkNums(const unsigned char* buf);
};


// --------------- P_Setup ----------------- //

struct P_SetupLevel_SubFunc {
  void (*func)(const unsigned char*);
  int lump_num;
};
static P_SetupLevel_SubFunc setupLevelSubfuncs[14] = {
  {P_SetupLevel_pt0, 0},
  {P_LoadBlockMap, ML_BLOCKMAP},
  {P_LoadVertexes, ML_VERTEXES},
  {P_LoadSectors, ML_SECTORS},
  {P_LoadSideDefs, ML_SIDEDEFS},
  {P_LoadLineDefs, ML_LINEDEFS},
  {P_LoadSubsectors, ML_SSECTORS},
  {P_LoadNodes, ML_NODES},
  // {P_LoadSegs, ML_SEGS},
  // {P_GroupLines, ML_SEGS},
  // {P_LoadReject, ML_REJECT},
  {P_LoadThings, ML_THINGS},
  // {P_SpawnSpecials, 0}
  {NULL, 0},  /* SENTINEL */
};

// THIS IS CALLING VIA A POINTER. THAT'S PROBABLY BAD...
class P_SetupLevel_Vertex : public poplar::Vertex {
  poplar::Input<poplar::Vector<unsigned char>> lumpBuf;
  poplar::Output<int> lumpNum;
 public:
  bool compute() {
    static int step = 0;
    setupLevelSubfuncs[step].func(&lumpBuf[0]);
    step += 1;
    *lumpNum = gamelumpnum + setupLevelSubfuncs[step].lump_num;
    if (setupLevelSubfuncs[step].func == NULL) {
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