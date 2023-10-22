// Yes, I know this file is full of awful unnecessary boilerplate and interfaces, the
// structure of the IPU<->Host interface is evolving __somewhat organically__ ...

#include "ipu_host.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <poplar/DeviceManager.hpp>
#include <poplar/Engine.hpp>
#include <poplar/IPUModel.hpp>
#include <poplar/Program.hpp>
#include <poplar/HostFunctionCallback.hpp>
#include <poplar/SyncType.hpp>

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
  void run_R_RenderPlayerView();
  void run_R_ExecuteSetViewSize();
  void run_R_Init();
  void run_IPU_Init();
  void run_IPU_MiscSetup();
  void run_G_DoLoadLevel();
  void run_G_Ticker();
  void run_G_Responder(G_Responder_MiscValues_t* buf);

  poplar::Device m_ipuDevice;
  poplar::Graph m_ipuGraph;
  std::unique_ptr<poplar::Engine> m_ipuEngine;

 private:
  poplar::Tensor m_lumpNum;
  int m_lumpNum_h;
  poplar::Tensor m_miscValuesBuf;
  unsigned char m_miscValuesBuf_h[IPUMISCVALUESSIZE];
};

IpuDoom::IpuDoom()
    : m_ipuDevice(getIpu(true, 1)),
      m_ipuGraph(/*poplar::Target::createIPUTarget(1, "ipu2")*/ m_ipuDevice.getTarget()),
      m_ipuEngine(nullptr) {
  buildIpuGraph();
}
IpuDoom::~IpuDoom(){};

void IpuDoom::buildIpuGraph() {
  m_ipuGraph.addCodelets("build/ipu_rt.gp");
  const size_t totalTiles = m_ipuDevice.getTarget().getNumTiles();

  // ---- The main frame buffer ---- //
  poplar::Tensor ipuFrame =
      m_ipuGraph.addVariable(poplar::UNSIGNED_CHAR, {(ulong)SCREENHEIGHT, (ulong)SCREENWIDTH}, "frame");
  assert(IPUCOLSPERRENDERTILE * IPUNUMRENDERTILES == SCREENWIDTH);
  std::vector<poplar::Tensor> ipuFrameSlices;
  for (int renderTile = 0; renderTile < IPUNUMRENDERTILES; ++renderTile) {
    ulong start = renderTile * IPUCOLSPERRENDERTILE;
    ulong end = start + IPUCOLSPERRENDERTILE;
    ipuFrameSlices.push_back(ipuFrame.slice({0, start}, {SCREENHEIGHT, end}).flatten());
    m_ipuGraph.setTileMapping(ipuFrameSlices.back(), renderTile + IPUFIRSTRENDERTILE);
  }
  auto frameInStream =
      m_ipuGraph.addHostToDeviceFIFO("frame-instream", poplar::UNSIGNED_CHAR, SCREENWIDTH * SCREENHEIGHT);
  auto frameOutStream =
      m_ipuGraph.addDeviceToHostFIFO("frame-outstream", poplar::UNSIGNED_CHAR, SCREENWIDTH * SCREENHEIGHT);

  // Stuff for exchange programs
  poplar::Tensor nonExecutableDummy = m_ipuGraph.addVariable(poplar::INT, {totalTiles, 1}, "nonExecutableDummy");
  poplar::Tensor progBuf = m_ipuGraph.addVariable(poplar::UNSIGNED_INT, {totalTiles, IPUPROGBUFSIZE}, "progBuf");
  poplar::Tensor commsBuf = m_ipuGraph.addVariable(poplar::UNSIGNED_INT, {totalTiles, IPUCOMMSBUFSIZE}, "commsBuf");
  for (unsigned t = 0; t < totalTiles; ++t) {
    m_ipuGraph.setTileMapping(nonExecutableDummy[t], t);
    m_ipuGraph.setTileMapping(progBuf[t], t);
    m_ipuGraph.setTileMapping(commsBuf[t], t);
  }
  

  // -------- AM_Drawer_CS ------ //

  poplar::ComputeSet AM_Drawer_CS = m_ipuGraph.addComputeSet("AM_Drawer_CS");
  auto vtx = m_ipuGraph.addVertex(AM_Drawer_CS, "AM_Drawer_Vertex", {{"frame", ipuFrameSlices[0]}});
  m_ipuGraph.setTileMapping(vtx, 0);
  m_ipuGraph.setPerfEstimate(vtx, 10000000);

  poplar::program::Sequence AM_Drawer_prog({
      poplar::program::Copy(frameInStream, ipuFrame),
      poplar::program::Execute(AM_Drawer_CS),
      poplar::program::Copy(ipuFrame, frameOutStream),
  });

  // -------- IPU_G_DoLoadLevel ------ //

  m_miscValuesBuf = m_ipuGraph.addVariable(poplar::UNSIGNED_CHAR, {(ulong)IPUMISCVALUESSIZE}, "miscValues");
  m_ipuGraph.setTileMapping(m_miscValuesBuf, IPUFIRSTRENDERTILE);
  auto miscValuesStream =
      m_ipuGraph.addHostToDeviceFIFO("miscValues-stream", poplar::UNSIGNED_CHAR, IPUMISCVALUESSIZE);
  poplar::ComputeSet G_DoLoadLevel_CS = m_ipuGraph.addComputeSet("G_DoLoadLevel_CS");
  for (int renderTile = 0; renderTile < IPUNUMRENDERTILES; ++renderTile) {
    vtx = m_ipuGraph.addVertex(G_DoLoadLevel_CS, "G_DoLoadLevel_Vertex", {{"miscValues", m_miscValuesBuf}});
    m_ipuGraph.setTileMapping(vtx, renderTile + IPUFIRSTRENDERTILE);
    m_ipuGraph.setPerfEstimate(vtx, 10000000);
  }

  m_lumpNum = m_ipuGraph.addVariable(poplar::INT, {IPUNUMRENDERTILES}, "lumpNum");
  // Manually overlap lumbuf with framebuf, since they're not used at the same time
  assert(IPUMAXLUMPBYTES <= ipuFrame.numElements());
  poplar::Tensor lumpBuf = ipuFrame.flatten().slice(0, IPUMAXLUMPBYTES); // TODO: Fix after frame split over multiple tiles
  auto lumpBufStream = m_ipuGraph.addHostToDeviceFIFO("lumpBuf-stream", poplar::UNSIGNED_CHAR, IPUMAXLUMPBYTES);
  auto lumpNumStream = m_ipuGraph.addDeviceToHostFIFO("lumpNum-stream", poplar::INT, 1);
  
  poplar::ComputeSet P_SetupLevel_CS = m_ipuGraph.addComputeSet("P_SetupLevel_CS");
  for (int renderTile = 0; renderTile < IPUNUMRENDERTILES; ++renderTile) {
    vtx = m_ipuGraph.addVertex(P_SetupLevel_CS, "P_SetupLevel_Vertex", {
      {"lumpNum", m_lumpNum[renderTile]}, {"lumpBuf", lumpBuf}});
    m_ipuGraph.setTileMapping(vtx, renderTile + IPUFIRSTRENDERTILE);
    m_ipuGraph.setPerfEstimate(vtx, 100);
    m_ipuGraph.setTileMapping(m_lumpNum[renderTile], renderTile + IPUFIRSTRENDERTILE);
  }

  poplar::program::Sequence G_DoLoadLevel_prog({
      poplar::program::Copy(miscValuesStream, m_miscValuesBuf),
      poplar::program::Execute(G_DoLoadLevel_CS),
      poplar::program::Repeat(11, poplar::program::Sequence({ // <- number of P_SetupLevel_CS steps
        poplar::program::Execute(P_SetupLevel_CS),
        poplar::program::Copy(m_lumpNum[0], lumpNumStream), // Only listen to first tile's requests
        poplar::program::Sync(poplar::SyncType::GLOBAL), // lumpnum must arrive before lump is loaded
        poplar::program::Copy(lumpBufStream, lumpBuf),
      })),
  });

  // -------- R_Init ------ //

  poplar::ComputeSet R_Init_CS = m_ipuGraph.addComputeSet("R_Init_CS");
  for (int renderTile = 0; renderTile < IPUNUMRENDERTILES; ++renderTile) {
    int logicalTile = IPUFIRSTRENDERTILE + renderTile;
    vtx = m_ipuGraph.addVertex(R_Init_CS, "R_Init_Vertex", {
      {"lumpNum", m_lumpNum[renderTile]},
      {"lumpBuf", lumpBuf}, 
      {"miscValues", m_miscValuesBuf},
      {"progBuf", progBuf[logicalTile]}
    });
    m_ipuGraph.setTileMapping(vtx, logicalTile);
    m_ipuGraph.setPerfEstimate(vtx, 100);
  }
  poplar::ComputeSet R_InitTextureOrSans_CS = m_ipuGraph.addComputeSet("R_InitTextureOrSans_CS");
  for (unsigned tile = IPUFIRSTTEXTURETILE; tile < totalTiles; ++tile) {
    vtx =  m_ipuGraph.addVertex(R_InitTextureOrSans_CS, "R_InitTextureOrSans_Vertex", {
      {"progBuf", progBuf[tile]}, 
    });
    m_ipuGraph.setTileMapping(vtx, tile);
    m_ipuGraph.setPerfEstimate(vtx, 2000);
  }

  poplar::HostFunction requestLumpFromHost = m_ipuGraph.addHostFunction(
    "requestLumpFromHost",
    {poplar::Graph::HostFunctionArgument{poplar::INT, 1}},
    {poplar::Graph::HostFunctionArgument{poplar::UNSIGNED_CHAR, IPUMAXLUMPBYTES}}
  );

  poplar::program::Sequence R_Init_prog({
    poplar::program::Execute(R_InitTextureOrSans_CS),
    poplar::program::Copy(miscValuesStream, m_miscValuesBuf),
    poplar::program::Repeat(2, poplar::program::Sequence({ // <- number of R_Init_CS steps
      poplar::program::Execute(R_Init_CS),
      poplar::program::Call(requestLumpFromHost, {m_lumpNum[0]}, {lumpBuf}),
    })),
  });

  // ---------------- G_Ticker --------------//


  poplar::ComputeSet G_Ticker_CS = m_ipuGraph.addComputeSet("G_Ticker_CS");
  for (int renderTile = 0; renderTile < IPUNUMRENDERTILES; ++renderTile) {
    vtx = m_ipuGraph.addVertex(G_Ticker_CS, "G_Ticker_Vertex", {{"miscValues", m_miscValuesBuf}});
    m_ipuGraph.setTileMapping(vtx, renderTile + IPUFIRSTRENDERTILE);
    m_ipuGraph.setPerfEstimate(vtx, 100);
  }
  
  poplar::program::Sequence G_Ticker_prog({
      poplar::program::Copy(miscValuesStream, m_miscValuesBuf),
      poplar::program::Execute(G_Ticker_CS),
  });


  // ---------------- G_Responder --------------//


  poplar::ComputeSet G_Responder_CS = m_ipuGraph.addComputeSet("G_Responder_CS");
  for (int renderTile = 0; renderTile < IPUNUMRENDERTILES; ++renderTile) {
    vtx = m_ipuGraph.addVertex(G_Responder_CS, "G_Responder_Vertex", {{"miscValues", m_miscValuesBuf}});
    m_ipuGraph.setTileMapping(vtx, renderTile + IPUFIRSTRENDERTILE);
    m_ipuGraph.setPerfEstimate(vtx, 100);
  }
  
  poplar::program::Sequence G_Responder_prog({
      poplar::program::Copy(miscValuesStream, m_miscValuesBuf),
      poplar::program::Execute(G_Responder_CS),
  });


  // -------------- IPU Init setup (Happens before most CPU setup) ------------//

  // Initialising vtx that runs on every tile
  poplar::ComputeSet IPU_Init_CS = m_ipuGraph.addComputeSet("IPU_Init_CS");
  for (unsigned tile = 0; tile < totalTiles; ++tile) {
    vtx = m_ipuGraph.addVertex(IPU_Init_CS, "IPU_Init_Vertex");
    m_ipuGraph.setTileMapping(vtx, tile);
    m_ipuGraph.setPerfEstimate(vtx, 1000);
  }
  poplar::program::Sequence IPU_Init_Prog({
      poplar::program::Execute(IPU_Init_CS),
  });


  // -------------- IPU misc state setup (Happens after most CPU setup) ------------//

  poplar::ComputeSet IPU_MiscSetup_CS = m_ipuGraph.addComputeSet("IPU_MiscSetup_CS");
  for (int renderTile = 0; renderTile < IPUNUMRENDERTILES; ++renderTile) {
    vtx = m_ipuGraph.addVertex(IPU_MiscSetup_CS, "IPU_MiscSetup_Vertex", 
      {{"frame", ipuFrameSlices[renderTile]}});
    m_ipuGraph.setTileMapping(vtx, renderTile + IPUFIRSTRENDERTILE);
    m_ipuGraph.setPerfEstimate(vtx, 10000);
  }

  poplar::Tensor marknumSpriteBuf = m_ipuGraph.addVariable(poplar::UNSIGNED_CHAR, {(ulong)IPUAMMARKBUFSIZE}, "marknumSpriteBuf");
  m_ipuGraph.setTileMapping(marknumSpriteBuf, 0);
  auto marknumSpriteBufStream =
      m_ipuGraph.addHostToDeviceFIFO("marknumSpriteBuf-stream", poplar::UNSIGNED_CHAR, IPUAMMARKBUFSIZE);

  poplar::ComputeSet IPU_UnpackMarknumSprites_CS = m_ipuGraph.addComputeSet("IPU_UnpackMarknumSprites_CS");
  for (int renderTile = 0; renderTile < IPUNUMRENDERTILES; ++renderTile) {
    vtx = m_ipuGraph.addVertex(IPU_UnpackMarknumSprites_CS, "IPU_UnpackMarknumSprites_Vertex", 
    {{"buf", marknumSpriteBuf}});
    m_ipuGraph.setTileMapping(vtx, renderTile + IPUFIRSTRENDERTILE);
    m_ipuGraph.setPerfEstimate(vtx, IPUAMMARKBUFSIZE * 100);
  }
  
  poplar::program::Sequence IPU_MiscSetup_Prog({
      poplar::program::Execute(IPU_MiscSetup_CS),
      poplar::program::Copy(marknumSpriteBufStream, marknumSpriteBuf),
      poplar::program::Execute(IPU_UnpackMarknumSprites_CS),
  });


  // -------- R_RenderPlayerView_CS ------ //

  poplar::Tensor textureBuf = m_ipuGraph.addVariable(
    poplar::UNSIGNED_INT, 
    {IPUNUMTEXTURETILES, IPUTEXTURETILEBUFSIZE}, 
    "textureBuf");
  poplar::Tensor textureCache = m_ipuGraph.addVariable(
    poplar::UNSIGNED_INT, 
    { IPUNUMRENDERTILES, 
      IPUNUMTEXTURECACHELINES * IPUTEXTURECACHELINESIZE }, 
    "textureCache");

  poplar::ComputeSet R_RenderPlayerView_CS = m_ipuGraph.addComputeSet("R_RenderPlayerView_CS");
  for (int renderTile = 0; renderTile < IPUNUMRENDERTILES; ++renderTile) {
    int logicalTile = IPUFIRSTRENDERTILE + renderTile;
    vtx = m_ipuGraph.addVertex(R_RenderPlayerView_CS, "R_RenderPlayerView_Vertex", {
      {"frame", ipuFrameSlices[renderTile]}, 
      {"textureCache", textureCache[renderTile]},
      {"progBuf", progBuf[logicalTile]}, 
      {"commsBuf", commsBuf[logicalTile]},
      {"nonExecutableDummy", nonExecutableDummy[logicalTile]}, 
      {"miscValues", m_miscValuesBuf},
    });
    m_ipuGraph.setTileMapping(vtx, logicalTile);
    m_ipuGraph.setTileMapping(textureCache[renderTile], logicalTile);
    m_ipuGraph.setPerfEstimate(vtx, 10000000);
  }
  for (int textureTile = 0; textureTile < IPUNUMTEXTURETILES; ++textureTile) {
    int logicalTile = IPUFIRSTTEXTURETILE + textureTile;
    vtx =  m_ipuGraph.addVertex(R_RenderPlayerView_CS, "R_FulfilColumnRequests_Vertex", {
      {"dummy", nonExecutableDummy[logicalTile]}, 
      {"textureBuf", textureBuf[textureTile]},
      {"progBuf", progBuf[logicalTile]},
      {"commsBuf", commsBuf[logicalTile]},
    });
    m_ipuGraph.setTileMapping(vtx, logicalTile);
    m_ipuGraph.setPerfEstimate(vtx, 1000);
    m_ipuGraph.setTileMapping(textureBuf[textureTile], textureTile);
  }
  for (unsigned tile = IPUFIRSTTEXTURETILE + IPUNUMTEXTURETILES; tile < totalTiles; ++tile) {
    vtx = m_ipuGraph.addVertex(R_RenderPlayerView_CS, "R_Sans_Vertex", {
      {"dummy", nonExecutableDummy[tile]},
      {"progBuf", progBuf[tile]},
      {"commsBuf", commsBuf[tile]},
    });
    m_ipuGraph.setTileMapping(vtx, tile);
  }
  // Cache line is used as the aggregation buffer, make sure it's big enough
  assert(IPUTEXTURECACHELINESIZE >= IPUNUMRENDERTILES); 

  poplar::program::Sequence R_RenderPlayerView_prog({
      poplar::program::Copy(miscValuesStream, m_miscValuesBuf),
      poplar::program::Copy(frameInStream, ipuFrame),
      poplar::program::Execute(R_RenderPlayerView_CS),
      poplar::program::Copy(ipuFrame, frameOutStream),
  });

  // -------- R_ExecuteSetViewSize_CS ------ //
  
  poplar::ComputeSet R_ExecuteSetViewSize_CS = m_ipuGraph.addComputeSet("R_ExecuteSetViewSize_CS");
  for (int renderTile = 0; renderTile < IPUNUMRENDERTILES; ++renderTile) {
    vtx = m_ipuGraph.addVertex(R_ExecuteSetViewSize_CS, "R_ExecuteSetViewSize_Vertex", {{"miscValues", m_miscValuesBuf}});
    m_ipuGraph.setTileMapping(vtx, renderTile + IPUFIRSTRENDERTILE);
    m_ipuGraph.setPerfEstimate(vtx, 100);
  }
  
  poplar::program::Sequence R_ExecuteSetViewSize_prog({
      poplar::program::Copy(miscValuesStream, m_miscValuesBuf),
      poplar::program::Execute(R_ExecuteSetViewSize_CS),
  });


  // ---------------- Final prog --------------//

  m_ipuEngine = std::make_unique<poplar::Engine>(std::move(poplar::Engine(
    m_ipuGraph, 
    {
      IPU_MiscSetup_Prog,
      G_DoLoadLevel_prog,
      G_Ticker_prog,
      G_Responder_prog,
      AM_Drawer_prog,
      R_RenderPlayerView_prog,
      R_ExecuteSetViewSize_prog,
      R_Init_prog,
      IPU_Init_Prog,
    },
    {{"opt.enableSkipSyncs", "false"}}
  )));

  m_ipuEngine->connectStream("miscValues-stream", m_miscValuesBuf_h);
  m_ipuEngine->connectStream("lumpNum-stream", &m_lumpNum_h);
  // Connect frame-instream/outstream later in run_IPU_MiscSetup because 
  // I_VideoBuffer is initialised quite late

  m_ipuEngine->connectStreamToCallback("lumpBuf-stream", [this](void* p) {
    IPU_LoadLumpForTransfer(m_lumpNum_h, (byte*) p);
  });
  m_ipuEngine->connectHostFunction(
    "requestLumpFromHost", 0, [](poplar::ArrayRef<const void *> inputs, poplar::ArrayRef<void *> outputs) {
      const int* _lumpNum = static_cast<const int *>(inputs[0]);
      unsigned char* _lumpBuf = static_cast<unsigned char *>(outputs[0]);
      IPU_LoadLumpForTransfer(*_lumpNum, _lumpBuf);
  });
  m_ipuEngine->connectStreamToCallback("marknumSpriteBuf-stream", [this](void* p) {
    IPU_Setup_PackMarkNums(p);
  });

  m_ipuEngine->load(m_ipuDevice);
}

// --- Internal interface from class IpuDoom to m_ipuEngine --- //

void IpuDoom::run_IPU_MiscSetup() {
  m_ipuEngine->connectStream("frame-instream", I_VideoBuffer); 
  m_ipuEngine->connectStream("frame-outstream", I_VideoBuffer); 
  m_ipuEngine->run(0);
}
void IpuDoom::run_G_DoLoadLevel() {
  IPU_G_LoadLevel_PackMiscValues(m_miscValuesBuf_h);
  m_ipuEngine->run(1);
}
void IpuDoom::run_G_Ticker() {
  IPU_G_Ticker_PackMiscValues(m_miscValuesBuf_h);
  m_ipuEngine->run(2);
}
void IpuDoom::run_G_Responder(G_Responder_MiscValues_t* src_buf) { 
  IPU_G_Responder_PackMiscValues(src_buf, m_miscValuesBuf_h);
  m_ipuEngine->run(3); 
}
void IpuDoom::run_AM_Drawer() {
  m_ipuEngine->run(4);
}
void IpuDoom::run_R_RenderPlayerView() {
  IPU_R_RenderPlayerView_PackMiscValues(m_miscValuesBuf_h);
  m_ipuEngine->run(5);
}
void IpuDoom::run_R_ExecuteSetViewSize() {
  IPU_R_ExecuteSetViewSize_PackMiscValues(m_miscValuesBuf_h);
  m_ipuEngine->run(6);
}
void IpuDoom::run_R_Init() {
  IPU_R_Init_PackMiscValues(m_miscValuesBuf_h);
  m_ipuEngine->run(7);
}
void IpuDoom::run_IPU_Init() {
  m_ipuEngine->run(8);
}

static std::unique_ptr<IpuDoom> ipuDoomInstance = nullptr;


// --- External interface from C to the singleton IpuDoom object --- //
extern "C" {
  void IPU_Init() { 
    ipuDoomInstance = std::make_unique<IpuDoom>(); 
    ipuDoomInstance->run_IPU_Init();
  }
  void IPU_MiscSetup() { ipuDoomInstance->run_IPU_MiscSetup(); }
  void IPU_AM_Drawer() { ipuDoomInstance->run_AM_Drawer(); }
  void IPU_R_RenderPlayerView() { ipuDoomInstance->run_R_RenderPlayerView(); }
  void IPU_G_DoLoadLevel() { ipuDoomInstance->run_G_DoLoadLevel(); }
  void IPU_G_Ticker() { ipuDoomInstance->run_G_Ticker(); }
  void IPU_G_Responder(G_Responder_MiscValues_t* buf) { ipuDoomInstance->run_G_Responder(buf); }
  void IPU_R_ExecuteSetViewSize() { ipuDoomInstance->run_R_ExecuteSetViewSize(); }
  void IPU_R_Init() { ipuDoomInstance->run_R_Init(); }
}
