
#include <Vertex.hpp>

#include "ipu_transfer.h"

extern "C" {
    __SUPER__ void G_DoLoadLevel(void);
    __SUPER__ void G_Ticker(void);
    __SUPER__ boolean G_Responder(event_t *ev);
};


class G_DoLoadLevel_Vertex : public poplar::SupervisorVertex {
 public:
    poplar::Input<poplar::Vector<unsigned char>> miscValues;
  __SUPER__ bool compute() {
    IPU_G_LoadLevel_UnpackMiscValues((G_LoadLevel_MiscValues_t*)&miscValues[0]);
    G_DoLoadLevel();
    return true;
  }
};


class G_Ticker_Vertex : public poplar::SupervisorVertex {
 public:
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  __SUPER__ bool compute() {
    IPU_G_Ticker_UnpackMiscValues((G_Ticker_MiscValues_t*)&miscValues[0]);
    G_Ticker();
    return true;
  }
};

class G_Responder_Vertex : public poplar::SupervisorVertex {
 public:
  poplar::Input<poplar::Vector<unsigned char>> miscValues;
  __SUPER__ bool compute() {
    // Respond to each of the buffered events in turn
    G_Responder_MiscValues_t* ev_buf = (G_Responder_MiscValues_t*)&miscValues[0];
    for (int i = 0; i < ev_buf->num_ev; ++i) {
      G_Responder(&ev_buf->events[i]);
    }
    return true;
  }
};