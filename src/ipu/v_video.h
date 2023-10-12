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
//	Gamma correction LUT.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//


#ifndef __V_VIDEO__
#define __V_VIDEO__

#include "doomtype.h"
#include "i_video.h"
// Needed because we are refering to patches.
#include "v_patch.h"

#include "ipu_utils.h"

//
// VIDEO
//

#define CENTERY			(SCREENHEIGHT/2)


extern int dirtybox[4];

extern byte *tinttable;

// haleyjd 08/28/10: implemented for Strife support
// haleyjd 08/28/10: Patch clipping callback, implemented to support Choco
// Strife.
typedef boolean (*vpatchclipfunc_t)(patch_t *, int, int);
__SUPER__ void V_SetPatchClipCallback(vpatchclipfunc_t func);


// Allocates buffer screens, call before R_Init.
__SUPER__ void V_Init (void);

// Draw a block from the specified source screen to the screen.

__SUPER__ void V_CopyRect(int srcx, int srcy, pixel_t *source,
                int width, int height,
                int destx, int desty);

__SUPER__ void V_DrawPatch(int x, int y, patch_t *patch);
__SUPER__ void V_DrawPatchFlipped(int x, int y, patch_t *patch);
__SUPER__ void V_DrawTLPatch(int x, int y, patch_t *patch);
__SUPER__ void V_DrawAltTLPatch(int x, int y, patch_t * patch);
__SUPER__ void V_DrawShadowedPatch(int x, int y, patch_t *patch);
__SUPER__ void V_DrawXlaPatch(int x, int y, patch_t * patch);     // villsa [STRIFE]
__SUPER__ void V_DrawPatchDirect(int x, int y, patch_t *patch);

// Draw a linear block of pixels into the view buffer.

__SUPER__ void V_DrawBlock(int x, int y, int width, int height, pixel_t *src);

__SUPER__ void V_MarkRect(int x, int y, int width, int height);

__SUPER__ void V_DrawFilledBox(int x, int y, int w, int h, int c);
__SUPER__ void V_DrawHorizLine(int x, int y, int w, int c);
__SUPER__ void V_DrawVertLine(int x, int y, int h, int c);
__SUPER__ void V_DrawBox(int x, int y, int w, int h, int c);

// Draw a raw screen lump

__SUPER__ void V_DrawRawScreen(byte *raw);

// Temporarily switch to using a different buffer to draw graphics, etc.

__SUPER__ void V_UseBuffer(pixel_t *buffer);

// Return to using the normal screen buffer to draw graphics.

__SUPER__ void V_RestoreBuffer(void);

// Save a screenshot of the current screen to a file, named in the 
// format described in the string passed to the function, eg.
// "DOOM%02i.pcx"

__SUPER__ void V_ScreenShot(char *format);

// Load the lookup table for translucency calculations from the TINTTAB
// lump.

__SUPER__ void V_LoadTintTable(void);

// villsa [STRIFE]
// Load the lookup table for translucency calculations from the XLATAB
// lump.

__SUPER__ void V_LoadXlaTable(void);

__SUPER__ void V_DrawMouseSpeedBox(int speed);

#endif

