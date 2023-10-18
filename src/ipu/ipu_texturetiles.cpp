
#include <poplar/TileConstants.hpp>
#include <poplar/Vertex.hpp>

#include "doomtype.h"

#include "ipu_interface.h"
#include "ipu_utils.h"
#include "ipu_texturetiles.h"
#include "../../xcom.hpp"


// Remeber! Copies of these vars exits independently on each tile
unsigned* tileLocalProgBuf;
unsigned* tileLocalTextureBuf;
int textureFetchCount = 0;


//  -------- Components for the tiles that serve textures ------------ //

__SUPER__ 
void IPU_R_InitTextureTile(unsigned* progBuf, int progBufSize) {
    // progBuf starts with a directory of 2 program offsets
    unsigned* progHead = progBuf + 2; 
    unsigned* progEnd = &progBuf[progBufSize];

    // Figure out which tiles are involved
    int renderTile = XCOM_logical2physical((tileID - IPUFIRSTTEXTURETILE) / IPUTEXTURETILESPERRENDERTILE);

    // First program receives the request
    {
        progBuf[0] = progHead - progBuf;
        XCOMAssembler assembler;
        int sendCycle = XCOM_WORSTSENDDELAY;
        int muxCycle = sendCycle + XCOM_TimeToMux(renderTile, __builtin_ipu_get_tile_id());
        int messageSize = sizeof(IPUColRequest_t) / sizeof(int);
        assembler.addRecv(0, messageSize, renderTile, muxCycle);
        progHead = assembler.assemble(progHead, progEnd - progHead);
        progHead++; // This program returns control flow, so don't override the `br $m10`
    }

    // Second program sends the response
    {
        progBuf[1] = progHead - progBuf;
        XCOMAssembler assembler;
        int messageSize = IPUTEXTURECACHELINESIZE;
        int recvCycle = XCOM_WORSTRECVDELAY;
        int sendCycle = recvCycle - XCOM_TimeToMux(__builtin_ipu_get_tile_id(), renderTile);
        int direction = XCOM_EastOrWest(__builtin_ipu_get_tile_id(), renderTile);
        assembler.addSend(0, messageSize, direction, sendCycle);
        progHead = assembler.assemble(progHead, progEnd - progHead);
        progHead++;
    }
}

__SUPER__ 
void IPU_R_FulfilColumnRequest(unsigned* progBuf, unsigned* textureBuf) {
    // Start of buffer is a directory of programs
    auto recvProgram = &progBuf[progBuf[0]];
    auto sendProgram = &progBuf[progBuf[1]];
    
    for(; textureFetchCount < 6; textureFetchCount++) {

      XCOM_Execute(recvProgram, NULL, textureBuf);

      for (int i = 0; i < IPUTEXTURECACHELINESIZE; i++) {
        textureBuf[i] = 0x20202020;
      }
      XCOM_Execute(sendProgram, textureBuf, NULL);
    }
}



//  -------- Components for the tiles that request textures ------------ //

#define NUMCACHECOLS (20)
#define CACHECOLSIZE (128)
static byte columnCache[NUMCACHECOLS][CACHECOLSIZE];

static int textureTileLUT[IPUTEXTURETILESPERRENDERTILE];
static int muxInstructionOffset;

extern "C" 
__SUPER__ 
void IPU_R_InitColumnRequester(unsigned* progBuf, int progBufSize) {
    // TMP colours
    for (int i = 0; i < NUMCACHECOLS; ++i) {
        unsigned* col = (unsigned*) columnCache[i];
        byte c1 = (i * 8) % 256;
        byte c2 = (c1 + 1) % 256;
        byte c3 = (c1 + 2) % 256;
        byte c4 = (c1 + 1) % 256;
        unsigned packedColour = c1 | (c2 << 8) | (c3 << 16) | (c4 << 24);
        for (int j = 0; j < CACHECOLSIZE / 4; j++) {
            col[j] = packedColour;
        }
    }

    // Figure out which tiles to talk to
    int firstTextureTile = IPUFIRSTTEXTURETILE + (IPUTEXTURETILESPERRENDERTILE * (tileID - IPUFIRSTRENDERTILE));
    for (int i = 0; i < IPUTEXTURETILESPERRENDERTILE; ++i) {
        textureTileLUT[i] = XCOM_logical2physical(firstTextureTile + i);
    }

    // Prog Buf starts with directory of programs
    unsigned* progHead = progBuf + 3;
    unsigned* progEnd = &progBuf[progBufSize];
    {
        progBuf[0] = progHead - progBuf;
        XCOMAssembler assembler;
        int messageSize = sizeof(IPUColRequest_t) / sizeof(int);
        assembler.addSend(0, messageSize, XCOM_BROADCAST, XCOM_WORSTSENDDELAY);
        progHead = assembler.assemble(progHead, progEnd - progHead);
    } 
    {
        XCOMAssembler assembler;
        int messageSize = IPUTEXTURECACHELINESIZE;
        assembler.addRecv(0, messageSize, 0, XCOM_WORSTRECVDELAY);
        unsigned* newProgHead = assembler.assemble(progHead, progEnd - progHead);
        for (unsigned* inst = progHead; inst < newProgHead; ++inst) {
            if ((*inst & 0xfc003fffu) == 0x64000000u) {
                muxInstructionOffset = (inst - &progBuf[progBuf[0]]);
                break;
            }
        }
        progHead = newProgHead;
    } 
    // Third program sends finished flag to flag reducer
    // {
    //     progBuf[2] = progHead - progBuf;
    //     XCOMAssembler assembler;
    //     int messageSize = 1;
    //     int recvCycle = XCOM_WORSTRECVDELAY;
    //     int sendCycle = recvCycle - XCOM_TimeToMux(__builtin_ipu_get_tile_id(), renderTile);
    //     int direction = XCOM_EastOrWest(__builtin_ipu_get_tile_id(), renderTile);
    //     assembler.addSend(0, messageSize, direction, sendCycle);
    //     progHead = assembler.assemble(progHead, progEnd - progHead);
    //     progHead++;
    // }
    
}


extern "C" 
__SUPER__ 
byte* IPU_R_RequestColumn(int texture, int column) {
    // progBuff starts with a program directory
    auto requestProg = &tileLocalProgBuf[tileLocalProgBuf[0]];

    if (textureFetchCount++ < 6) {
        int sourceTile = 0;
        XCOM_PatchMuxAndExecute(
            requestProg,          // Prog
            tileLocalTextureBuf,       // Read offset
            tileLocalTextureBuf,       // Write offset
            muxInstructionOffset,      // Patch offset
            textureTileLUT[sourceTile] // Mux value
        );

        return (byte*) tileLocalTextureBuf;
    }
    return columnCache[texture % NUMCACHECOLS];
}



//  -------- Components for the sans tiles ------------ //


extern "C"
__SUPER__ 
void IPU_R_InitSansTile(unsigned* progBuf, int progBufSize) {
    // TODO
    XCOMAssembler assembler;
    int srcTile = 0;
    int sendCycle = XCOM_WORSTSENDDELAY;
    int muxCycle = sendCycle + XCOM_TimeToMux(srcTile, __builtin_ipu_get_tile_id());
    int messageSize = sizeof(IPUColRequest_t) / sizeof(int);
    assembler.addRecv(0, messageSize, 0, muxCycle);
    assembler.assemble(progBuf, progBufSize);
}

extern "C" 
__SUPER__ 
void IPU_R_Sans(unsigned* progBuf, int progBufSize) {
    (void) progBuf;
    (void) progBufSize;

    for(; textureFetchCount < 6; textureFetchCount++) {
      asm volatile(R"(
          sans 1
          sync 0x1
      )");
    }
}

