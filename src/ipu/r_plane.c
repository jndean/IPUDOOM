//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Here is a core component: drawing the floors and ceilings,
//	 while maintaining a per column clipping list only.
//	Moreover, the sky areas have to be determined.
//

#include <stdlib.h>
#include <string.h>

#include "doomstat.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_video.h"
#include "m_fixed.h"
#include "r_bsp.h"
#include "r_data.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_plane.h"
#include "r_segs.h"
#include "r_sky.h"
#include "r_state.h"
#include "r_things.h"
#include "tables.h"
// #include "w_wad.h" // LATER
// #include "z_zone.h" // LATER

#include "ipu_interface.h"
#include "ipu_texturetiles.h"
#include "print.h"



planefunction_t floorfunc;
planefunction_t ceilingfunc;

//
// opening
//

// Here comes the obnoxious "visplane".
#define MAXVISPLANES 48 // JOSEF: 128
visplane_t visplanes[MAXVISPLANES];
visplane_t *lastvisplane;
visplane_t *floorplane;
visplane_t *ceilingplane;

// ?
#define MAXOPENINGS SCREENWIDTH * 64 // JOSEF: Surely this can be smaller on IPU
short openings[MAXOPENINGS];
short *lastopening;

//
// Clip values are the solid pixel bounding the range.
//  floorclip starts out SCREENHEIGHT
//  ceilingclip starts out -1
//
short floorclip[SCREENWIDTH];
short ceilingclip[SCREENWIDTH];

//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
int spanstart[SCREENHEIGHT];
int spanstop[SCREENHEIGHT];

//
// texture mapping
//
lighttable_t **planezlight;
fixed_t planeheight;

fixed_t yslope[SCREENHEIGHT];
fixed_t distscale[SCREENWIDTH];
fixed_t basexscale;
fixed_t baseyscale;

// JOSEF: Tbh, could forgo these caches and just recompute every time on IPU?
fixed_t cachedheight[SCREENHEIGHT];
fixed_t cacheddistance[SCREENHEIGHT];
fixed_t cachedxstep[SCREENHEIGHT];
fixed_t cachedystep[SCREENHEIGHT];

// JOSEF: Global to pass over exchange
int flatnum;

__SUPER__ 
int abs(int x); // JOSEF


//
// R_InitPlanes
// Only at game startup.
//
__SUPER__ 
void R_InitPlanes(void) {
  // Doh!
}

//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  ds_source
//  basexscale
//  baseyscale
//  viewx
//  viewy
//
// BASIC PRIMITIVE
//
__SUPER__
void R_MapPlane(int y, int x1, int x2) {
  angle_t angle;
  fixed_t distance;
  fixed_t length;
  // unsigned index; // JOSEF: store in walllightindex instead

  if (x2 < x1 || x1 < 0 || x2 >= viewwidth || y > viewheight) {
    printf("ERROR: R_MapPlane: %d, %d at %d", x1, x2, y);
  }

  if (planeheight != cachedheight[y]) {
    cachedheight[y] = planeheight;
    distance = cacheddistance[y] = FixedMul(planeheight, yslope[y]);
    ds_xstep = cachedxstep[y] = FixedMul(distance, basexscale);
    ds_ystep = cachedystep[y] = FixedMul(distance, baseyscale);
  } else {
    distance = cacheddistance[y];
    ds_xstep = cachedxstep[y];
    ds_ystep = cachedystep[y];
  }

  length = FixedMul(distance, distscale[x1]);
  angle = (viewangle + xtoviewangle[x1]) >> ANGLETOFINESHIFT;
  ds_xfrac = viewx + FixedMul(finecosine[angle], length);
  ds_yfrac = -viewy - FixedMul(finesine[angle], length);

  if (fixedcolormap)
    ds_colormap = fixedcolormap;
  else {
    walllightindex = distance >> LIGHTZSHIFT;

    if (walllightindex >= MAXLIGHTZ)
      walllightindex = MAXLIGHTZ - 1;

    // ds_colormap = planezlight[index]; // JOSEF: LATER, store this in lightscale
  }

  ds_y = y;
  ds_x1 = x1;
  ds_x2 = x2;

  // high or low detail
  // spanfunc(); // LATER
  // R_DrawSpan();
  IPURequest_R_DrawSpan();
}

//
// R_ClearPlanes
// At begining of frame.
//
__SUPER__ 
void R_ClearPlanes(void) {
  int i;
  angle_t angle;

  // opening / clipping determination
  for (i = 0; i < viewwidth; i++) {
    floorclip[i] = viewheight;
    ceilingclip[i] = -1;
  }

  lastvisplane = visplanes;
  lastopening = openings;

  // texture calculation
  memset(cachedheight, 0, sizeof(cachedheight));

  // left to right mapping
  angle = (viewangle - ANG90) >> ANGLETOFINESHIFT;

  // scale will be unit scale at SCREENWIDTH/2 distance
  basexscale = FixedDiv(finecosine[angle], centerxfrac);
  baseyscale = -FixedDiv(finesine[angle], centerxfrac);
}

//
// R_FindPlane
//
__SUPER__
visplane_t *R_FindPlane(fixed_t height, int picnum, int lightlevel) {
  visplane_t *check;

  if (picnum == skyflatnum) {
    height = 0; // all skys map together
    lightlevel = 0;
  }

  for (check = visplanes; check < lastvisplane; check++) {
    if (height == check->height && picnum == check->picnum &&
        lightlevel == check->lightlevel) {
      break;
    }
  }

  if (check < lastvisplane)
    return check;

  if (lastvisplane - visplanes == MAXVISPLANES)
    printf("R_FindPlane: no more visplanes\n");
    // I_Error("R_FindPlane: no more visplanes");

  lastvisplane++;

  check->height = height;
  check->picnum = picnum;
  check->lightlevel = lightlevel;
  check->minx = SCREENWIDTH;
  check->maxx = -1;

  memset(check->top, 0xff, sizeof(check->top));

  return check;
}

//
// R_CheckPlane
//
__SUPER__
visplane_t *R_CheckPlane(visplane_t *pl, int start, int stop) {
  int intrl;
  int intrh;
  int unionl;
  int unionh;
  int x;

  if (start < pl->minx) {
    intrl = pl->minx;
    unionl = start;
  } else {
    unionl = pl->minx;
    intrl = start;
  }

  if (stop > pl->maxx) {
    intrh = pl->maxx;
    unionh = stop;
  } else {
    unionh = pl->maxx;
    intrh = stop;
  }

  for (x = intrl; x <= intrh; x++)
    if (pl->top[x] != 0xff)
      break;

  if (x > intrh) {
    pl->minx = unionl;
    pl->maxx = unionh;

    // use the same one
    return pl;
  }

  // make a new visplane
  lastvisplane->height = pl->height;
  lastvisplane->picnum = pl->picnum;
  lastvisplane->lightlevel = pl->lightlevel;

  pl = lastvisplane++;
  pl->minx = start;
  pl->maxx = stop;

  memset(pl->top, 0xff, sizeof(pl->top));

  return pl;
}

//
// R_MakeSpans
//
__SUPER__
void R_MakeSpans(int x, int t1, int b1, int t2, int b2) {
  while (t1 < t2 && t1 <= b1) {
    R_MapPlane(t1, spanstart[t1], x - 1);
    t1++;
  }
  while (b1 > b2 && b1 >= t1) {
    R_MapPlane(b1, spanstart[b1], x - 1);
    b1--;
  }

  while (t2 < t1 && t2 <= b2) {
    spanstart[t2] = x;
    t2++;
  }
  while (b2 > b1 && b2 >= t2) {
    spanstart[b2] = x;
    b2--;
  }
}


//
// R_DrawPlanes
// At the end of each frame.
//
__SUPER__ 
void R_DrawPlanes(void) {
  visplane_t *pl;
  // int light; JOSEF: store in `lightnum` instead herein
  int x;
  int stop;
  int angle;
  int lumpnum;

  if (ds_p - drawsegs > MAXDRAWSEGS)
    printf("R_DrawPlanes: drawsegs overflow (%u)\n", ds_p - drawsegs);
    // I_Error("R_DrawPlanes: drawsegs overflow (%i)", ds_p - drawsegs);

  if (lastvisplane - visplanes > MAXVISPLANES)
    printf("R_DrawPlanes: visplane overflow (%u)\n", lastvisplane - visplanes);
    // I_Error("R_DrawPlanes: visplane overflow (%i)", lastvisplane - visplanes);

  if (lastopening - openings > MAXOPENINGS)
    printf("R_DrawPlanes: opening overflow (%u)\n", lastopening - openings);
    // I_Error("R_DrawPlanes: opening overflow (%i)", lastopening - openings);

  for (pl = visplanes; pl < lastvisplane; pl++) {
    if (pl->minx > pl->maxx)
      continue;

    // sky flat
    if (pl->picnum == skyflatnum) {
      dc_iscale = pspriteiscale >> detailshift;

      // Sky is allways drawn full bright,
      //  i.e. colormaps[0] is used.
      // Because of this hack, sky is not affected
      //  by INVUL inverse mapping.
      // dc_colormap = colormaps; // JOSEF: IPU sends light params to texture tiles instead
      walllightindex = MAXLIGHTSCALE - 1; // JOSEF
      lightnum = LIGHTLEVELS - 1; // JOSEF
      dc_texturemid = skytexturemid;
      for (x = pl->minx; x <= pl->maxx; x++) {
        dc_yl = pl->top[x];
        dc_yh = pl->bottom[x];

        if (dc_yl <= dc_yh) {
          angle = (viewangle + xtoviewangle[x]) >> ANGLETOSKYSHIFT;
          dc_x = x;
          // dc_source = R_GetColumn(skytexture, angle); // JOSEF
          dc_source = IPU_R_RequestColumn(skytexture, angle); 
          R_DrawColumn(); // JOSEF: colfunc();
        }
      }
      continue;
    }

    // JOSEF: Don't load lump, just record flatnum to send over exchange later
    flatnum = numtextures + pl->picnum; // LATER: numtextures + flattranslation[pl->picnum];
    // regular flat
    // lumpnum = firstflat + flattranslation[pl->picnum];
    // ds_source = W_CacheLumpNum(lumpnum, PU_STATIC);

    planeheight = abs(pl->height - viewz);
    lightnum = (pl->lightlevel >> LIGHTSEGSHIFT) + extralight;

    if (lightnum >= LIGHTLEVELS)
      lightnum = LIGHTLEVELS - 1;

    if (lightnum < 0)
      lightnum = 0;

    // JOSEF: Texture tiles are in charge of scaling by lightnum now
    // planezlight = zlight[lightnum]; 

    pl->top[pl->maxx + 1] = 0xff;
    pl->top[pl->minx - 1] = 0xff;

    stop = pl->maxx + 1;

    for (x = pl->minx; x <= stop; x++) {
      R_MakeSpans(x, pl->top[x - 1], pl->bottom[x - 1], pl->top[x],
                  pl->bottom[x]);

      // JOSEF: TMP solid colour visualisation
      // pixel_t colour = (140 + (pl - visplanes) * 2) % 256;
      // pixel_t colour = (pl->picnum * 17 + 209) % 256;
      // for (int y = pl->top[x]; y <= pl->bottom[x]; y++) {
      //   pixel_t* dest = (I_VideoBuffer + (y + viewwindowy) * IPUCOLSPERRENDERTILE) + (viewwindowx + x - tileLeftClip);
      //   *dest = colour;
      // }
    }

    // W_ReleaseLumpNum(lumpnum); // JOSEF
    
  }
}
