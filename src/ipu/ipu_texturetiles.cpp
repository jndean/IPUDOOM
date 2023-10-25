
#include <poplar/TileConstants.hpp>
#include <poplar/Vertex.hpp>

#include "doomtype.h"

#include "ipu_interface.h"
#include "ipu_utils.h"
#include "ipu_texturetiles.h"
#include "../../xcom.hpp"


// Remeber! Copies of these vars exits independently on each tile
unsigned* tileLocalProgBuf;
unsigned* tileLocalCommsBuf;
unsigned* tileLocalTextureBuf;
const int* tileLocalTextureRange;
const int* tileLocalTextureOffsets;


//  -------- Components for the tiles that serve textures ------------ //

extern "C" 
__SUPER__ 
void IPU_R_InitTextureTile(unsigned* progBuf, int progBufSize) {
    // progBuf starts with a directory of 3 program offsets
    unsigned* progHead = progBuf + 3; 
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

    // Third program receives the `done`
    {
        progBuf[2] = progHead - progBuf;
        XCOMAssembler assembler;
        int aggrTile = XCOM_logical2physical(IPUFIRSTRENDERTILE);
        int sendCycle = XCOM_WORSTSENDDELAY;
        int muxCycle = sendCycle + XCOM_TimeToMux(aggrTile, __builtin_ipu_get_tile_id());
        assembler.addRecv(0, 1, aggrTile, muxCycle);
        progHead = assembler.assemble(progHead, progEnd - progHead);
        progHead++;
    }
}

extern "C" 
__SUPER__ 
void IPU_R_FulfilColumnRequest(unsigned* progBuf, unsigned char* textureBuf, unsigned* commsBuf) {
    // Start of buffer is a directory of programs
    auto recvProgram = &progBuf[progBuf[0]];
    auto sendProgram = &progBuf[progBuf[1]];
    auto aggrProgram = &progBuf[progBuf[2]];
    
    // for(; textureFetchCount < tmpRepeatCount; textureFetchCount++) {
    while (1) {

      XCOM_Execute(recvProgram, NULL, textureBuf);

      // Unpack received data
      unsigned textureNum = ((IPUColRequest_t*) textureBuf)->texture;
      unsigned colNum = ((IPUColRequest_t*) textureBuf)->column;
      
      byte c1 = (textureNum * 9) % 256;
      byte c2 = (c1 + 1) % 256;
      byte c3 = (c1 + 2) % 256;
      byte c4 = (c1 + 1) % 256;
      unsigned colour = c1 | (c2 << 8) | (c3 << 16) | (c4 << 24);
      for (int i = 0; i < IPUTEXTURECACHELINESIZE; i++) {
        ((unsigned*)textureBuf)[i] = colour;
      }
      XCOM_Execute(sendProgram, textureBuf, NULL);

      XCOM_Execute(aggrProgram, NULL, commsBuf);
      //   if (tileID == 35) printf ("Texture flag: %d\n", commsBuf[0]);
      if (commsBuf[0]) 
        return;
    }
}



//  -------- Components for the tiles that request textures ------------ //

static int textureTileLUT[IPUTEXTURETILESPERRENDERTILE];
static int muxInstructionOffset;

extern "C" 
__SUPER__ 
void IPU_R_InitColumnRequester(unsigned* progBuf, int progBufSize) {

    // Figure out which tiles to talk to
    int firstTextureTile = IPUFIRSTTEXTURETILE + (IPUTEXTURETILESPERRENDERTILE * (tileID - IPUFIRSTRENDERTILE));
    for (int i = 0; i < IPUTEXTURETILESPERRENDERTILE; ++i) {
        textureTileLUT[i] = XCOM_logical2physical(firstTextureTile + i);
    }

    // ProgBuf starts with directory of programs
    unsigned* progHead = progBuf + 3;
    unsigned* progEnd = &progBuf[progBufSize];
    int aggrTile = XCOM_logical2physical(IPUFIRSTRENDERTILE);
    XCOMAssembler assembler;
    
    // First Program: performs the column request and sends flags to aggrTile
    progBuf[0] = progHead - progBuf;
    int messageSize = sizeof(IPUColRequest_t) / sizeof(int);
    assembler.addSend(0, messageSize, XCOM_BROADCAST, XCOM_WORSTSENDDELAY);
    // Do the first step of the `done` flag aggregation
    if (__builtin_ipu_get_tile_id() == aggrTile) {
        for (int slot = 1; slot < IPUNUMRENDERTILES; ++slot) {
            int senderID = XCOM_logical2physical(IPUFIRSTRENDERTILE + slot);
            bool clearMux = slot == IPUNUMRENDERTILES - 1;
            int recvCycle = XCOM_WORSTRECVDELAY + XCOM_WORSTRECVDELAY + slot - 1;
            unsigned* recvAddr = (unsigned*)(slot * sizeof(unsigned));
            assembler.addRecv(recvAddr, 1, senderID, recvCycle, clearMux);
        }
    } else {
        int myTimeSlot = tileID - (IPUFIRSTRENDERTILE + 1);
        int direction = XCOM_EastOrWest(__builtin_ipu_get_tile_id(), aggrTile);
        int muxCycle = XCOM_WORSTRECVDELAY + XCOM_WORSTRECVDELAY + myTimeSlot;
        int sendCycle = muxCycle - XCOM_TimeToMux(__builtin_ipu_get_tile_id(), aggrTile);
        unsigned* addr = (unsigned*) sizeof(IPUColRequest_t);
        assembler.addSend(addr, 1, direction, sendCycle);
    }
    progHead = assembler.assemble(progHead, progEnd - progHead);
    progHead++;
    assembler.reset();

    // Second Proggram: Receives the request response
    progBuf[1] = progHead - progBuf;
    assembler.addRecv(0, IPUTEXTURECACHELINESIZE, 0, XCOM_WORSTRECVDELAY);
    unsigned* newProgHead = assembler.assemble(progHead, progEnd - progHead);
    // Record the mux-setting instruction location for later live patching
    for (unsigned* inst = progHead; inst < newProgHead; ++inst) {
        if ((*inst & 0xfc003fffu) == 0x64000000u) {
            muxInstructionOffset = (inst - &progBuf[progBuf[1]]);
            break;
        }
    }
    progHead = newProgHead + 1; // +1 => don't overwrite the return statement
    assembler.reset();

    // Third Program: distributes the `done` flag
    progBuf[2] = progHead - progBuf;
    int sendCycle = XCOM_WORSTSENDDELAY;
    if (__builtin_ipu_get_tile_id() == aggrTile) {
        assembler.addSend(0, 1, XCOM_BROADCAST, sendCycle);
    } else {
        int muxCycle = sendCycle + XCOM_TimeToMux(aggrTile, __builtin_ipu_get_tile_id());
        assembler.addRecv(0, 1, aggrTile, muxCycle);
    }
    progHead = assembler.assemble(progHead, progEnd - progHead);
    progHead++;
    assembler.reset();
    
}

extern "C" 
__SUPER__ 
byte* IPU_R_RequestColumn(int texture, int column) {
    // progBuff starts with a program directory
    auto requestProg = &tileLocalProgBuf[tileLocalProgBuf[0]];
    auto receiveProg = &tileLocalProgBuf[tileLocalProgBuf[1]];
    auto aggregateProg = &tileLocalProgBuf[tileLocalProgBuf[2]];

    // Populate buffer with data to be exchanged
    IPUColRequest_t *request = (IPUColRequest_t*) tileLocalTextureBuf;
    request->texture = texture;
    request->column = column;
    *((unsigned*) &request[1]) = 0; // The Done Flag
    
    XCOM_Execute(
        requestProg,         // Prog
        tileLocalTextureBuf, // Read offset
        tileLocalCommsBuf    // Write offset
    );

    // aggrTile aggregates
    if (tileID == IPUFIRSTRENDERTILE) {
        tileLocalCommsBuf[0] = 0;
    }

    int textureSourceTile = 0;
    XCOM_PatchMuxAndExecute(
        receiveProg,                      // Prog
        NULL,                             // Read offset
        tileLocalTextureBuf,              // Write offset
        muxInstructionOffset,             // Patch offset
        textureTileLUT[textureSourceTile] // Mux value
    );
    
    XCOM_Execute(
        aggregateProg,     // Prog
        tileLocalCommsBuf, // Read offset
        tileLocalCommsBuf  // Write offset
    );

    return (byte*) tileLocalTextureBuf;
}

extern "C" 
__SUPER__ 
void IPU_R_RenderTileDone() {
    // progBuff starts with a program directory
    auto requestProg = &tileLocalProgBuf[tileLocalProgBuf[0]];
    auto aggregateProg = &tileLocalProgBuf[tileLocalProgBuf[2]];

    while (1) {

        // Populate buffer with data to be exchanged
        IPUColRequest_t *request = (IPUColRequest_t*) tileLocalTextureBuf;
        request->texture = 0xffffffff;
        request->column = 0xffffffff;
        *((unsigned*) &request[1]) = 1; // The `done` Flag

        XCOM_Execute(
            requestProg,         // Prog
            tileLocalTextureBuf, // Read offset
            tileLocalCommsBuf    // Write offset
        );

        // aggrTile aggregates `done` flags
        unsigned aggr = true;
        if (tileID == IPUFIRSTRENDERTILE) {
            for (int i = 1; i < IPUNUMRENDERTILES; i++) {
                aggr &= tileLocalCommsBuf[i];
            }
        }
        tileLocalCommsBuf[0] = aggr;

        // Don't bother receiving  the response
        asm volatile(R"(
            sans 0
            sync 0x1
        )");
        
        XCOM_Execute(
            aggregateProg,     // Prog
            tileLocalCommsBuf, // Read offset
            tileLocalCommsBuf  // Write offset
        );
        
        if (tileLocalCommsBuf[0]) // Done flag
            return;
    }
}



//  -------- Components for the sans tiles ------------ //


extern "C" 
__SUPER__ 
void IPU_R_Sans(unsigned* progBuf, unsigned* commsBuf) {
    // Start of buffer is a directory of programs
    auto recvProgram = &progBuf[progBuf[0]];
    auto sendProgram = &progBuf[progBuf[1]];
    auto aggrProgram = &progBuf[progBuf[2]];

    while (1) {
      asm volatile(R"(
          sans 1
          sync 0x1
      )");

      XCOM_Execute(aggrProgram, commsBuf, commsBuf);

      if (commsBuf[0])
        return;
    }
}



/*
    THE PLAN

Prog 0 {
    Render tiles send request from textureBuf[0:1], then 
            - aggrTile receives flags into comms buf
            - the rest send done flag from textureBuf[2]
    Texture tiles receive request into texture buf
    Sans tiles sans x 2
}

Render tiles patch mux address
    aggrTile aggregates flags into commsBuf[0]
Texture tiles fetch coluns into textureBuf
Sans tiles do nothing

Prog 1 {
    Render tiles receive cols into textureBuf
    TextureTiles send cols from texture buf
    Sans tiles do nothing (no sans)
}


Prog 2 {
    Render tiles receive doneFlag
        AggrTile sends done flag
    TextureTiles receive done flag
    Sans tiles receive done flag
}



*/