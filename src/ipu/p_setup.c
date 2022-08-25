
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
#define MAXVERTEXES (1024)
int numvertexes;
vertex_t vertexes[MAXVERTEXES];

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
  // JOSEF: assumer endianness of SHORT is fine

  // Read the header

  bmaporgx = blockmaplump[0] << FRACBITS;
  bmaporgy = blockmaplump[1] << FRACBITS;
  bmapwidth = blockmaplump[2];
  bmapheight = blockmaplump[3];


  ipuprint("[IPU] bmaporgx ");
  ipuprintnum(bmaporgx);
  ipuprint(", bmaporgy ");
  ipuprintnum(bmaporgy);
  ipuprint(", bmapwidth ");
  ipuprintnum(bmapwidth);
  ipuprint(", bmapheight ");
  ipuprintnum(bmapheight);
  ipuprint("\n");

  // Clear out mobj chains

  count = sizeof(*blocklinks) * bmapwidth * bmapheight;
  blocklinks = IPU_level_malloc(count);
  memset(blocklinks, 0, count);
}