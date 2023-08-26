#include <cassert>
#include <Vertex.hpp>

#include <print.h>

#include "doomstat.h"
#include "i_video.h"

#include "ipu_transfer.h"


extern "C" {
  void R_RenderPlayerView(player_t *player);
  void R_ExecuteSetViewSize(void);
};


// --------------- P_Setup ----------------- //

struct R_RenderPlayerView_Vertex : public poplar::Vertex {
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  poplar::InOut<poplar::Vector<unsigned char>> frame;

  void compute() {
    assert(&frame[0] == I_VideoBuffer);

    for (int i = 0; i < 100; ++i){
      frame[i + 320 * i    ] = 1;
      frame[i + 320 * i + 1] = 1;
    }
    IPU_R_RenderPlayerView_UnpackMiscValues(
      (R_RenderPlayerView_MiscValues_t*) &miscValues[0]
    );
    R_RenderPlayerView(&players[displayplayer]);
    return ;
  }
};


class R_ExecuteSetViewSize_Vertex : public poplar::Vertex {
 public:
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  bool compute() {
    IPU_R_ExecuteSetViewSize_UnpackMiscValues(
      (R_ExecuteSetViewSize_MiscValues_t*) &miscValues[0]
    );
    R_ExecuteSetViewSize();
    return true;
  }
};