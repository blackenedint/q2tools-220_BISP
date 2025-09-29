//
// blackened interactive texture.
//
#ifndef BI_TEXTURE_H
#define BI_TEXTURE_H
#pragma once

#include "cmdlib.h"

#define BTF_IDENT  	(('F'<<24) + ('T'<<16) + ('I'<<8) + 'B') //BITX (bi [t]e[x]ture)
#define BTF_FRAMEID  	(('M'<<24) + ('A'<< 16) + ('R'<<8) + 'F') //BITF (bi [t]exture [f]rame)
#define BTF_VER_MAJOR 1
#define BTF_VER_MINOR 0

#define BTFVersion( vmaj, vmin ) ((vmaj * 100) + (vmin * 10))
#define BTFHighest() ((BTF_VER_MAJOR*100) + (BTF_VER_MINOR*10))

#define SHA1_BUFFER_SIZE 20
#define MAX_BITEXTURE_NAME  64

// maintaining the same limit of named textures.
// +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 (sequence)
// -0 -1 -2 -3 -4 -5 -6 -7 -8 -9 (random)
// +a +b +c +d +e +f +g +h +i +j (alternates)
#define MAX_TEXTURE_FRAMES 10
#define MAX_ALTERNATE_TEX 10
static const char* BITEXTURE_EXT = "btf"; //leave with the . for bimap

typedef enum
{
	FMT_RGBA = 0,
	// FUTURE (TBD)
	FMT_RGB,
	FMT_ARGB,
} btf_format;

typedef enum
{
	Anim_None = 0,
	Anim_Sequence,
	Anim_Random
} btf_anim_type;

typedef struct btf_header_s
{
	uint32_t 	ident;
	int16_t 	ver_major;
	int16_t 	ver_minor;
} btf_header_t;

typedef struct btf_texinfo_s
{
	int32_t		width;
	int32_t		height;

	int16_t		compressiontype;
	int16_t		format;
	int16_t		animType;
	int16_t		frame_count;

	byte		reserved[44];

	int32_t		metadatasize;
	uint32_t	metadatatype;
} btf_texinfo_t;

typedef struct btf_frame_s
{
	uint32_t	ident;
	char		sha1[SHA1_BUFFER_SIZE];
	byte		reserved[36];
} btf_frame_t;

// metadata for quake2/vigil7
#define BTF_METAQ2 (('A'<<24) + ('T'<<16) + ('M'<<8) + 'Q')	//QMTA
typedef struct btf_q2meta_s
{
	int32_t		surfaceflags;
	int32_t		contents;
	float		value;
	int16_t		emissive;
	int16_t		alternate_count;
	//								//16
} btf_q2meta_t;


// to make my life not complete hell.
typedef struct btf_texture_s
{
	int32_t width;
	int32_t height;

	// q2 data.
	int32_t surfaceflags;
	int32_t contents;
	int32_t value;

	btf_format colorformat;
	byte* colordata;
	size_t colordatasize;
} btf_texture_t;

qboolean BTFLoadFromBuffer( byte* buffer, size_t buffersize, qboolean include_colordata, btf_texture_t* outTexture);



#endif // BI_TEXTURE_H