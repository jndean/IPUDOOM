
#include "m_fixed.h"

#include "p_local.h"

#include "st_stuff.h"


// For use if I do walls with outsides/insides
#define REDS (256 - 5 * 16)
#define REDRANGE 16
#define BLUES (256 - 4 * 16 + 8)
#define BLUERANGE 8
#define GREENS (7 * 16)
#define GREENRANGE 16
#define GRAYS (6 * 16)
#define GRAYSRANGE 16
#define BROWNS (4 * 16)
#define BROWNRANGE 16
#define YELLOWS (256 - 32 + 7)
#define YELLOWRANGE 1
#define BLACK 0
#define WHITE (256 - 47)

// Automap colors
#define BACKGROUND BLACK
#define YOURCOLORS WHITE
#define YOURRANGE 0
#define WALLCOLORS REDS
#define WALLRANGE REDRANGE
#define TSWALLCOLORS GRAYS
#define TSWALLRANGE GRAYSRANGE
#define FDWALLCOLORS BROWNS
#define FDWALLRANGE BROWNRANGE
#define CDWALLCOLORS YELLOWS
#define CDWALLRANGE YELLOWRANGE
#define THINGCOLORS GREENS
#define THINGRANGE GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE WALLRANGE
#define GRIDCOLORS (GRAYS + GRAYSRANGE / 2)
#define GRIDRANGE 0
#define XHAIRCOLORS GRAYS

// drawing stuff

#define AM_NUMMARKPOINTS 10

// scale on entry
#define INITSCALEMTOF (.2 * FRACUNIT)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC 4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN ((int)(1.02 * FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT ((int)(FRACUNIT / 1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x) << FRACBITS), scale_ftom)
#define MTOF(x) (FixedMul((x), scale_mtof) >> FRACBITS)
// translates between frame-buffer and map coordinates
#define CXMTOF(x) (f_x + MTOF((x)-m_x))
#define CYMTOF(y) (f_y + (f_h - MTOF((y)-m_y)))

// the following is crap
#define LINE_NEVERSEE ML_DONTDRAW

typedef struct { int x, y; } fpoint_t;

typedef struct { fpoint_t a, b; } fline_t;

typedef struct { fixed_t x, y; } mpoint_t;

typedef struct { mpoint_t a, b; } mline_t;

typedef struct { fixed_t slp, islp; } islope_t;

//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8 * PLAYERRADIUS) / 7)
mline_t player_arrow[] = {{{-R + R / 8, 0}, {R, 0}},    // -----
                          {{R, 0}, {R - R / 2, R / 4}}, // ----->
                          {{R, 0}, {R - R / 2, -R / 4}},
                          {{-R + R / 8, 0}, {-R - R / 8, R / 4}}, // >---->
                          {{-R + R / 8, 0}, {-R - R / 8, -R / 4}},
                          {{-R + 3 * R / 8, 0}, {-R + R / 8, R / 4}}, // >>--->
                          {{-R + 3 * R / 8, 0}, {-R + R / 8, -R / 4}}};
#undef R

#define R ((8 * PLAYERRADIUS) / 7)
mline_t cheat_player_arrow[] = {
    {{-R + R / 8, 0}, {R, 0}},    // -----
    {{R, 0}, {R - R / 2, R / 6}}, // ----->
    {{R, 0}, {R - R / 2, -R / 6}},
    {{-R + R / 8, 0}, {-R - R / 8, R / 6}}, // >----->
    {{-R + R / 8, 0}, {-R - R / 8, -R / 6}},
    {{-R + 3 * R / 8, 0}, {-R + R / 8, R / 6}}, // >>----->
    {{-R + 3 * R / 8, 0}, {-R + R / 8, -R / 6}},
    {{-R / 2, 0}, {-R / 2, -R / 6}}, // >>-d--->
    {{-R / 2, -R / 6}, {-R / 2 + R / 6, -R / 6}},
    {{-R / 2 + R / 6, -R / 6}, {-R / 2 + R / 6, R / 4}},
    {{-R / 6, 0}, {-R / 6, -R / 6}}, // >>-dd-->
    {{-R / 6, -R / 6}, {0, -R / 6}},
    {{0, -R / 6}, {0, R / 4}},
    {{R / 6, R / 4}, {R / 6, -R / 7}}, // >>-ddt->
    {{R / 6, -R / 7}, {R / 6 + R / 32, -R / 7 - R / 32}},
    {{R / 6 + R / 32, -R / 7 - R / 32}, {R / 6 + R / 10, -R / 7}}};
#undef R

#define R (FRACUNIT)
mline_t triangle_guy[] = {
    {{(fixed_t)(-.867 * R), (fixed_t)(-.5 * R)},
     {(fixed_t)(.867 * R), (fixed_t)(-.5 * R)}},
    {{(fixed_t)(.867 * R), (fixed_t)(-.5 * R)}, {(fixed_t)(0), (fixed_t)(R)}},
    {{(fixed_t)(0), (fixed_t)(R)}, {(fixed_t)(-.867 * R), (fixed_t)(-.5 * R)}}};
#undef R

#define R (FRACUNIT)
mline_t thintriangle_guy[] = {
    {{(fixed_t)(-.5 * R), (fixed_t)(-.7 * R)}, {(fixed_t)(R), (fixed_t)(0)}},
    {{(fixed_t)(R), (fixed_t)(0)}, {(fixed_t)(-.5 * R), (fixed_t)(.7 * R)}},
    {{(fixed_t)(-.5 * R), (fixed_t)(.7 * R)},
     {(fixed_t)(-.5 * R), (fixed_t)(-.7 * R)}}};
#undef R

static int cheating = 0;
static int grid = 0;

static int leveljuststarted = 1; // kluge until AM_LevelInit() is called

boolean automapactive = false;
static int finit_width = SCREENWIDTH;
static int finit_height = SCREENHEIGHT - ST_HEIGHT;

// location of window on screen
static int f_x;
static int f_y;

// size of window on screen
static int f_w;
static int f_h;

static int lightlev; // used for funky strobing effect
static pixel_t *fb;  // pseudo-frame buffer
static int amclock;

static mpoint_t m_paninc; // how far the window pans each tic (map coords)
static fixed_t
    mtof_zoommul; // how far the window zooms in each tic (map coords)
static fixed_t ftom_zoommul; // how far the window zooms in each tic (fb coords)

static fixed_t m_x, m_y;   // LL x,y where the window is on the map (map coords)
static fixed_t m_x2, m_y2; // UR x,y where the window is on the map (map coords)

//
// width/height of window on map (map coords)
//
static fixed_t m_w;
static fixed_t m_h;

// based on level size
static fixed_t min_x;
static fixed_t min_y;
static fixed_t max_x;
static fixed_t max_y;

static fixed_t max_w; // max_x-min_x,
static fixed_t max_h; // max_y-min_y

// based on player size
static fixed_t min_w;
static fixed_t min_h;

static fixed_t min_scale_mtof; // used to tell when to stop zooming out
static fixed_t max_scale_mtof; // used to tell when to stop zooming in

// old stuff for recovery later
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;

// old location used by the Follower routine
static mpoint_t f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_mtof = (fixed_t)INITSCALEMTOF;
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t scale_ftom;

static player_t *plr; // the player represented by an arrow

static patch_t *marknums[10]; // numbers used for marking by the automap
static mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
static int markpointnum = 0;                  // next point to be assigned

static int followplayer = 1; // specifies whether to follow the player around

cheatseq_t cheat_amap = CHEAT("iddt", 0);

static boolean stopped = true;


void AM_clearMarks(void) {
  int i;

  for (i = 0; i < AM_NUMMARKPOINTS; i++)
    markpoints[i].x = -1; // means empty
  markpointnum = 0;
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit(void) {
  leveljuststarted = 0;

  f_x = f_y = 0;
  f_w = finit_width;
  f_h = finit_height;

  AM_clearMarks();

  //AM_findMinMaxBoundaries(); // LATER
  scale_mtof = FixedDiv(min_scale_mtof, (int)(0.7 * FRACUNIT));
  if (scale_mtof > max_scale_mtof)
    scale_mtof = min_scale_mtof;
  scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}


//
// Clear automap frame buffer.
//
void AM_clearFB(int color) { memset(fb, color, f_w * f_h * sizeof(*fb)); }


void AM_Drawer(pixel_t* fb_tensor) {
    fb = fb_tensor; // JOSEF
    
    // if (!automapactive)  // LATER
    //     return;

    
    AM_clearFB(BACKGROUND);

    for (int i = 0; i < 100; ++i) {
        fb[i * 321] = 56;
    }
}