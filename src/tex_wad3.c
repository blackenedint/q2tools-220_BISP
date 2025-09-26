#include "tex_wad3.h"
#include "mathlib.h"
#include "bspfile.h"
#include "scriplib.h"
#include "strtools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#pragma pack(push,1)
typedef struct { 
	char id[4]; 
	int32_t numlumps; 
	int32_t infotableofs; 
} wad3_hdr_t;

typedef struct { 
	int32_t filepos;
	int32_t disksize;
	int32_t size; 
	char type;
	char compression;
	char pad1;
	char pad2; 
	char name[WAD_NAME_LEN]; 
} wad3_lumpinfo_t;

typedef struct { 
	char name[WAD_NAME_LEN]; 
	uint32_t width, height; 
	uint32_t offs[4]; 
} wad3_miptex_t;
#pragma pack(pop)

typedef struct {
    byte* data; 
	size_t size;
    wad3_hdr_t header;
    wad3_lumpinfo_t* lumps;
} wad3_t;

typedef struct {
	char name[WAD_NAME_LEN];
	int width, height;
	byte* pixels;
	byte* palette;
} wadtexture_t ;

qboolean HeaderValid( wad3_hdr_t * header ) { return ( header->id[0] == 'W' && header->id[1] == 'A' && header->id[2] == 'D' && header->id[3] == '3' ); }

void wad_close( wad3_t* wad)
{
	if ( !wad) return;
	free(wad->data);
	free(wad);
}

wad3_t* wad_open( const char* path )
{
	FILE *f = fopen(path, "rb");
	if ( !f ) 
		return NULL;
	
	fseek(f, 0, SEEK_END);
	
	size_t size = ftell(f);
	if ( size <= 0 ) {
		fclose(f); 
		return NULL;
	}
	fseek(f, 0, SEEK_SET);

	wad3_t*wad = (wad3_t*)calloc(1, sizeof(*wad));
	wad->data = (byte*)malloc(size);
	wad->size = size;
	if ( fread( wad->data, 1, size, f) != size) {
		fclose(f);
		free(wad->data);
		free(wad);
		printf("failed to load WAD: %s\n", path);
		return NULL;
	}
	fclose(f);

	memcpy(&wad->header, wad->data, sizeof(wad3_hdr_t));
	if ( !HeaderValid(&wad->header) ) {
		wad_close(wad);
		printf("failed to load WAD: %s\n", path);
		return NULL;
	}

	if ( (wad->header.infotableofs + wad->header.numlumps * sizeof(wad3_lumpinfo_t)) > wad->size ) {
		wad_close(wad); 
		printf("failed to load WAD: %s\n", path);
		return NULL;
	}
	
	return wad;
}

static int wad_comparename(const char* a, const char* b) {
    for (int i=0;i<WAD_NAME_LEN;i++) {
        unsigned char ca = (unsigned char)a[i], cb = (unsigned char)b[i];
        if (ca>='A' && ca<='Z') ca += 32;
        if (cb>='A' && cb<='Z') cb += 32;
        if (ca != cb) return 0;
        if (ca == 0) return 1;
    }
    return 1;
}

qboolean wad_find_texture( wad3_t* wad, const char* name, wadtexture_t* out )
{
    if (!wad || !name) 
		return false;
    for (int i=0;i<wad->header.numlumps;i++) {
        wad3_lumpinfo_t* L = &wad->lumps[i];
        if (!wad_comparename(L->name, name))
			continue;
        if ((size_t)(L->filepos + L->disksize) > wad->size) 
			return false;

        wad3_miptex_t* mt = (wad3_miptex_t*)(wad->data + L->filepos);
        if (L->disksize < (int)sizeof(*mt))
			 return false;

        memset(out, 0, sizeof(*out));
        memcpy(out->name, mt->name, WAD_NAME_LEN);
        out->width  = (int32_t)mt->width;
        out->height = (int32_t)mt->height;

        uint32_t pofs = mt->offs[0];
        if (!pofs || (size_t)(L->filepos + pofs + (size_t)out->width * out->height) > wad->size)
			return false;
        out->pixels = wad->data + L->filepos + pofs;

        /* Palette sits at end of lump: trailing 2-byte count (256) + 256*3 RGB */
        size_t lump_end = (size_t)L->filepos + (size_t)L->disksize;
        if (L->disksize > 2 + 256*3) 
			out->palette = wad->data + lump_end - (256*3);
        return true;
    }
    return false;
}

qboolean wad_average_RGB(wad3_t* wad, const char* texname, int32_t flags, float* rr, float* gg, float* bb) {
    wadtexture_t tex; 
	if (!wad_find_texture(wad, texname, &tex)) 
		return false;

    if (!tex.pixels || !tex.palette)
		return false;

	// kill contribution for warp/nodraw.
    if ( (flags & (SURF_WARP | SURF_NODRAW)) != 0 ) {
        if (rr) *rr = 0.0f; if (gg) *gg = 0.0f; if (bb) *bb = 0.0f;
        return true;
    }

	// alphatest / masked textures.
	qboolean is_masked = (flags & SURF_ALPHATEST) != 0;

	const byte* pal = tex.palette;
	const byte* px  = tex.pixels;

	const size_t n  = (size_t)tex.width * (size_t)tex.height;
	uint64_t R = 0, G = 0, B = 0;
	size_t used = 0;

	for (size_t i = 0; i < n; ++i) {
		const byte idx = px[i];
		if (is_masked && idx == 255) continue;         // fully transparent texel
		R += pal[idx*3+0]; G += pal[idx*3+1]; B += pal[idx*3+2];
		++used;
	}

	// Average visible texels to 0..1
	float rf = 0.0f, gf = 0.0f, bf = 0.0f;
	if (used > 0) {
		const float inv = 1.0f / (float)used;
		rf = (float)R * inv / 255.0f;
		gf = (float)G * inv / 255.0f;
		bf = (float)B * inv / 255.0f;
    }

	// Base alpha = coverage (masked) or 1.0 (opaque)
	float base_alpha = is_masked ? ((float)used / (float)n) : 1.0f;

	// Reproduce your TGA alpha rules with average alpha:
	// In your loop, a = tex_a/xxx, where tex_a ∈ [0..255].
	// Here we use avg alpha = base_alpha*255.
	float a;
	if ( (flags & SURF_TRANS33) && (flags & SURF_TRANS66) ) {
		a = (base_alpha * 255.0f) / 511.0f;
	} else if (flags & SURF_TRANS33) {
		a = (base_alpha * 255.0f) / 765.0f;
	} else if (flags & SURF_TRANS66) {
		a = (base_alpha * 255.0f) / 382.5f;
	} else {
		a = base_alpha;
	}
	if (a < 0.0f) 
		a = 0.0f; 
	else if (a > 1.0f)
		 a = 1.0f;

    // premultiply
	if (rr) *rr = rf * a;
	if (gg) *gg = gf * a;
	if (bb) *bb = bf * a;

	return true;
}


typedef struct {
	char path[WAD_MAX_PATH];
	wad3_t* wad;
} wadlist_t;

static wadlist_t g_wadlist[32];
static int g_wadcount = 0;

qboolean using_wads()
{
	return g_wadcount>0;
}

static void open_wad( const char* wadfile ) {
	if ( g_wadcount >= (int)(sizeof(g_wadlist)/sizeof(g_wadlist[0]))) return;
	if (!wadfile || !wadfile[0]) return;

	char path[1024];
	if ( FileExists((char*)wadfile)) {
		strncpy( path, wadfile, WAD_MAX_PATH-1);
	} else {
		snprintf(path, WAD_MAX_PATH, "%s%s", moddir, wadfile);
		if ( !FileExists(path) ) {
			snprintf(path, WAD_MAX_PATH, "%s%s", wadfile);
		}
	}

	wad3_t* wad = wad_open( path );
	if ( !wad ) return;
	strncpy( g_wadlist[g_wadcount].path, path, WAD_MAX_PATH-1);
	g_wadlist[g_wadcount].wad = wad;
	g_wadcount++;
	qprintf("Opened WAD: %s\n", path);
}

char* wad_parsevalue(char* value) {
	qprintf("parsing \"wad\": %s", value);
	int num_wads = 0;
	wadlist_t list[32];

	const char* str = value;
	char token[WAD_MAX_PATH];
	int idx = 0;
	while(1) {
		char c = *str++;
		int sep = (c==0) || (c==';') || (c==',') || (c==':') || isspace(c);
		if (!sep) { 
			if (idx < WAD_MAX_PATH-1) 
				token[idx++] = c; 
		} else {
			token[idx] = 0;
			if ( idx > 0 ) {
				Str_FixSlashes( token );
				const char* wadfile = Str_ExtractFilename( token );
				strncpy(list[num_wads].path, wadfile, WAD_MAX_PATH);
				list->wad = NULL;
				num_wads++;
				if ( num_wads >= 31 ) {
					qprintf("WAD limit reached\n");
					break; //can't include anymore!
				}
				idx = 0;
			}
			if (c==0) 
				break;
		}		
	}
	char tempstr[MAX_VALUE];
	for ( int i = 0; i < num_wads; i++ ) {
		if ( i != 0 ) strncat( tempstr, list[i].path, MAX_VALUE-1 );
		strncat( tempstr, list->path, MAX_VALUE-1);
	}

	return copystring(tempstr);
}

void wad_init()
{
	g_wadcount = 0;
	for (int i=0;i<(int)(sizeof(g_wadlist)/sizeof(g_wadlist[0]));i++) 
		g_wadlist[i].wad = NULL;

    if (num_entities <= 0) 
		return;
    const char* wadlist = ValueForKey(&entities[0], "wad");		
	const char* str = wadlist;
	char token[WAD_MAX_PATH];
	int idx = 0;
	while(1) {
		char c = *str++;
		int sep = (c==0) || (c==';') || (c==',') || (c==':') || isspace(c);
		if (!sep) { 
			if (idx < WAD_MAX_PATH-1) 
				token[idx++] = c; 
		} else {
			token[idx] = 0;
			if ( idx > 0 ) {
				// the wad key should already be clean; 
				// eg: "wad" "devtex.wad;liquids.wad;banana.wad" etc.
				open_wad( token );
				idx = 0;
			}
			if (c==0) 
				break;
		}
	}

	if ( g_wadcount > 0 )
		qprintf( "%d WADs loaded\n", g_wadcount );
}

void wad_shutdown( void )
{
	for ( int i = 0; i < g_wadcount; i++) {
		wad_close(g_wadlist[i].wad);
		g_wadlist[i].wad = NULL;
	}
	g_wadcount = 0;
}

qboolean wad_get_texture_avgrbg( const char* texname, int32_t flags, float *r, float* g, float *b ) {
	if (!texname || !texname[0]) 
    for (int i=0;i<g_wadcount;i++) {
        if (wad_average_RGB(g_wadlist[i].wad, texname, flags, r, g, b))
		 	return true;
    }
    return false;
}

qboolean wad_has_texture( const char* texname ) {
	if ( !texname || !texname[0] )
	for ( int i = 0; i < g_wadcount; i++ ) {
	
	wadtexture_t tex; 
	if (wad_find_texture(g_wadlist[i].wad, texname, &tex)) 
		return true;
	}
	return false;
}