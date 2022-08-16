#include <Vertex.hpp>

extern "C" {
    void P_SetupLevel(void);
};


class P_SetupLevel_Vertex : public poplar::Vertex {
 public:
  bool compute() {
    P_SetupLevel(); 
    return true;
  }
};
