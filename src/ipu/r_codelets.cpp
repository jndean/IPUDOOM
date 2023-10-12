#include <cassert>
#include <Vertex.hpp>

#include <print.h>

#include "doomstat.h"
#include "i_video.h"

#include "ipu_transfer.h"
#include "ipu_utils.h"
#include "ipu_texturetiles.h"


extern "C" {
  __SUPER__ void R_InitTextures(int* maptex, R_Init_MiscValues_t* miscVals);
  __SUPER__ void R_RenderPlayerView(player_t *player);
  __SUPER__ void R_ExecuteSetViewSize(void);
};


struct R_Init_Vertex: public poplar::SupervisorVertex {
  
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  poplar::Input<poplar::Vector<unsigned char>> lumpBuf;
  poplar::Output<int> lumpNum;

  __SUPER__ 
  void compute() {
    static int step = 0;

    switch (step++) {
    case 0:
      *lumpNum = 105;
    
    break; case 1:
      R_InitTextures((int*)&lumpBuf[0], (R_Init_MiscValues_t*)&miscValues[0]);
      IPU_R_InitColumnRequester();

      *lumpNum = 0;
      step = 0;
    }

  }
};


struct R_RenderPlayerView_Vertex : public poplar::SupervisorVertex {
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  poplar::InOut<poplar::Vector<unsigned char>> frame;

  __SUPER__ void compute() {
    assert(&frame[0] == I_VideoBuffer);

    IPU_R_RenderPlayerView_UnpackMiscValues(
      (R_RenderPlayerView_MiscValues_t*) &miscValues[0]
    );

    R_RenderPlayerView(&players[displayplayer]);
    return ;
  }
};


class R_ExecuteSetViewSize_Vertex : public poplar::SupervisorVertex {
 public:
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  __SUPER__ 
  void compute() {
    IPU_R_ExecuteSetViewSize_UnpackMiscValues(
      (R_ExecuteSetViewSize_MiscValues_t*) &miscValues[0]
    );
    R_ExecuteSetViewSize();
  }
};