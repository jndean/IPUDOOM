
#include "m_fixed.h"
#include "p_local.h"
#include "r_state.h"
#include "st_stuff.h"

#include "ipu_print.h"


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


//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries(void) {
  int i;
  fixed_t a;
  fixed_t b;

  min_x = min_y = INT_MAX;
  max_x = max_y = -INT_MAX;

  for (i = 0; i < numvertexes; i++) {
    if (vertexes[i].x < min_x)
      min_x = vertexes[i].x;
    else if (vertexes[i].x > max_x)
      max_x = vertexes[i].x;

    if (vertexes[i].y < min_y)
      min_y = vertexes[i].y;
    else if (vertexes[i].y > max_y)
      max_y = vertexes[i].y;
  }

  max_w = max_x - min_x;
  max_h = max_y - min_y;

  min_w = 2 * PLAYERRADIUS; // const? never changed?
  min_h = 2 * PLAYERRADIUS;

  a = FixedDiv(f_w << FRACBITS, max_w);
  b = FixedDiv(f_h << FRACBITS, max_h);

  min_scale_mtof = a < b ? a : b;
  max_scale_mtof = FixedDiv(f_h << FRACBITS, 2 * PLAYERRADIUS);
}


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

  AM_findMinMaxBoundaries();
  scale_mtof = FixedDiv(min_scale_mtof, (int)(0.7 * FRACUNIT));
  if (scale_mtof > max_scale_mtof)
    scale_mtof = min_scale_mtof;
  scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}


//
// Handle events (user inputs) in automap mode
//
boolean AM_Responder(event_t *ev) {

  int rc;
  static int bigstate = 0;
  static char buffer[20];
  int key;
  ipuprint("AM_Responder responding\n");
  /* LATER
  rc = false;

  if (ev->type == ev_joystick && joybautomap >= 0 &&
      (ev->data1 & (1 << joybautomap)) != 0) {
    joywait = I_GetTime() + 5;

    if (!automapactive) {
      AM_Start();
      viewactive = false;
    } else {
      bigstate = 0;
      viewactive = true;
      AM_Stop();
    }

    return true;
  }

  if (!automapactive) {
    if (ev->type == ev_keydown && ev->data1 == key_map_toggle) {
      AM_Start();
      viewactive = false;
      rc = true;
    }
  } else if (ev->type == ev_keydown) {
    rc = true;
    key = ev->data1;

    if (key == key_map_east) // pan right
    {
      if (!followplayer)
        m_paninc.x = FTOM(F_PANINC);
      else
        rc = false;
    } else if (key == key_map_west) // pan left
    {
      if (!followplayer)
        m_paninc.x = -FTOM(F_PANINC);
      else
        rc = false;
    } else if (key == key_map_north) // pan up
    {
      if (!followplayer)
        m_paninc.y = FTOM(F_PANINC);
      else
        rc = false;
    } else if (key == key_map_south) // pan down
    {
      if (!followplayer)
        m_paninc.y = -FTOM(F_PANINC);
      else
        rc = false;
    } else if (key == key_map_zoomout) // zoom out
    {
      mtof_zoommul = M_ZOOMOUT;
      ftom_zoommul = M_ZOOMIN;
    } else if (key == key_map_zoomin) // zoom in
    {
      mtof_zoommul = M_ZOOMIN;
      ftom_zoommul = M_ZOOMOUT;
    } else if (key == key_map_toggle) {
      bigstate = 0;
      viewactive = true;
      AM_Stop();
    } else if (key == key_map_maxzoom) {
      bigstate = !bigstate;
      if (bigstate) {
        AM_saveScaleAndLoc();
        AM_minOutWindowScale();
      } else
        AM_restoreScaleAndLoc();
    } else if (key == key_map_follow) {
      followplayer = !followplayer;
      f_oldloc.x = INT_MAX;
      if (followplayer)
        plr->message = (AMSTR_FOLLOWON);
      else
        plr->message = (AMSTR_FOLLOWOFF);
    } else if (key == key_map_grid) {
      grid = !grid;
      if (grid)
        plr->message = (AMSTR_GRIDON);
      else
        plr->message = (AMSTR_GRIDOFF);
    } else if (key == key_map_mark) {
      M_snprintf(buffer, sizeof(buffer), "%s %d", (AMSTR_MARKEDSPOT),
                 markpointnum);
      plr->message = buffer;
      AM_addMark();
    } else if (key == key_map_clearmark) {
      AM_clearMarks();
      plr->message = (AMSTR_MARKSCLEARED);
    } else {
      rc = false;
    }

    if ((!deathmatch || gameversion <= exe_doom_1_8) &&
        cht_CheckCheat(&cheat_amap, ev->data2)) {
      rc = false;
      cheating = (cheating + 1) % 3;
    }
  } else if (ev->type == ev_keyup) {
    rc = false;
    key = ev->data1;

    if (key == key_map_east) {
      if (!followplayer)
        m_paninc.x = 0;
    } else if (key == key_map_west) {
      if (!followplayer)
        m_paninc.x = 0;
    } else if (key == key_map_north) {
      if (!followplayer)
        m_paninc.y = 0;
    } else if (key == key_map_south) {
      if (!followplayer)
        m_paninc.y = 0;
    } else if (key == key_map_zoomout || key == key_map_zoomin) {
      mtof_zoommul = FRACUNIT;
      ftom_zoommul = FRACUNIT;
    }
  }

  return rc;
  */
 return false; // TMP LATER
}


//
// Updates on Game Tick
//
void AM_Ticker(void) {
  // ipuprint("TICK!\n");
  if (!automapactive) 
    return;

  amclock++;
/* LATER

  if (followplayer)
    AM_doFollowPlayer();

  // Change the zoom if necessary
  if (ftom_zoommul != FRACUNIT)
    AM_changeWindowScale();

  // Change x,y location
  if (m_paninc.x || m_paninc.y)
    AM_changeWindowLoc();

*/

  // Update light level
  // AM_updateLightLev(); // NOT JOSEF
}

//
// Clear automap frame buffer.
//
void AM_clearFB(int color) { memset(fb, color, f_w * f_h * sizeof(*fb)); }

//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
boolean AM_clipMline(mline_t *ml, fline_t *fl) {
  enum { LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8 };

  register int outcode1 = 0;
  register int outcode2 = 0;
  register int outside;

  fpoint_t tmp;
  int dx;
  int dy;

#define DOOUTCODE(oc, mx, my)                                                  \
  (oc) = 0;                                                                    \
  if ((my) < 0)                                                                \
    (oc) |= TOP;                                                               \
  else if ((my) >= f_h)                                                        \
    (oc) |= BOTTOM;                                                            \
  if ((mx) < 0)                                                                \
    (oc) |= LEFT;                                                              \
  else if ((mx) >= f_w)                                                        \
    (oc) |= RIGHT;

  // do trivial rejects and outcodes
  if (ml->a.y > m_y2)
    outcode1 = TOP;
  else if (ml->a.y < m_y)
    outcode1 = BOTTOM;

  if (ml->b.y > m_y2)
    outcode2 = TOP;
  else if (ml->b.y < m_y)
    outcode2 = BOTTOM;

  if (outcode1 & outcode2)
    return false; // trivially outside

  if (ml->a.x < m_x)
    outcode1 |= LEFT;
  else if (ml->a.x > m_x2)
    outcode1 |= RIGHT;

  if (ml->b.x < m_x)
    outcode2 |= LEFT;
  else if (ml->b.x > m_x2)
    outcode2 |= RIGHT;

  if (outcode1 & outcode2)
    return false; // trivially outside

  // transform to frame-buffer coordinates.
  fl->a.x = CXMTOF(ml->a.x);
  fl->a.y = CYMTOF(ml->a.y);
  fl->b.x = CXMTOF(ml->b.x);
  fl->b.y = CYMTOF(ml->b.y);

  DOOUTCODE(outcode1, fl->a.x, fl->a.y);
  DOOUTCODE(outcode2, fl->b.x, fl->b.y);

  if (outcode1 & outcode2)
    return false;

  while (outcode1 | outcode2) {
    // may be partially inside box
    // find an outside point
    if (outcode1)
      outside = outcode1;
    else
      outside = outcode2;

    // clip to each side
    if (outside & TOP) {
      dy = fl->a.y - fl->b.y;
      dx = fl->b.x - fl->a.x;
      tmp.x = fl->a.x + (dx * (fl->a.y)) / dy;
      tmp.y = 0;
    } else if (outside & BOTTOM) {
      dy = fl->a.y - fl->b.y;
      dx = fl->b.x - fl->a.x;
      tmp.x = fl->a.x + (dx * (fl->a.y - f_h)) / dy;
      tmp.y = f_h - 1;
    } else if (outside & RIGHT) {
      dy = fl->b.y - fl->a.y;
      dx = fl->b.x - fl->a.x;
      tmp.y = fl->a.y + (dy * (f_w - 1 - fl->a.x)) / dx;
      tmp.x = f_w - 1;
    } else if (outside & LEFT) {
      dy = fl->b.y - fl->a.y;
      dx = fl->b.x - fl->a.x;
      tmp.y = fl->a.y + (dy * (-fl->a.x)) / dx;
      tmp.x = 0;
    } else {
      tmp.x = 0;
      tmp.y = 0;
    }

    if (outside == outcode1) {
      fl->a = tmp;
      DOOUTCODE(outcode1, fl->a.x, fl->a.y);
    } else {
      fl->b = tmp;
      DOOUTCODE(outcode2, fl->b.x, fl->b.y);
    }

    if (outcode1 & outcode2)
      return false; // trivially outside
  }

  return true;
}
#undef DOOUTCODE

//
// Classic Bresenham w/ whatever optimizations needed for speed
//
void AM_drawFline(fline_t *fl, int color) {
  register int x;
  register int y;
  register int dx;
  register int dy;
  register int sx;
  register int sy;
  register int ax;
  register int ay;
  register int d;

//   static int fuck = 0;

//   // For debugging only
//   if (fl->a.x < 0 || fl->a.x >= f_w || fl->a.y < 0 || fl->a.y >= f_h ||
//       fl->b.x < 0 || fl->b.x >= f_w || fl->b.y < 0 || fl->b.y >= f_h) {
//     fprintf(stderr, "fuck %d \r", fuck++);
//     return;
//   }

#define PUTDOT(xx, yy, cc) fb[(yy)*f_w + (xx)] = (cc)

  dx = fl->b.x - fl->a.x;
  ax = 2 * (dx < 0 ? -dx : dx);
  sx = dx < 0 ? -1 : 1;

  dy = fl->b.y - fl->a.y;
  ay = 2 * (dy < 0 ? -dy : dy);
  sy = dy < 0 ? -1 : 1;

  x = fl->a.x;
  y = fl->a.y;

  if (ax > ay) {
    d = ay - ax / 2;
    while (1) {
      PUTDOT(x, y, color);
      if (x == fl->b.x)
        return;
      if (d >= 0) {
        y += sy;
        d -= ax;
      }
      x += sx;
      d += ay;
    }
  } else {
    d = ax - ay / 2;
    while (1) {
      PUTDOT(x, y, color);
      if (y == fl->b.y)
        return;
      if (d >= 0) {
        x += sx;
        d -= ay;
      }
      y += sy;
      d += ax;
    }
  }
}

//
// Clip lines, draw visible part sof lines.
//
void AM_drawMline(mline_t *ml, int color) {
  static fline_t fl;

  if (AM_clipMline(ml, &fl))
    AM_drawFline(&fl, color); // draws it on frame buffer using fb coords
}



void AM_Drawer(pixel_t* fb_tensor) {
    fb = fb_tensor; // JOSEF

    // if (!automapactive)  // LATER
    //     return;

    
    AM_clearFB(BACKGROUND); 
    // if (grid) // LATER
        // AM_drawGrid(GRIDCOLORS);

    
    // mline_t ln = {{0, 0}, {10<< FRACBITS, 20<< FRACBITS}};
    fline_t fln = {{0, 0}, {10, 20}};
    // AM_drawMline(&ln, 56);
    AM_drawFline(&fln, 56);
    // for (int i = 0; i < 100; ++i) {
    //     fb[i * 321] = 56;
    // }
}