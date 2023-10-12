
#include "doomtype.h"

#include "ipu_utils.h"
#include "ipu_texturetiles.h"


#define NUMCACHECOLS (20)
#define CACHECOLSIZE (128)
static byte columnCache[NUMCACHECOLS][CACHECOLSIZE];


extern "C" __SUPER__ void IPU_R_InitColumnRequester(void) {
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
}


extern "C" __SUPER__ byte* IPU_R_RequestColumn(int texture, int column) {
    return columnCache[texture % NUMCACHECOLS];
}