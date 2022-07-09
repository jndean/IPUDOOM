#include "ipu_host.h"

#include <algorithm>
#include <memory>
#include <poplar/DeviceManager.hpp>
#include <poplar/Engine.hpp>
#include <poplar/IPUModel.hpp>

#include "i_video.h"

poplar::Device getIpu(bool use_hardware, int num_ipus) {
  if (use_hardware) {
    auto manager = poplar::DeviceManager::createDeviceManager();
    auto devices = manager.getDevices(poplar::TargetType::IPU, num_ipus);
    auto it =
        std::find_if(devices.begin(), devices.end(), [](poplar::Device &device) { return device.attach(); });
    if (it == devices.end()) {
      std::cerr << "IPU: Error attaching to device\n";
      exit(EXIT_FAILURE);
    }
    std::cout << "Attached to IPU " << it->getId() << std::endl;
    return std::move(*it);

  } else {
    poplar::IPUModel ipuModel;
    std::cout << "Using simulated IPU" << std::endl;
    return ipuModel.createDevice();
  }
}

class IpuDoom {
 public:
  IpuDoom();
  ~IpuDoom();

  void buildIpuGraph();
  void run_AM_LevelInit();
  void run_AM_Drawer();

  poplar::Device m_ipuDevice;
  poplar::Graph m_ipuGraph;
  std::unique_ptr<poplar::Engine> m_ipuEngine;
};

IpuDoom::IpuDoom()
    : m_ipuDevice(getIpu(false, 1)), m_ipuGraph(m_ipuDevice.getTarget()), m_ipuEngine(nullptr) {
  buildIpuGraph();
}
IpuDoom::~IpuDoom(){};

void IpuDoom::buildIpuGraph() {
  m_ipuGraph.addCodelets("build/ipu_rt.gp");

  poplar::Tensor ipuFrame =
      m_ipuGraph.addVariable(poplar::UNSIGNED_CHAR, {(ulong)SCREENWIDTH * SCREENHEIGHT}, "frame");
  m_ipuGraph.setTileMapping(ipuFrame, 0);
  auto frameInStream =
      m_ipuGraph.addHostToDeviceFIFO("frame-instream", poplar::UNSIGNED_CHAR, SCREENWIDTH * SCREENHEIGHT);
  auto frameOutStream =
      m_ipuGraph.addDeviceToHostFIFO("frame-outstream", poplar::UNSIGNED_CHAR, SCREENWIDTH * SCREENHEIGHT);

  poplar::ComputeSet AM_Drawer_CS = m_ipuGraph.addComputeSet("AM_Drawer_CS");
  poplar::VertexRef vtx = m_ipuGraph.addVertex(AM_Drawer_CS, "AM_Drawer_Vertex", {{"frame", ipuFrame}});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, 10000000);

  poplar::program::Sequence AM_Drawer_prog({
      poplar::program::Copy(frameInStream, ipuFrame),
      poplar::program::Execute(AM_Drawer_CS),
      poplar::program::Copy(ipuFrame, frameOutStream),
  });

  poplar::ComputeSet AM_LevelInit_CS = m_ipuGraph.addComputeSet("AM_LevelInit_CS");
  vtx = m_ipuGraph.addVertex(AM_LevelInit_CS, "AM_LevelInit_Vertex", {});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, 10000000);

  poplar::program::Sequence AM_LevelInit_prog({
      poplar::program::Execute(AM_LevelInit_CS),
  });

  m_ipuEngine = std::make_unique<poplar::Engine>(
      std::move(poplar::Engine(m_ipuGraph, 
      {
        AM_LevelInit_prog, 
        AM_Drawer_prog
      }
  )));
  //   m_ipuEngine->connectStream("frame-instream", I_VideoBuffer);
  //   m_ipuEngine->connectStream("frame-outstream", I_VideoBuffer);

  m_ipuEngine->load(m_ipuDevice);
}

void IpuDoom::run_AM_LevelInit() { m_ipuEngine->run(0); }

void IpuDoom::run_AM_Drawer() {
  static bool setup = true;
  if (setup) {
    setup = false;
    m_ipuEngine->connectStream("frame-instream", I_VideoBuffer);
    m_ipuEngine->connectStream("frame-outstream", I_VideoBuffer);
  }
  m_ipuEngine->run(1);
}

static std::unique_ptr<IpuDoom> ipuDoomInstance = nullptr;
extern "C" {
void IPU_Init(void) { ipuDoomInstance = std::make_unique<IpuDoom>(); }
void IPU_AM_LevelInit() { ipuDoomInstance->run_AM_LevelInit(); }
void IPU_AM_Drawer() { ipuDoomInstance->run_AM_Drawer(); }
}