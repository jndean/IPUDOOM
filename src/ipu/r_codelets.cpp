#include <cassert>
#include <Vertex.hpp>

#include <print.h>

#include "doomstat.h"
#include "i_video.h"

#include "ipu_transfer.h"
#include "ipu_utils.h"
#include "ipu_texturetiles.h"


extern "C" {
  __SUPER__ void R_InitTextures(int* maptex);
  __SUPER__ void R_InitTextures_TT(int* maptex);
  __SUPER__ void R_RenderPlayerView(player_t *player);
  __SUPER__ void R_ExecuteSetViewSize(void);
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


struct R_Init_Vertex: public poplar::SupervisorVertex {
  
  poplar::Output<poplar::Vector<unsigned>> progBuf;
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  poplar::Input<poplar::Vector<unsigned char>> lumpBuf;
  poplar::Output<int> lumpNum;

  __SUPER__ 
  void compute() {
    static int step = 0;


    switch (step++) {
    case 0:
      *lumpNum = ((R_Init_MiscValues_t*)&miscValues[0])->TEXTURE1_lumpnum;
    
    break; case 1:
      R_InitTextures((int*)&lumpBuf[0]);
      IPU_R_InitColumnRequester(&progBuf[0], progBuf.size());

      *lumpNum = 0;
      step = 0;
    }

  }
};


struct 
[[
poplar::constraint("region(*nonExecutableDummy) != region(*progBuf)"),
poplar::constraint("elem(*textureCache) != elem(*progBuf)"),
poplar::constraint("elem(*textureCache) != elem(*commsBuf)"),
poplar::constraint("elem(*progBuf) != elem(*commsBuf)"),
]] 
R_RenderPlayerView_Vertex : public poplar::SupervisorVertex {
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  poplar::InOut<poplar::Vector<unsigned char>> frame;
  poplar::InOut<poplar::Vector<
    int, poplar::VectorLayout::SPAN, 4, true>> nonExecutableDummy;
  poplar::InOut<poplar::Vector<unsigned>> progBuf;
  poplar::InOut<poplar::Vector<unsigned>> commsBuf;
  poplar::InOut<poplar::Vector<unsigned>> textureCache;
  poplar::Input<poplar::Vector<int>> textureRange;

  __SUPER__ void compute() {
    assert(&frame[0] == I_VideoBuffer);
    tileLocalProgBuf = &progBuf[0];
    tileLocalCommsBuf = &commsBuf[0];
    tileLocalTextureBuf = &textureCache[0];
    tileLocalTextureRange = &textureRange[0];

    IPU_R_RenderPlayerView_UnpackMiscValues(
      (R_RenderPlayerView_MiscValues_t*) &miscValues[0]
    );

    // For visualisation of progress
    // memset(I_VideoBuffer, 0, IPUCOLSPERRENDERTILE * SCREENHEIGHT);

    R_RenderPlayerView(&players[displayplayer]);
    IPU_R_RenderTileDone();
    return;
  }
};

struct R_InitTexture_Vertex : public poplar::SupervisorVertex {
  poplar::Output<poplar::Vector<unsigned>> progBuf;

  __SUPER__ void compute() {
    IPU_R_InitTextureTile(&progBuf[0], progBuf.size()); 
  }
};

struct 
[[
poplar::constraint("region(*dummy)  != region(*progBuf)"),
poplar::constraint("elem(*textureBuf) != elem(*progBuf)"),
poplar::constraint("elem(*textureBuf) != elem(*commsBuf)"),
poplar::constraint("elem(*progBuf)    != elem(*commsBuf)"),
]] 
R_FulfilColumnRequests_Vertex : public poplar::SupervisorVertex {
  poplar::InOut<poplar::Vector<
    int, poplar::VectorLayout::SPAN, 4, true>> dummy;
  poplar::InOut<poplar::Vector<unsigned>> progBuf;
  poplar::InOut<poplar::Vector<unsigned>> commsBuf;
  poplar::Output<poplar::Vector<unsigned char>> textureBuf;
  poplar::Input<poplar::Vector<int>> textureOffsets;
  poplar::Input<poplar::Vector<int>> textureRange;

  __SUPER__ void compute() {
    tileLocalTextureRange = &textureRange[0];
    tileLocalTextureOffsets = &textureOffsets[0];
    
    IPU_R_FulfilColumnRequest(&progBuf[0], &textureBuf[0], &commsBuf[0]);
  }
};


struct R_InitSans_Vertex : public poplar::SupervisorVertex {
  poplar::Output<poplar::Vector<unsigned>> progBuf;

  __SUPER__ void compute() {
    // Reuse IPU_R_InitTextureTile because the 'done' flag receiving program
    // it compiles is perfectly valid for use by a sans tile
    IPU_R_InitTextureTile(&progBuf[0], progBuf.size());   
  }
};

struct 
[[
poplar::constraint("region(*dummy) != region(*progBuf)"),
poplar::constraint("elem(*progBuf) != elem(*commsBuf)"),
]] 
R_Sans_Vertex : public poplar::SupervisorVertex {
  poplar::InOut<poplar::Vector<
    int, poplar::VectorLayout::SPAN, 4, true>> dummy;
  poplar::InOut<poplar::Vector<unsigned>> progBuf;
  poplar::InOut<poplar::Vector<unsigned>> commsBuf;

  __SUPER__ void compute() {
    IPU_R_Sans(&progBuf[0], &commsBuf[0]);
  }
};
