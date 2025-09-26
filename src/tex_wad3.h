// wad loading for texture reflectivity calculation
#ifndef TEX_WAD3_H
#define TEX_WAD3_H
#pragma once

#include "cmdlib.h"
#define WAD_NAME_LEN 16
#define WAD_MAX_PATH 1024

void wad_init();
void wad_shutdown( void );

qboolean using_wads();
qboolean wad_get_texture_avgrbg( const char* texname, int32_t flags, float *r, float* g, float *b );
qboolean wad_has_texture( const char* texname );

#endif // TEX_WAD3_H