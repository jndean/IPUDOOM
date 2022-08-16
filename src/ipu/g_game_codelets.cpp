
#include <Vertex.hpp>

#include "ipu_transfer.h"

extern "C" {
    void G_DoLoadLevel(void);
};


class G_DoLoadLevel_Vertex : public poplar::Vertex {
 public:
    poplar::Input<poplar::Vector<unsigned char>> miscValues;

  bool compute() {

    IPU_G_LoadLevel_UnpackMiscValues((G_LoadLevel_MiscValues_t*)&miscValues[0]);
    G_DoLoadLevel();
    return true;
  }
};
