
#include <Vertex.hpp>

// #include "doomtype.h"
typedef uint8_t pixel_t;


extern "C" {
    void AM_LevelInit(void);
    void AM_Drawer(pixel_t*);
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