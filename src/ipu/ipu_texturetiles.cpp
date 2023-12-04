
#include <poplar/TileConstants.hpp>
#include <poplar/Vertex.hpp>

extern "C" {
    #include "doomtype.h"
    #include "r_data.h"
    #include "r_draw.h"
    #include "r_main.h"
    #include "r_segs.h"

    #include "ipu_interface.h"
    #include "ipu_utils.h"
    #include "ipu_texturetiles.h"
}

#include "../../xcom.hpp"


// Remeber! Copies of these vars exits independently on each tile
unsigned* tileLocalProgBuf;
unsigned* tileLocalCommsBuf;
unsigned* tileLocalTextureBuf;
const int* tileLocalTextureRange;
const int* tileLocalTextureOffsets;



//  -------- Components for the tiles that serve textures ------------ //

static pixel_t colourMap_TT[COLOURMAPSIZE];
static pixel_t* scalelight_TT[LIGHTLEVELS][MAXLIGHTSCALE];
static pixel_t* zlight_TT[LIGHTLEVELS][MAXLIGHTZ];

extern "C" 
__SUPER__
void IPU_R_InitTextureTile(unsigned* progBuf, int progBufSize, const pixel_t* colourMapBuf) {
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
        int messageSize = sizeof(IPUTextureRequest_t) / sizeof(int);
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

    // Next, precompute light scales from the colourmap. This normally happens in R_ExecuteSetViewSize
    memcpy(colourMap_TT, colourMapBuf, sizeof(colourMap_TT));
    // Firstly do scalelight_TT, which is used for cols
    for (int i = 0; i < LIGHTLEVELS; i++) {
        int startmap = ((LIGHTLEVELS - 1 - i) * 2) * NUMCOLORMAPS / LIGHTLEVELS;
        for (int j = 0; j < MAXLIGHTSCALE; j++) {
          int level = startmap - j / DISTMAP;

          if (level < 0)
            level = 0;

          if (level >= NUMCOLORMAPS)
            level = NUMCOLORMAPS - 1;

          scalelight_TT[i][j] = colourMap_TT + level * 256;
        }
    }
    // Next do zlight_TT, which is used for spans
    for (int i = 0; i < LIGHTLEVELS; i++) {
        int startmap = ((LIGHTLEVELS - 1 - i) * 2) * NUMCOLORMAPS / LIGHTLEVELS;
        for (int j = 0; j < MAXLIGHTZ; j++) {
            int scale = FixedDiv((SCREENWIDTH / 2 * FRACUNIT), (j + 1) << LIGHTZSHIFT);
            scale >>= LIGHTSCALESHIFT;
            int level = startmap - scale / DISTMAP;

            if (level < 0)
                level = 0;

            if (level >= NUMCOLORMAPS)
                level = NUMCOLORMAPS - 1;

            zlight_TT[i][j] = colourMap_TT + level * 256;
        }
    }
}

extern "C" 
__SUPER__ 
void IPU_R_FulfilColumnRequest(unsigned* progBuf, unsigned char* textureBuf, unsigned* commsBuf) {
    // Start of buffer is a directory of programs
    auto recvProgram = &progBuf[progBuf[0]];
    auto sendProgram = &progBuf[progBuf[1]];
    auto aggrProgram = &progBuf[progBuf[2]];

    // !! TMP !! need seperate send buffer for lightscaled output to be sent from, currently use end of textureBuf
    pixel_t* sendBuf = &textureBuf[IPUTEXTURETILEBUFSIZE - (IPUTEXTURECACHELINESIZE * sizeof(int))];
    IPUTextureRequest_t* incomingRequest = (IPUTextureRequest_t*) textureBuf; // Err...? Overwriting texture? replace this with sendBuf?

    while (1) {
      // Receive a request
      XCOM_Execute(recvProgram, NULL, incomingRequest);

      unsigned textureNum = incomingRequest->texture;
      // Check for dummy request
      if (textureNum != 0xffffffff) { 

        unsigned isSpanRequest = textureNum & IPUTEXREQUESTISSPAN;
        textureNum ^= isSpanRequest; // Actual texture number

        // Check if the texture lives on this tile
        if (textureNum >= tileLocalTextureRange[0] && 
            textureNum < tileLocalTextureRange[1]) {

            // Find the start of the texture
            byte* texture = &textureBuf[tileLocalTextureOffsets[textureNum]];

            if (isSpanRequest) {
                // Is a Span request (batch)
                
                pixel_t *dest = sendBuf;
                for (int i = 0; i < incomingRequest->numSpanRequests; ++i) {
                    IPUSpanRequest_t* req = &incomingRequest->spanRequests[i];
                    unsigned position = req->position;
                    unsigned step = req->step;
                    unsigned count = req->count;
                    unsigned lightNum = req->lightNum;
                    unsigned lightScale = req->lightScale;
                    ds_colormap = zlight_TT[lightNum][lightScale];
                    // Do the render loop for this line of pixels
                    do {
                        // Does 6 levels of indentation count as code smell?
                        unsigned ytemp = (position >> 4) & 0x0fc0;
                        unsigned xtemp = (position >> 26);
                        int spot = xtemp | ytemp;
                        *dest++ = ds_colormap[texture[spot]];
                        position += step;
                    } while (count--);
                }

            } else { 
                // Is a Column request
                unsigned columnOffset = incomingRequest->colRequest.columnOffset;
                unsigned lightNum = incomingRequest->colRequest.lightNum;
                unsigned lightScale = incomingRequest->colRequest.lightScale;
                byte* src = &texture[columnOffset];
                pixel_t* dc_colourmap = scalelight_TT[lightNum][lightScale];
                for (int i = 0; i < IPUTEXTURECACHELINESIZE * sizeof(int); ++i) {
                    sendBuf[i] = dc_colourmap[src[i]];
                }
            }
        }
      }
      
      // Send response to request
      XCOM_Execute(sendProgram, sendBuf, NULL);

      // Find out if we're done
      XCOM_Execute(aggrProgram, NULL, commsBuf);
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
    int messageSize = sizeof(IPUTextureRequest_t) / sizeof(int);
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
        unsigned* addr = (unsigned*) sizeof(IPUTextureRequest_t);
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

static
__SUPER__ 
byte* renderTileExchange(int textureNum) {
    // progBuff starts with a program directory
    auto requestProg = &tileLocalProgBuf[tileLocalProgBuf[0]];
    auto receiveProg = &tileLocalProgBuf[tileLocalProgBuf[1]];
    auto aggregateProg = &tileLocalProgBuf[tileLocalProgBuf[2]];

    // Figure out which texture tile is going to respond
    int textureSourceTile;
    for (textureSourceTile = 0; textureSourceTile < IPUTEXTURETILESPERRENDERTILE; ++textureSourceTile) {
        if (textureNum >= tileLocalTextureRange[textureSourceTile] && 
            textureNum < tileLocalTextureRange[textureSourceTile + 1]) {
        break;
        }
    }

    // Request will be populated by the caller, but we follow it up by setting the 'done' flag to 0
    IPUTextureRequest_t *request = (IPUTextureRequest_t*) tileLocalTextureBuf;
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
byte* IPU_R_RequestColumn(int texture, int column) {

    // Populate buffer with data to be exchanged
    IPUTextureRequest_t *request = (IPUTextureRequest_t*) tileLocalTextureBuf;
    request->texture = texture;
    request->colRequest.columnOffset = (column & texturewidthmask[texture]) * (textureheight[texture] >> FRACBITS);
    request->colRequest.lightNum = lightnum;
    request->colRequest.lightScale = walllightindex;

    return renderTileExchange(texture);
}

extern "C" 
__SUPER__ 
byte* IPU_R_RequestSpanBatch(IPUTextureRequest_t *requestBatch) {

    // Populate buffer with data to be exchanged
    IPUTextureRequest_t* sendBuf = (IPUTextureRequest_t*) tileLocalTextureBuf;
    memcpy(sendBuf, requestBatch, sizeof(*requestBatch));
    sendBuf->texture |= IPUTEXREQUESTISSPAN; // Flag saying this is a span request not a col request

    return renderTileExchange(requestBatch->texture);
}


// extern "C" 
// __SUPER__ 
// byte* IPU_R_RequestSpan() {

//     // Populate buffer with data to be exchanged
//     IPUTextureRequest_t *request = (IPUTextureRequest_t*) tileLocalTextureBuf;
//     request->texture = texture | IS_SPAN; // Set the request type to SPAN instead of COLUMN
//     // request->spanRequest. = (column & texturewidthmask[texture]) * (textureheight[texture] >> FRACBITS);
//     // request->colRequest.lightNum = lightnum;
//     // request->colRequest.lightScale = walllightindex;

//     // Figure out which texture tile is going to respond
//     // int textureSourceTile;
//     // for (textureSourceTile = 0; textureSourceTile < IPUTEXTURETILESPERRENDERTILE; ++textureSourceTile) {
//     //     if (texture >= tileLocalTextureRange[textureSourceTile] && 
//     //         texture < tileLocalTextureRange[textureSourceTile + 1]) {
//     //     break;
//     //     }
//     // }

//     return renderTileExchange(textureSourceTile);
// }

extern "C" 
__SUPER__ 
void IPU_R_RenderTileDone() {

    // First flush the last possibly incomplete spanRequest batch
    IPURequest_R_DrawSpan_FulfillBatch();

    // progBuff starts with a program directory
    auto requestProg = &tileLocalProgBuf[tileLocalProgBuf[0]];
    auto aggregateProg = &tileLocalProgBuf[tileLocalProgBuf[2]];

    while (1) {

        // Populate buffer with data to be exchanged
        IPUTextureRequest_t *request = (IPUTextureRequest_t*) tileLocalTextureBuf;
        request->texture = 0xffffffff;
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
void IPU_R_InitSansTile(unsigned* progBuf, int progBufSize) {
    // progBuf starts with a directory of 3 program offsets
    unsigned* progHead = progBuf + 1; 
    unsigned* progEnd = &progBuf[progBufSize];

    // First program receives the `done` flag
    progBuf[0] = progHead - progBuf;
    XCOMAssembler assembler;
    int aggrTile = XCOM_logical2physical(IPUFIRSTRENDERTILE);
    int sendCycle = XCOM_WORSTSENDDELAY;
    int muxCycle = sendCycle + XCOM_TimeToMux(aggrTile, __builtin_ipu_get_tile_id());
    assembler.addRecv(0, 1, aggrTile, muxCycle);
    progHead = assembler.assemble(progHead, progEnd - progHead);
    progHead++;
}

extern "C" 
__SUPER__ 
void IPU_R_Sans(unsigned* progBuf, unsigned* commsBuf) {
    // Start of buffer is a directory of programs
    auto aggrProgram = &progBuf[progBuf[0]];

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