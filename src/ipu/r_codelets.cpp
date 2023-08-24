#include <Vertex.hpp>

#include <print.h>

#include "doomstat.h"

#include "ipu_transfer.h"


extern "C" {
  void R_RenderPlayerView(player_t *player);
};


// --------------- P_Setup ----------------- //

struct R_RenderPlayerView_Vertex : public poplar::Vertex {
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  poplar::InOut<poplar::Vector<unsigned char>> frame;

  void compute() {
    // for (int i = 0; i < 100; ++i){
    //   frame[i + 320 * i    ] = 1;
    //   frame[i + 320 * i + 1] = 1;
    // }
    IPU_R_RenderPlayerView_UnpackMiscValues(
      (R_RenderPlayerView_MiscValues_t*) &miscValues[0]
    );
    R_RenderPlayerView(&players[displayplayer]);
    return ;
  }
};
