#include <string.h>

#include "r_defs.h"
#include "r_state.h"
#include "../ipu_transfer.h"




void IPU_UnpackVertexes(const unsigned char* buf) {
    IpuPackedLevel_t* pack = (IpuPackedLevel_t*) buf;
    uint32_t pos = 0;

    numvertexes = *(uint32_t*)buf;
    
    mapvertex_t* ml = (mapvertex_t*)(&buf[4]);
    vertex_t *li not done here;
    for (int i = 0; i < numvertex)
    vertexes = (vertex_t*) &pack->data[pos];
    pos += numvertexes * sizeof(vertex_t);

    numlines = pack->numlines;
    lines = (line_t*) &pack->data[pos];
    pos += numlines * sizeof(line_t);

    // for (int i=0; i <)
    // LATER: repoint other contents of lines


}
