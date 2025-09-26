//
// blackened interactive texture.
//
#ifndef BI_TEXTURE_H
#define BI_TEXTURE_H
#pragma once

#include "cmdlib.h"

#define BITEXTURE_MAGIC  	(('F'<<24) + ('T'<<16) + ('I'<<8) + 'B') //BITX (bi [t]e[x]ture)
#define BITEXTURE_RGBA  	(('A'<<24) + ('B'<< 16) + ('G'<<8) + 'R') //BITF (bi [t]exture [f]rame)
#define BITEX_VER_MAJOR 1
#define BITEX_VER_MINOR 0

#define MAX_BITEXTURE_NAME  64

// maintaining the same limit of named textures.
// +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 (sequence)
// -0 -1 -2 -3 -4 -5 -6 -7 -8 -9 (random)
// +a +b +c +d +e +f +g +h +i +j (alternates)
#define MAX_TEXTURE_FRAMES 10
#define MAX_ALTERNATE_TEX 10
static const char* BITEXTURE_EXT = ".btf";

typedef struct
{
	uint32_t id;
	int16_t ver_major;
	int16_t ver_minor;
	int16_t compressiontype;
	int16_t animType;

	int32_t width;
	int32_t height;

	int32_t surfaceflags;
	int32_t contents;
	int32_t lightvalue;		// only for SURF_LIGHT. nothing else uses it.
	int16_t emissive;		// texture is emissive; alpha is mask.
	int16_t	pad1;

	int16_t alternate_count;
	int16_t frame_count;
} bitexture_t;

#endif // BI_TEXTURE_H