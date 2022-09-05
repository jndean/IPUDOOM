#include <Vertex.hpp>

#include "ipu_transfer.h"


extern "C" {
    void P_SetupLevel_pt0(void);
    void P_LoadBlockMap(const unsigned char *buf);
    void P_LoadVertexes(const unsigned char *buf);
    void P_LoadSectors(const unsigned char *buf);
};


class P_SetupLevel_pt0_Vertex : public poplar::Vertex {
  poplar::Output<int> lumpNum;
 public:
  bool compute() {
    P_SetupLevel_pt0();
    *lumpNum = requestedlumpnum;
    return true;
  }
};


class P_LoadBlockMap_Vertex : public poplar::Vertex {
  poplar::Input<poplar::Vector<unsigned char>> lumpBuf;
  poplar::Output<int> lumpNum;
 public:
  bool compute() {
    P_LoadBlockMap(&lumpBuf[0]); 
    *lumpNum = requestedlumpnum;
    return true;
  }
};

class P_LoadVertexes_Vertex : public poplar::Vertex {
  poplar::Input<poplar::Vector<unsigned char>> lumpBuf;
  poplar::Output<int> lumpNum;
 public:
  bool compute() {
    P_LoadVertexes(&lumpBuf[0]); 
    *lumpNum = requestedlumpnum;
    return true;
  }
};


class P_LoadSectors_Vertex : public poplar::Vertex {
  poplar::Input<poplar::Vector<unsigned char>> lumpBuf;
  poplar::Output<int> lumpNum;
 public:
  bool compute() {
    P_LoadSectors(&lumpBuf[0]); 
    *lumpNum = requestedlumpnum;
    return true;
  }
};
