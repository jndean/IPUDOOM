
#include <Vertex.hpp>

#include "doomtype.h"


typedef uint8_t pixel_t;


extern "C" {
    void AM_LevelInit(void);
    void AM_Drawer(pixel_t*);
    void G_DoLoadLevel(void);
};


class AM_LevelInit_Vertex : public poplar::Vertex {
 public:
  bool compute() {
    AM_LevelInit(); 
    return true;
  }
};


class AM_Drawer_Vertex : public poplar::Vertex {
 public:
  poplar::InOut<poplar::Vector<unsigned char>> frame;

  bool compute() {
    AM_Drawer(&frame[0]);
    return true;
  }
};


// class G_Ticker_Vertex : public poplar::Vertex { Call G_ticker in D_ProcessEvents
//  public:

//   bool compute() {
//     return true;
//   }
// };

class D_ProcessEvents_Vertex : public poplar::Vertex {
 public:

  bool compute() {
    // Unpack and process events
    return true;
  }
};


class TMP_UnpackTicTransfer_Vertex : public poplar::Vertex {
 public:

  bool compute() {
    // Temporary vertex to unpack state copied to IPU for dev, to keep
    // cpu and ipu in sync
    return true;
  }
};

class G_DoLoadLevel_Vertex : public poplar::Vertex {
 public:
    poplar::Input<poplar::Vector<unsigned char>> buf;

  bool compute() {
    G_DoLoadLevel();
    return true;
  }
};

class IPU_UnpackVertexes_Vertex : public poplar::Vertex {
 public:
    poplar::Input<poplar::Vector<unsigned char>> lumpBuf;

  bool compute() {
    // IPU_UnpackLevel(&levelState[0]);
    return true;
  }
};