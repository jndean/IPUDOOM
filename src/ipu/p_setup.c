
#include "d_mode.h"
#include "doomstat.h"
#include "r_defs.h"
#include "p_local.h"

#include "ipu_malloc.h"
#include "ipu_transfer.h"
#include "ipu_print.h"


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int numvertexes;
vertex_t *vertexes;

int numsegs;
seg_t *segs;

int numsectors;
sector_t *sectors;

int numsubsectors;
subsector_t *subsectors;

int numnodes;
node_t *nodes;

int numlines;
line_t *lines;

int numsides;
side_t *sides;

static int totallines;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int bmapwidth;
int bmapheight;  // size in mapblocks
short *blockmap; // int for larger maps
// offsets in blockmap are from here
short *blockmaplump;
// origin of block map
fixed_t bmaporgx;
fixed_t bmaporgy;
// for thing chains
mobj_t **blocklinks;

// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
byte *rejectmatrix;

// Maintain single and multi player starting spots.
#define MAX_DEATHMATCH_STARTS 10

mapthing_t deathmatchstarts[MAX_DEATHMATCH_STARTS];
mapthing_t *deathmatch_p;
mapthing_t playerstarts[MAXPLAYERS];



// P_LoadVertexes
//
void P_LoadVertexes(const unsigned char *buf) {
  byte *data;
  int i;
  mapvertex_t *ml;
  vertex_t *li;

  // Determine number of lumps:
  //  total lump length / vertex record length.
  int lumplen = ((int*)buf)[0];
  numvertexes = lumplen / sizeof(mapvertex_t);

  // Allocate zone memory for buffer.
  vertexes = IPU_level_malloc(numvertexes * sizeof(vertex_t));

  // Load data into cache.
  // JOSEF: data = W_CacheLumpNum(lump, PU_STATIC);

  ml = (mapvertex_t *)(&buf[sizeof(int)]);
  li = vertexes;

  // Copy and convert vertex coordinates,
  // internal representation as fixed.
  for (i = 0; i < numvertexes; i++, li++, ml++) {
    li->x = ml->x << FRACBITS;
    li->y = ml->y << FRACBITS;
  }

  ipuprint("numvertexes: ");
  ipuprintnum(numvertexes);
  ipuprint(", 1x: ");
  ipuprintnum(vertexes[0].x);
  ipuprint(", -1y: ");
  ipuprintnum(vertexes[numvertexes-1].y);
  ipuprint("\n");

  // Free buffer memory.
  // JOSEF: W_ReleaseLumpNum(lump);

  requestedlumpnum = gamelumpnum + ML_SECTORS;
}


//
// P_SetupLevel
//
void P_SetupLevel_pt0(void) {
  int i;

  /* LATER
  totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
  wminfo.partime = 180;
  for (i = 0; i < MAXPLAYERS; i++) {
    players[i].killcount = players[i].secretcount = players[i].itemcount = 0;
  }

  // Initial height of PointOfView
  // will be set by player think.
  players[consoleplayer].viewz = 1;

  // Make sure all sounds are stopped before Z_FreeTags.
  S_Start();
  */

  // JOSEF, replaced; Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);
  IPU_level_free(); // LATER: free other tags, here not just level

  /* LATER
  // UNUSED W_Profile ();
  P_InitThinkers();

  // if working with a devlopment map, reload it
  // W_Reload(); // JOSEF: disabled

  // find map name
  lumpname[0] = 'E';
  lumpname[1] = '0' + gameepisode;
  lumpname[2] = 'M';
  lumpname[3] = '0' + gamemap;
  lumpname[4] = '\0';

  lumpnum = W_GetNumForName(lumpname);

  */
  leveltime = 0;

  reset_ipuprint();
  ipuprint("Map starts at lump "); ipuprintnum(gamelumpnum); ipuprint("\n");

  // JOSEF: Lumpnum for P_LoadBlockMap
  requestedlumpnum = gamelumpnum + ML_BLOCKMAP;
  return;

  // note: most of this ordering is important
  /* LATER
  P_LoadBlockMap(lumpnum + ML_BLOCKMAP);
  P_LoadVertexes(lumpnum + ML_VERTEXES);
  P_LoadSectors(lumpnum + ML_SECTORS);
  P_LoadSideDefs(lumpnum + ML_SIDEDEFS);

  P_LoadLineDefs(lumpnum + ML_LINEDEFS);
  P_LoadSubsectors(lumpnum + ML_SSECTORS);
  P_LoadNodes(lumpnum + ML_NODES);
  P_LoadSegs(lumpnum + ML_SEGS);
  */

  /* LATER
  P_GroupLines();
  P_LoadReject(lumpnum + ML_REJECT);

  bodyqueslot = 0;
  deathmatch_p = deathmatchstarts;
  P_LoadThings(lumpnum + ML_THINGS);

  // if deathmatch, randomly spawn the active players
  if (deathmatch) {
    for (i = 0; i < MAXPLAYERS; i++)
      if (playeringame[i]) {
        players[i].mo = NULL;
        G_DeathMatchSpawnPlayer(i);
      }
  }

  // clear special respawning que
  iquehead = iquetail = 0;

  // set up world state
  P_SpawnSpecials();

  // build subsector connect matrix
  //	UNUSED P_ConnectSubsectors ();

  // preload graphics
  if (precache)
    R_PrecacheLevel();

  */
}



//
// P_LoadBlockMap
//
void P_LoadBlockMap(const unsigned char *buf) {
  int i;
  int count;
  int lumplen;

  lumplen = ((int*)buf)[0];
  blockmaplump = IPU_level_malloc(lumplen);
  memcpy(blockmaplump, &buf[4], lumplen);
  blockmap = blockmaplump + 4;

  // Swap all short integers to native byte ordering.
  // JOSEF: assume endianness of SHORT is fine

  // Read the header

  bmaporgx = blockmaplump[0] << FRACBITS;
  bmaporgy = blockmaplump[1] << FRACBITS;
  bmapwidth = blockmaplump[2];
  bmapheight = blockmaplump[3];

  // Clear out mobj chains

  count = sizeof(*blocklinks) * bmapwidth * bmapheight;
  blocklinks = IPU_level_malloc(count);
  memset(blocklinks, 0, count);

  // JOSEF: next lump to load
  requestedlumpnum = gamelumpnum + ML_VERTEXES;
}