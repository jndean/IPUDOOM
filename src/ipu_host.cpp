#include "ipu_host.h"

#include <algorithm>
#include <memory>
#include <poplar/DeviceManager.hpp>
#include <poplar/Engine.hpp>
#include <poplar/IPUModel.hpp>

#include "i_video.h"
#include "ipu/ipu_interface.h"
#include "ipu_transfer.h"


poplar::Device getIpu(bool use_hardware, int num_ipus) {
  if (use_hardware) {
    auto manager = poplar::DeviceManager::createDeviceManager();
    auto devices = manager.getDevices(poplar::TargetType::IPU, num_ipus);
    auto it =
        std::find_if(devices.begin(), devices.end(), [](poplar::Device& device) { return device.attach(); });
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
  void run_AM_Drawer();
  void run_G_DoLoadLevel();
  void run_G_Ticker();
  void run_G_Responder(G_Responder_MiscValues_t* buf);

  poplar::Device m_ipuDevice;
  poplar::Graph m_ipuGraph;
  std::unique_ptr<poplar::Engine> m_ipuEngine;

 private:
  poplar::Tensor m_lumpBuf;
  poplar::Tensor m_lumpNum;
  int m_lumpNum_h;
  poplar::Tensor m_miscValuesBuf;
  unsigned char m_miscValuesBuf_h[IPUMISCVALUESSIZE];
};

IpuDoom::IpuDoom()
    : m_ipuDevice(getIpu(false, 1)),
      m_ipuGraph(/*poplar::Target::createIPUTarget(1, "ipu2")*/ m_ipuDevice.getTarget()),
      m_ipuEngine(nullptr) {
  buildIpuGraph();
}
IpuDoom::~IpuDoom(){};

void IpuDoom::buildIpuGraph() {
  m_ipuGraph.addCodelets("build/ipu_rt.gp");

  // -------- AM_Drawer_CS ------ //

  poplar::Tensor printbuf =
      m_ipuGraph.addVariable(poplar::CHAR, {(ulong)IPUPRINTBUFSIZE}, "ipuprint_buf");
  m_ipuGraph.setTileMapping(printbuf, 0);
  auto printbufOutStream =
      m_ipuGraph.addDeviceToHostFIFO("printbuf-stream", poplar::CHAR, IPUPRINTBUFSIZE);

  poplar::ComputeSet GetPrintbuf_CS = m_ipuGraph.addComputeSet("GetPrintbuf_CS");
  poplar::VertexRef vtx = m_ipuGraph.addVertex(GetPrintbuf_CS, "IPU_GetPrintBuf_Vertex", {{"printbuf", printbuf}});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, IPUPRINTBUFSIZE);

  poplar::program::Sequence  GetPrintbuf_prog({
      poplar::program::Execute(GetPrintbuf_CS),
      poplar::program::Copy(printbuf, printbufOutStream),
  });

  // -------- AM_Drawer_CS ------ //

  poplar::Tensor ipuFrame =
      m_ipuGraph.addVariable(poplar::UNSIGNED_CHAR, {(ulong)SCREENWIDTH * SCREENHEIGHT}, "frame");
  m_ipuGraph.setTileMapping(ipuFrame, 0);
  auto frameInStream =
      m_ipuGraph.addHostToDeviceFIFO("frame-instream", poplar::UNSIGNED_CHAR, SCREENWIDTH * SCREENHEIGHT);
  auto frameOutStream =
      m_ipuGraph.addDeviceToHostFIFO("frame-outstream", poplar::UNSIGNED_CHAR, SCREENWIDTH * SCREENHEIGHT);

  poplar::ComputeSet AM_Drawer_CS = m_ipuGraph.addComputeSet("AM_Drawer_CS");
  vtx = m_ipuGraph.addVertex(AM_Drawer_CS, "AM_Drawer_Vertex", {{"frame", ipuFrame}});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, 10000000);

  poplar::program::Sequence AM_Drawer_prog({
      poplar::program::Copy(frameInStream, ipuFrame),
      poplar::program::Execute(AM_Drawer_CS),
      poplar::program::Copy(ipuFrame, frameOutStream),
  });

  // -------- IPU_G_DoLoadLevel ------ //

  m_miscValuesBuf = m_ipuGraph.addVariable(poplar::UNSIGNED_CHAR, {(ulong)IPUMISCVALUESSIZE}, "miscValues");
  m_ipuGraph.setTileMapping(m_miscValuesBuf, 0);
  auto miscValuesStream =
      m_ipuGraph.addHostToDeviceFIFO("miscValues-stream", poplar::UNSIGNED_CHAR, IPUMISCVALUESSIZE);
  poplar::ComputeSet G_DoLoadLevel_CS = m_ipuGraph.addComputeSet("G_DoLoadLevel_CS");
  vtx = m_ipuGraph.addVertex(G_DoLoadLevel_CS, "G_DoLoadLevel_Vertex", {{"miscValues", m_miscValuesBuf}});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, 10000000);

  m_lumpBuf = m_ipuGraph.addVariable(poplar::UNSIGNED_CHAR, {(ulong)IPUMAXLUMPBYTES}, "lumpBuf");
  m_lumpNum = m_ipuGraph.addVariable(poplar::INT, {}, "lumpNum");
  m_ipuGraph.setTileMapping(m_lumpBuf, 0);
  m_ipuGraph.setTileMapping(m_lumpNum, 0);
  auto lumpBufStream = m_ipuGraph.addHostToDeviceFIFO("lumpBuf-stream", poplar::UNSIGNED_CHAR, IPUMAXLUMPBYTES);
  auto lumpNumStream = m_ipuGraph.addDeviceToHostFIFO("lumpNum-stream", poplar::INT, 1);
  
  poplar::ComputeSet P_SetupLevel_CS = m_ipuGraph.addComputeSet("P_SetupLevel_CS");
  vtx = m_ipuGraph.addVertex(P_SetupLevel_CS, "P_SetupLevel_pt0_Vertex", {{"lumpNum", m_lumpNum}});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, 100);

  poplar::ComputeSet P_LoadBlockMap_CS = m_ipuGraph.addComputeSet("P_LoadBlockMap_CS");
  vtx = m_ipuGraph.addVertex(P_LoadBlockMap_CS, "P_LoadBlockMap_Vertex", {
    {"lumpNum", m_lumpNum}, {"lumpBuf", m_lumpBuf}});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, 100);

  poplar::ComputeSet P_LoadVertexes_CS = m_ipuGraph.addComputeSet("P_LoadVertexes_CS");
  vtx = m_ipuGraph.addVertex(P_LoadVertexes_CS, "P_LoadVertexes_Vertex", {
    {"lumpNum", m_lumpNum}, {"lumpBuf", m_lumpBuf}});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, 100);


  poplar::program::Sequence G_DoLoadLevel_prog({
      poplar::program::Copy(miscValuesStream, m_miscValuesBuf),
      poplar::program::Execute(G_DoLoadLevel_CS),
      poplar::program::Execute(P_SetupLevel_CS),
      poplar::program::Copy(m_lumpNum, lumpNumStream),
      poplar::program::Copy(lumpBufStream, m_lumpBuf),
      poplar::program::Execute(P_LoadBlockMap_CS),
      poplar::program::Copy(m_lumpNum, lumpNumStream),
      poplar::program::Copy(lumpBufStream, m_lumpBuf),
      poplar::program::Execute(P_LoadVertexes_CS),
      GetPrintbuf_prog,
      // poplar::program::Copy(lumpStream, m_lumpBuf),
      // poplar::program::Execute(IPU_UnpackLineDefs_CS),
  });

  // ---------------- G_Ticker --------------//


  poplar::ComputeSet G_Ticker_CS = m_ipuGraph.addComputeSet("G_Ticker_CS");
  vtx = m_ipuGraph.addVertex(G_Ticker_CS, "G_Ticker_Vertex", {{"miscValues", m_miscValuesBuf}});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, 100);
  
  poplar::program::Sequence G_Ticker_prog({
      poplar::program::Copy(miscValuesStream, m_miscValuesBuf),
      poplar::program::Execute(G_Ticker_CS),
      GetPrintbuf_prog,
  });


  // ---------------- G_Responder --------------//


  poplar::ComputeSet G_Responder_CS = m_ipuGraph.addComputeSet("G_Responder_CS");
  vtx = m_ipuGraph.addVertex(G_Responder_CS, "G_Responder_Vertex", {{"miscValues", m_miscValuesBuf}});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, 100);
  
  poplar::program::Sequence G_Responder_prog({
      poplar::program::Copy(miscValuesStream, m_miscValuesBuf),
      poplar::program::Execute(G_Responder_CS),
      GetPrintbuf_prog,
  });



  // ---------------- Final prog --------------//

  printf("Creating engine...\n");
  m_ipuEngine = std::make_unique<poplar::Engine>(std::move(poplar::Engine(
    m_ipuGraph, {
      AM_Drawer_prog,
      G_DoLoadLevel_prog,
      G_Ticker_prog,
      G_Responder_prog,
  })));

  // m_ipuEngine->connectStream("frame-instream", I_VideoBuffer); // Can do this is IPU_Init run afer video
  // init m_ipuEngine->connectStream("frame-outstream", I_VideoBuffer);
  m_ipuEngine->connectStreamToCallback("printbuf-stream", [](void* p) {
    if (((char*)p)[0] == '\0') return;
    printf("[IPU] %.*s\n", IPUPRINTBUFSIZE, (char*)p);
  });

  m_ipuEngine->connectStream("miscValues-stream", m_miscValuesBuf_h);

  m_ipuEngine->connectStream("lumpNum-stream", &m_lumpNum_h);
  m_ipuEngine->connectStreamToCallback("lumpBuf-stream", [this](void* p) {
    printf("Fulfilling request for lump %d\n", m_lumpNum_h);
    IPU_LoadLumpForTransfer(m_lumpNum_h, (byte*) p);
  });

  m_ipuEngine->load(m_ipuDevice);
}

void IpuDoom::run_AM_Drawer() {
  static bool setup = true;
  if (setup) {
    setup = false;
    m_ipuEngine->connectStream("frame-instream", I_VideoBuffer);
    m_ipuEngine->connectStream("frame-outstream", I_VideoBuffer);
  }
  m_ipuEngine->run(0);
}
void IpuDoom::run_G_DoLoadLevel() { IPU_G_LoadLevel_PackMiscValues(m_miscValuesBuf_h); m_ipuEngine->run(1); }
void IpuDoom::run_G_Ticker() { IPU_G_Ticker_PackMiscValues(m_miscValuesBuf_h); m_ipuEngine->run(2); }
void IpuDoom::run_G_Responder(G_Responder_MiscValues_t* src_buf) { 
  IPU_G_Responder_PackMiscValues(src_buf, m_miscValuesBuf_h);
  m_ipuEngine->run(3); 
}

static std::unique_ptr<IpuDoom> ipuDoomInstance = nullptr;
extern "C" {
void IPU_Init(void) { ipuDoomInstance = std::make_unique<IpuDoom>(); }
void IPU_AM_Drawer() { ipuDoomInstance->run_AM_Drawer(); }
void IPU_G_DoLoadLevel() { ipuDoomInstance->run_G_DoLoadLevel(); }
void IPU_G_Ticker() { ipuDoomInstance->run_G_Ticker(); }
void IPU_G_Responder(G_Responder_MiscValues_t* buf) { ipuDoomInstance->run_G_Responder(buf); }
}