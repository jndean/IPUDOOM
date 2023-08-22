#include <Vertex.hpp>

#include <print.h>

// #include "doomdata.h"

#include "ipu_transfer.h"


extern "C" {
    // void P_SetupLevel_pt0(const unsigned char *buf);
};


// --------------- P_Setup ----------------- //

struct R_RenderPlayerView_Vertex : public poplar::Vertex {
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  poplar::InOut<poplar::Vector<unsigned char>> frame;

  void compute() {
    // printf("Running R_RenderPlayerView_Vertex\n");
    // AM_Drawer(&frame[0]);
    for (int i = 0; i < 100; ++i){
      frame[i + 320 * i    ] = 1;
      frame[i + 320 * i + 1] = 1;
    }
    IPU_R_RenderPlayerView_UnpackMiscValues(
      (R_RenderPlayerView_MiscValues_t*) &miscValues[0]
    );
    return ;
  }
};
