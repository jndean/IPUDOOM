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
//  Refresh module, data I/O, caching, retrieval of graphics
//  by name.
//

#ifndef __R_DATA__
#define __R_DATA__

#include "doomtype.h"
#include "m_fixed.h"

#include "ipu_utils.h"


extern int* texturewidthmask;
extern fixed_t* textureheight;

// Retrieve column data for span blitting.
__SUPER__ byte *R_GetColumn(int tex, int col);

// I/O, setting up the stuff.
__SUPER__ void R_InitData(void);
__SUPER__ void R_PrecacheLevel(void);

// Retrieval.
// Floor/ceiling opaque texture tiles,
// lookup by name. For animation?
__SUPER__ int R_FlatNumForName(char *name);

// Called by P_Ticker for switches and animations,
// returns the texture number for the texture name.
__SUPER__ int R_TextureNumForName(char *name);
__SUPER__ int R_CheckTextureNumForName(char *name);

#endif
