
#include "d_mode.h"
#include "doomstat.h"
#include "i_swap.h"
#include "m_bbox.h"
#include "r_defs.h"
#include "p_local.h"

#include "ipu_malloc.h"
#include "ipu_transfer.h"
#include "ipu_print.h"
#include "print.h"


void P_SpawnMapThing(mapthing_t *mthing);

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


//
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
}

//
// P_LoadSectors
//
void P_LoadSectors(const unsigned char *buf) {
  byte *data;
  int i;
  mapsector_t *ms;
  sector_t *ss;

  int lumplen = ((int*)buf)[0];
  numsectors = lumplen / sizeof(mapsector_t);
  sectors = IPU_level_malloc(numsectors * sizeof(sector_t));
  memset(sectors, 0, numsectors * sizeof(sector_t));

  ms = (mapsector_t *)(&buf[sizeof(int)]);
  ss = sectors;
  for (i = 0; i < numsectors; i++, ss++, ms++) {
    ss->floorheight = SHORT(ms->floorheight) << FRACBITS;
    ss->ceilingheight = SHORT(ms->ceilingheight) << FRACBITS;
    // ss->floorpic = R_FlatNumForName(ms->floorpic); // LATER
    // ss->ceilingpic = R_FlatNumForName(ms->ceilingpic); // LATER
    ss->lightlevel = SHORT(ms->lightlevel);
    ss->special = SHORT(ms->special);
    ss->tag = SHORT(ms->tag);
    ss->thinglist = NULL;
  }
}


//
// P_LoadNodes
//
__attribute__((optnone))  // Workaround for https://graphcore.atlassian.net/browse/LLVM-330
void P_LoadNodes(const unsigned char *buf) {
  int i;
  int j;
  int k;
  mapnode_t *mn;
  node_t *no;

  int lumplen = ((int*)buf)[0];
  numnodes = lumplen / sizeof(mapnode_t);
  nodes = IPU_level_malloc(numnodes * sizeof(node_t));
  memset(nodes, 0, numnodes * sizeof(node_t));

  mn = (mapnode_t *)(&buf[sizeof(int)]);
  no = nodes;

  for (i = 0; i < numnodes; i++, no++, mn++) {
    no->x = SHORT(mn->x) << FRACBITS;
    no->y = SHORT(mn->y) << FRACBITS;
    no->dx = SHORT(mn->dx) << FRACBITS;
    no->dy = SHORT(mn->dy) << FRACBITS;
    for (j = 0; j < 2; j++) {
      no->children[j] = SHORT(mn->children[j]);
      for (k = 0; k < 4; k++) {
        no->bbox[j][k] = SHORT(mn->bbox[j][k]) << FRACBITS;
      }
    }
  }

}

//
// P_LoadThings
//
void P_LoadThings(const unsigned char *buf) {
  byte *data;
  int i;
  mapthing_t *mt;
  mapthing_t spawnthing;
  int numthings;
  boolean spawn;

  int lumplen = ((int*)buf)[0];
  numthings = lumplen / sizeof(mapthing_t);

  mt = (mapthing_t *)(&buf[sizeof(int)]);
  for (i = 0; i < numthings; i++, mt++) {
    spawn = true;

    // Do not spawn cool, new monsters if !commercial
    if (gamemode != commercial) {
      switch (SHORT(mt->type)) {
      case 68: // Arachnotron
      case 64: // Archvile
      case 88: // Boss Brain
      case 89: // Boss Shooter
      case 69: // Hell Knight
      case 67: // Mancubus
      case 71: // Pain Elemental
      case 65: // Former Human Commando
      case 66: // Revenant
      case 84: // Wolf SS
        spawn = false;
        break;
      }
    }
    if (spawn == false)
      break;

    // Do spawn all other stuff.
    spawnthing.x = SHORT(mt->x);
    spawnthing.y = SHORT(mt->y);
    spawnthing.angle = SHORT(mt->angle);
    spawnthing.type = SHORT(mt->type);
    spawnthing.options = SHORT(mt->options);

    P_SpawnMapThing(&spawnthing); // TODO
  }

}

//
// P_LoadSideDefs
//
void P_LoadSideDefs(const unsigned char *buf) {
  byte *data;
  int i;
  mapsidedef_t *msd;
  side_t *sd;

  int lumplen = ((int*)buf)[0];
  numsides = lumplen / sizeof(mapsidedef_t);
  sides = IPU_level_malloc(numsides * sizeof(side_t));
  memset(sides, 0, numsides * sizeof(side_t));

  msd = (mapsidedef_t *)(&buf[sizeof(int)]);
  sd = sides;
  for (i = 0; i < numsides; i++, msd++, sd++) {
    sd->textureoffset = SHORT(msd->textureoffset) << FRACBITS;
    sd->rowoffset = SHORT(msd->rowoffset) << FRACBITS;
    /* LATER (or, not needed on tile 0)
    sd->toptexture = R_TextureNumForName(msd->toptexture);
    sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
    sd->midtexture = R_TextureNumForName(msd->midtexture);
    */
    sd->sector = &sectors[SHORT(msd->sector)];
  }
}

//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs(const unsigned char *buf) {
  byte *data;
  int i;
  maplinedef_t *mld;
  line_t *ld;
  vertex_t *v1;
  vertex_t *v2;

  int lumplen = ((int*)buf)[0];
  numlines = lumplen / sizeof(maplinedef_t);
  lines = IPU_level_malloc(numlines * sizeof(line_t));
  memset(lines, 0, numlines * sizeof(line_t));

  mld = (maplinedef_t *)(&buf[sizeof(int)]);
  ld = lines;
  for (i = 0; i < numlines; i++, mld++, ld++) {
    ld->flags = (mld->flags);
    ld->special = (mld->special);
    ld->tag = (mld->tag);
    v1 = ld->v1 = &vertexes[(mld->v1)];
    v2 = ld->v2 = &vertexes[(mld->v2)];
    ld->dx = v2->x - v1->x;
    ld->dy = v2->y - v1->y;

    if (!ld->dx)
      ld->slopetype = ST_VERTICAL;
    else if (!ld->dy)
      ld->slopetype = ST_HORIZONTAL;
    else {
      if (FixedDiv(ld->dy, ld->dx) > 0)
        ld->slopetype = ST_POSITIVE;
      else
        ld->slopetype = ST_NEGATIVE;
    }

    if (v1->x < v2->x) {
      ld->bbox[BOXLEFT] = v1->x;
      ld->bbox[BOXRIGHT] = v2->x;
    } else {
      ld->bbox[BOXLEFT] = v2->x;
      ld->bbox[BOXRIGHT] = v1->x;
    }

    if (v1->y < v2->y) {
      ld->bbox[BOXBOTTOM] = v1->y;
      ld->bbox[BOXTOP] = v2->y;
    } else {
      ld->bbox[BOXBOTTOM] = v2->y;
      ld->bbox[BOXTOP] = v1->y;
    }

    ld->sidenum[0] = SHORT(mld->sidenum[0]);
    ld->sidenum[1] = SHORT(mld->sidenum[1]);

    if (ld->sidenum[0] != -1)
      ld->frontsector = sides[ld->sidenum[0]].sector;
    else
      ld->frontsector = 0;

    if (ld->sidenum[1] != -1)
      ld->backsector = sides[ld->sidenum[1]].sector;
    else
      ld->backsector = 0;
  }
}

//
// P_LoadSubsectors
//
void P_LoadSubsectors(const unsigned char *buf) {
  byte *data;
  int i;
  mapsubsector_t *ms;
  subsector_t *ss;

  int lumplen = ((int*)buf)[0];
  numsubsectors = lumplen / sizeof(mapsubsector_t);
  subsectors = IPU_level_malloc(numsubsectors * sizeof(subsector_t));

  ms = (mapsubsector_t *)(&buf[sizeof(int)]);
  memset(subsectors, 0, numsubsectors * sizeof(subsector_t));
  ss = subsectors;

  for (i = 0; i < numsubsectors; i++, ss++, ms++) {
    ss->numlines = SHORT(ms->numsegs);
    ss->firstline = SHORT(ms->firstseg);
  }

}

//
// P_SetupLevel
//
void P_SetupLevel_pt0(const unsigned char unused) {
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
  // ipuprint("Map starts at lump "); ipuprintnum(gamelumpnum); ipuprint("\n");

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
}