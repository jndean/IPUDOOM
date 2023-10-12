
#include <Vertex.hpp>
#include <cassert>

#include "doomtype.h"
#include "i_video.h"

#include "ipu_utils.h"
#include "ipu_texturetiles.h"


typedef uint8_t pixel_t;


extern "C" {
  __SUPER__ void AM_LevelInit(void);
  __SUPER__ void AM_Drawer(pixel_t*);
  __SUPER__ void IPU_Setup_UnpackMarkNums(const unsigned char* buf);
};


class AM_LevelInit_Vertex : public poplar::SupervisorVertex {
 public:
  __SUPER__ bool compute() {
    AM_LevelInit(); 
    return true;
  }
};


class AM_Drawer_Vertex : public poplar::SupervisorVertex {
 public:
  poplar::InOut<poplar::Vector<unsigned char>> frame;

  __SUPER__ void compute() {
    assert(&frame[0] == I_VideoBuffer);
    AM_Drawer(&frame[0]);
  }
};


// ------  Happens before most CPU setup is done ------------- //

struct IPU_Init_Vertex : public poplar::SupervisorVertex {

  __SUPER__ void compute() {

    // Deduce logical tile ID
    int physical = __builtin_ipu_get_tile_id();
    int row = physical / 64;
    int subcolumn = physical % 64;
    int column = subcolumn / 4;
    int subtile = subcolumn % 4;
    int logical = (92 * column) + (subtile / 2);
    if (subtile & 1) {
        row = 45 - row;
    }
    logical += 2 * row;
    tileID = logical;

  }
};


// ------  Happens after most CPU setup is done ------------- //

struct IPU_MiscSetup_Vertex : public poplar::SupervisorVertex {
  poplar::InOut<poplar::Vector<unsigned char>> frame;

  __SUPER__ void compute() {

    I_VideoBuffer = &frame[0];

  }
};

struct IPU_UnpackMarknumSprites_Vertex : public poplar::SupervisorVertex {
  poplar::Input<poplar::Vector<unsigned char>> buf;

  __SUPER__ void compute() {
    IPU_Setup_UnpackMarkNums(&buf[0]);
    return;
  }
};