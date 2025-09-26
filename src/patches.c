/*
===========================================================================
Copyright (C) 1997-2006 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
===========================================================================
*/

#include "qrad.h"

vec3_t texture_reflectivity[MAX_MAP_TEXINFO_QBSP];

int32_t cluster_neg_one = 0;
float *texture_data[MAX_MAP_TEXINFO_QBSP];
int32_t texture_sizes[MAX_MAP_TEXINFO_QBSP][2];
/*
===================================================================

  TEXTURE LIGHT VALUES

===================================================================
*/

/*
   ======================
   CalcTextureReflectivity_Heretic2
   ======================
 */
void CalcTextureReflectivity_Heretic2( void ){
	int32_t i;
	int32_t j, texels;
	int32_t color[3];
	int32_t texel;
	char path[1100];
	float r;
	miptex_m8_t     *mt;
	miptex_m32_t        *mt32;
	byte            *pos;


	// allways set index 0 even if no textures
	texture_reflectivity[0][0] = 0.5;
	texture_reflectivity[0][1] = 0.5;
	texture_reflectivity[0][2] = 0.5;

	for ( i = 0 ; i < numtexinfo ; i++ )
	{
		// see if an earlier texinfo allready got the value
		for ( j = 0 ; j < i ; j++ )
		{
			if ( !strcmp( texinfo[i].texture, texinfo[j].texture ) ) {
				VectorCopy( texture_reflectivity[j], texture_reflectivity[i] );
				break;
			}
		}
		if ( j != i ) {
			continue;
		}

		// load the wal file

		sprintf( path, "%stextures/%s.m32", moddir, texinfo[i].texture );
		if ( TryLoadFile( path, (void **)&mt32, false ) == -1 ) {
			sprintf( path, "%stextures/%s.m8", moddir, texinfo[i].texture );
			if ( TryLoadFile( path, (void **)&mt, false ) == -1 ) {
				texture_reflectivity[i][0] = 0.5;
				texture_reflectivity[i][1] = 0.5;
				texture_reflectivity[i][2] = 0.5;
				continue;
			}
			texels = LittleLong( mt->width[0] ) * LittleLong( mt->height[0] );
			color[0] = color[1] = color[2] = 0;

			for ( j = 0 ; j < texels ; j++ )
			{
				texel = ( (byte *)mt )[LittleLong( mt->offsets[0] ) + j];
				color[0] += mt->palette[texel].r;
				color[1] += mt->palette[texel].g;
				color[2] += mt->palette[texel].b;
			}

			free( mt );
		}
		else
		{
			texels = LittleLong( mt32->width[0] ) * LittleLong( mt32->height[0] );
			color[0] = color[1] = color[2] = 0;

			for ( j = 0 ; j < texels ; j++ )
			{
				pos = (byte *)mt32 + mt32->offsets[0] + ( j << 2 );
				color[0] += *pos++; // r
				color[1] += *pos++; // g
				color[2] += *pos++; // b
			}

			free( mt32 );
		}


		for ( j = 0 ; j < 3 ; j++ )
		{
			r = color[j] / ( (float) texels * 255.0 );
			texture_reflectivity[i][j] = r;
		}
	}
}

/*
======================
CalcTextureReflectivity
======================
*/
void CalcTextureReflectivity(void) {
    int32_t i, j, k, count;
    int32_t texels, texel;
    qboolean wal_tex = false;
    float color[3], cur_color[3], tex_a, a;
    char path[1200];
    float *r, *g, *b;
    float c;
    byte *pbuffer = NULL; // mxd. "potentially uninitialized local pointer variable" in VS2017 if uninitialized
    byte *palette = NULL;
    byte *palette_frompak = NULL;
    byte *ptexel = NULL;
    miptex_t *mt = NULL; // mxd. "potentially uninitialized local pointer variable" in VS2017 if uninitialized
    float *fbuffer = NULL;
	float *ftexel = NULL;
    int32_t width, height;

#ifdef BLACKENED
	bitexture_t* mt_btf = NULL;
	size_t btf_size = 0;
	qboolean btf_tex = false;
	qboolean wal_palette_loaded = false; // extra safety.
	

	if ( !using_wads() ) {
		qprintf("No WADs loaded\nUsing .BTF, .wal or .tga for reflectivity\n");
	}
#endif

    // get the game palette
    // qb: looks in moddir then basedir
    sprintf(path, "%spics/colormap.pcx", moddir);
    if (FileExists(path)) {
        Load256Image(path, NULL, &palette, NULL, NULL);
#ifdef BLACKENED			
		wal_palette_loaded = true;
#endif			
    } else {
        sprintf(path, "%spics/colormap.pcx", basedir);
        if(FileExists(path)) {
            Load256Image(path, NULL, &palette, NULL, NULL);
#ifdef BLACKENED			
			wal_palette_loaded = true;
#endif			
        } else if((i = TryLoadFileFromPak("pics/colormap.pcx", (void **)&palette_frompak, moddir)) != -1) {
            // unicat: load from pack files, palette is loaded from the last 768 bytes
            palette = palette_frompak - (i - 768);
#ifdef BLACKENED			
			wal_palette_loaded = true;
#endif			
        } else {
#ifdef BLACKENED
			qprintf( "unable to load pics/colormap.pcx\nWAL textures unavailable\n");
			if ( !using_wads() ) {
				qprintf( "Warning: No WADs or WAL textures available for calculating reflectivity; If TGA replacements are also missing, all reflectivity will be default!\n");
			}
#else			
			Error("unable to load pics/colormap.pcx");
#endif			
		}
    }

    // always set index 0 even if no textures
    texture_reflectivity[0][0] = 0.5;
    texture_reflectivity[0][1] = 0.5;
    texture_reflectivity[0][2] = 0.5;

    for (i = 0; i < numtexinfo; i++) {
        // default
        texture_reflectivity[i][0] = 0.5f;
        texture_reflectivity[i][1] = 0.5f;
        texture_reflectivity[i][2] = 0.5f;

        // see if an earlier texinfo already got the value
        for (j = 0; j < i; j++) {
            if (!strcmp(texinfo[i].texture, texinfo[j].texture)) {
                VectorCopy(texture_reflectivity[j], texture_reflectivity[i]);
                texture_data[i]     = texture_data[j];
                texture_sizes[i][0] = texture_sizes[j][0];
                texture_sizes[i][1] = texture_sizes[j][1];
                break;
            }
        }
        if (j != i)
            continue;

#ifdef BLACKENED
		// if using wads, try and get the average directly from there.
		// FIXME: This is broken. the texture bytes need saved too, for the rest of rad!!
		if ( using_wads() ) {
			float rr, gg, bb;
			if ( wad_has_texture( texinfo[i].texture) ) {
				if ( wad_get_texture_avgrbg( texinfo[i].texture, texinfo[i].flags, &rr, &gg, &bb)) {
        			texture_reflectivity[i][0] = rr;
        			texture_reflectivity[i][1] = gg;
        			texture_reflectivity[i][2] = bb;
					goto calc_reflectivity;
				}
			}
		}

		// look for BTF in moddir
		sprintf(path, "%stextures/%s%s", moddir, texinfo[i].texture, BITEXTURE_EXT);
		qprintf("attempting %s\n", path);
		if ( FileExists(path)) {
			int32_t len = TryLoadFile(path, (void**)&mt_btf, false);
			if (len != -1) { //TODO: check version ranges.
				if (LittleLong(mt_btf->id) != BITEXTURE_MAGIC && LittleShort(mt_btf->ver_major) != BITEX_VER_MAJOR &&  LittleShort(mt_btf->ver_minor) != BITEX_VER_MINOR) {
					btf_tex = true;
					btf_size = (size_t)len;
				} else  {
					qprintf( "invalid BTF header\n"); 
					free(mt_btf);
					mt_btf = NULL;
				}
			}
		} else {
			// look in base dir.
			sprintf(path, "%stextures/%s%s", basedir, texinfo[i].texture, BITEXTURE_EXT);
			qprintf("load %s\n", path);
			if (FileExists(path)) {
				int32_t len = TryLoadFile(path, (void **)&mt_btf, false);
				if (len != -1) { //TODO: check version ranges.
					if (LittleLong(mt_btf->id) != BITEXTURE_MAGIC && LittleShort(mt_btf->ver_major) != BITEX_VER_MAJOR &&  LittleShort(mt_btf->ver_minor) != BITEX_VER_MINOR) {
						btf_tex = true;
						btf_size = (size_t)len;
					} else  {
						qprintf( "invalid BTF header\n"); 
						free(mt_btf);
						mt_btf = NULL;
					}
				}
			}
		}

		if ( btf_tex ) {
			// frame RGBA data is always hdr->width * hdr->height * 4.
			const uint32_t frame_bytes = mt_btf->width * mt_btf->height * 4;
			if (frame_bytes <= 0) {
    			qprintf("tex %i (%s) invalid dimensions %dx%d\n", i, path, mt_btf->width, mt_btf->height);
    			continue;
			}
			
			// start right after the header
			byte* p = (byte*)mt_btf + sizeof(bitexture_t);
			
			// align to 4 bytes
			uintptr_t up = (uintptr_t)p;
			if (up & 3) p += (4 - (up & 3));

			// skip the alternate textures block. we don't care.
			if ( mt_btf->alternate_count > 0 )
				p += mt_btf->alternate_count * MAX_BITEXTURE_NAME;

			// read sentinel
			if ((size_t)(p - (byte*)mt_btf + 8) > btf_size) {
    			qprintf("tex %i (%s) truncated BTF before frame header\n", i, path);
    			continue;
			}

			uint32_t raw_id = *(uint32_t*)p; p += 4;
			uint32_t rgba_tag = LittleLong(raw_id);
			
			if (rgba_tag != BITEXTURE_RGBA) {
    			qprintf("tex %i (%s) first frame missing (got 0x%08X)\n", i, path, rgba_tag);
    			continue;
			}
			
			if ((size_t)(p - (byte*)mt_btf + frame_bytes) > btf_size) {
				qprintf("tex %i (%s) truncated BTF in frame payload\n", i, path);
    			continue;
			}

			pbuffer = (byte*)malloc(frame_bytes);
			if (!pbuffer) {
    			qprintf("tex %i (%s) unable to allocate %d bytes for frame\n", i, path, frame_bytes);
    			continue;
			}
			
			// copy only expected RGBA pixels (ignore any padding/extra)
			memcpy(pbuffer, p, (size_t)frame_bytes);

			color[0] = color[1] = color[2] = 0.0f;
			ptexel						 = pbuffer;
			fbuffer						= malloc(texels * 4 * sizeof(float));
			ftexel						 = fbuffer;

			for (count = texels; count--;) {
				cur_color[0] 	= (float)(*ptexel++); // r
				cur_color[1] 	= (float)(*ptexel++); // g
				cur_color[2] 	= (float)(*ptexel++); // b
				tex_a			= (float)(*ptexel++);

				if (texinfo[i].flags & (SURF_WARP | SURF_NODRAW)) {
					a = 0.0; //no contribution.
				} else if ((texinfo[i].flags & SURF_ALPHATEST)) {
					// for alpha test, count the pixels that are above the alpha clip as opaque.
					// otherwise, no contribution.
					a = (tex_a > 127.5f) ? 1.0f : 0.0f;
				} else if ((texinfo[i].flags & SURF_TRANSLUCENT) ) {
					a = tex_a;
				} else if ((texinfo[i].flags & SURF_TRANS33) && (texinfo[i].flags & SURF_TRANS66)) {
					a = tex_a / 511.0;
				} else if (texinfo[i].flags & SURF_TRANS33) {
					a = tex_a / 765.0;
				} else if (texinfo[i].flags & SURF_TRANS66) {
					a = tex_a / 382.5;
				} else {
					a = 1.0;
				}

				for (j = 0; j < 3; j++) {
					*ftexel++ = cur_color[j] / 255.0;
					color[j] += cur_color[j] * a;
				}
				*ftexel++ = a;
			}

			texture_data[i] = fbuffer;
			//NOTE; these don't appear to be actually used anywhere; but I'm setting them anyway.
			texture_sizes[i][0] = mt_btf->width;
			texture_sizes[i][1] = mt_btf->height;
			free(pbuffer);
			
			// we can free the loaded buffer now, because the texture data was copied.
			free(mt_btf);
			mt_btf = NULL;

			// skip the rest of the wal / tga loading.
			goto calc_reflectivity;
		}
#endif
        // buffer is RGBA  (A  set to 255 for 24 bit format)
        // qb: looks in moddir then basedir
        sprintf(path, "%stextures/%s.tga", moddir, texinfo[i].texture);
        if (FileExists(path)) // LoadTGA expects file to exist
        {
            LoadTGA(path, &pbuffer, &width, &height); // load rgba data
            qprintf("load %s\n", path);
        } else {
			#ifdef BLACKENED
			if ( !wal_palette_loaded ) {
				// skip .wal and attempt a tga in basedir.
				sprintf(path, "%stextures/%s.tga", basedir, texinfo[i].texture);
				 if (FileExists(path)) {
					LoadTGA(path, &pbuffer, &width, &height); // load rgba data
					qprintf("load %s\n", path);
                }
				else {
						qprintf("NOT FOUND %s\n", path);
						continue;
				}
			}
			#endif

            // look for wal file in moddir
            sprintf(path, "%stextures/%s.wal", moddir, texinfo[i].texture);
            qprintf("attempting %s\n", path);

            // load the miptex to get the flags and values
            if (FileExists(path)) // qb: linux segfault if not exist
            {
                if (TryLoadFile(path, (void **)&mt, false) != -1)
                    wal_tex = true;
            } else {
                // look for TGA in basedir            
				sprintf(path, "%stextures/%s.tga", basedir, texinfo[i].texture);
                if (FileExists(path)) {
                    LoadTGA(path, &pbuffer, &width, &height); // load rgba data
                    qprintf("load %s\n", path);
                } else {
                    // look for wal file in base dir
                    sprintf(path, "%stextures/%s.wal", basedir, texinfo[i].texture);
                    qprintf("load %s\n", path);

                    // load the miptex to get the flags and values
                    if (FileExists(path)) // qb: linux segfault if not exist
                    {
                        if (TryLoadFile(path, (void **)&mt, false) != -1)
                            wal_tex = true;
                    } else {
                        qprintf("NOT FOUND %s\n", path);
                        continue;
                    }
                }
            }
        }

        //
        // Calculate the "average color" for the texture
        //

        if (wal_tex) {
            texels   = LittleLong(mt->width) * LittleLong(mt->height);
            color[0] = color[1] = color[2] = 0;

            for (j = 0; j < texels; j++) {
                texel = ((byte *)mt)[LittleLong(mt->offsets[0]) + j];
                for (k = 0; k < 3; k++)
                    color[k] += palette[texel * 3 + k];
            }
        } else {
            texels = width * height;
            if (texels <= 0) {
                qprintf("tex %i (%s) no rgba data (file broken?)\n", i, path);
                continue; // empty texture, possible bad file
            }

            color[0] = color[1] = color[2] = 0.0f;
            ptexel                         = pbuffer;
            fbuffer                        = malloc(texels * 4 * sizeof(float));
            ftexel                         = fbuffer;

            for (count = texels; count--;) {
                cur_color[0] = (float)(*ptexel++); // r
                cur_color[1] = (float)(*ptexel++); // g
                cur_color[2] = (float)(*ptexel++); // b
                tex_a        = (float)(*ptexel++);

                if (texinfo[i].flags & (SURF_WARP | SURF_NODRAW | SURF_ALPHATEST)) {
                    a = 0.0;
                } else if ((texinfo[i].flags & SURF_TRANS33) && (texinfo[i].flags & SURF_TRANS66)) {
                    a = tex_a / 511.0;
                } else if (texinfo[i].flags & SURF_TRANS33) {
                    a = tex_a / 765.0;
                } else if (texinfo[i].flags & SURF_TRANS66) {
                    a = tex_a / 382.5;
                } else {
                    a = 1.0;
                }

                for (j = 0; j < 3; j++) {
                    *ftexel++ = cur_color[j] / 255.0;
                    color[j] += cur_color[j] * a;
                }
                *ftexel++ = a;
            }

            // never freed but we'll need it up until the end
            texture_data[i] = fbuffer;
            // qb: freed in LoadTGA now.  free(pbuffer);
        }

        for (j = 0; j < 3; j++) {
            // average RGB for the texture to 0.0..1.0 range
            c                          = color[j] / (float)texels / 255.0f;
            texture_reflectivity[i][j] = c;
        }

#ifdef BLACKENED
calc_reflectivity:
#endif		

// reflectivity saturation
#define Pr .299
#define Pg .587
#define Pb .114

        //  public-domain function by Darel Rex Finley
        //
        //  The passed-in RGB values can be on any desired scale, such as 0 to
        //  to 1, or 0 to 255.  (But use the same scale for all three!)
        //
        //  The "saturation" parameter works like this:
        //    0.0 creates a black-and-white image.
        //    0.5 reduces the color saturation by half.
        //    1.0 causes no change.
        //    2.0 doubles the color saturation.
        //  Note:  A "change" value greater than 1.0 may project your RGB values
        //  beyond their normal range, in which case you probably should truncate
        //  them to the desired range before trying to use them in an image.

        r       = &texture_reflectivity[i][0];
        g       = &texture_reflectivity[i][1];
        b       = &texture_reflectivity[i][2];

        float P = sqrt(
            (*r) * (*r) * Pr +
            (*g) * (*g) * Pg +
            (*b) * (*b) * Pb);

        *r = BOUND(0, P + (*r - P) * saturation, 255);
        *g = BOUND(0, P + (*g - P) * saturation, 255);
        *b = BOUND(0, P + (*b - P) * saturation, 255);

        qprintf("tex %i (%s) avg rgb [ %f, %f, %f ]\n",
                i, path, texture_reflectivity[i][0],
                texture_reflectivity[i][1], texture_reflectivity[i][2]);
    }

    if(palette_frompak) {
        free(palette_frompak);
    } else {
        free(palette);
    }
}

/*
=======================================================================

MAKE FACES

=======================================================================
*/

/*
=============
WindingFromFace
=============
*/
winding_t *WindingFromFaceX(dface_tx *f) {
    int32_t i;
    int32_t se;
    dvertex_t *dv;
    int32_t v;
    winding_t *w;

    w            = AllocWinding(f->numedges);
    w->numpoints = f->numedges;

    for (i = 0; i < f->numedges; i++) {
        se = dsurfedges[f->firstedge + i];

        if (se < 0)
            v = dedgesX[-se].v[1];
        else
            v = dedgesX[se].v[0];

        dv = &dvertexes[v];
        VectorCopy(dv->point, w->p[i]);
    }

    RemoveColinearPoints(w);

    return w;
}

winding_t *WindingFromFace(dface_t *f) {
    int32_t i;
    int32_t se;
    dvertex_t *dv;
    int32_t v;
    winding_t *w;

    w            = AllocWinding(f->numedges);
    w->numpoints = f->numedges;

    for (i = 0; i < f->numedges; i++) {
        se = dsurfedges[f->firstedge + i];
        if (se < 0)
            v = dedges[-se].v[1];
        else
            v = dedges[se].v[0];

        dv = &dvertexes[v];
        VectorCopy(dv->point, w->p[i]);
    }

    RemoveColinearPoints(w);

    return w;
}

/*
=============
BaseLightForFace
=============
*/
void BaseLightForFaceX(dface_tx *f, vec3_t color) {
    texinfo_t *tx;

    //
    // check for light emited by texture
    //
    tx = &texinfo[f->texinfo];
    if (!(tx->flags & SURF_LIGHT) || tx->value == 0) {
        if (tx->flags & SURF_LIGHT) {
            printf("Surface light has 0 intensity.\n");
        }
        VectorClear(color);
        return;
    }
    VectorScale(texture_reflectivity[f->texinfo], tx->value, color);
}

void BaseLightForFaceI(dface_t *f, vec3_t color) {
    texinfo_t *tx;

    //
    // check for light emited by texture
    //
    tx = &texinfo[f->texinfo];
    if (!(tx->flags & SURF_LIGHT) || tx->value == 0) {
        if (tx->flags & SURF_LIGHT) {
            printf("Surface light has 0 intensity.\n");
        }
        VectorClear(color);
        return;
    }
    VectorScale(texture_reflectivity[f->texinfo], tx->value, color);
}

qboolean IsSkyX(dface_tx *f) {
    texinfo_t *tx;

    tx = &texinfo[f->texinfo];
    if (tx->flags & SURF_SKY)
        return true;
    return false;
}

qboolean IsSkyI(dface_t *f) {
    texinfo_t *tx;

    tx = &texinfo[f->texinfo];
    if (tx->flags & SURF_SKY)
        return true;
    return false;
}

/*
=============
MakePatchForFace
=============
*/
float totalarea;
void MakePatchForFace(int32_t fn, winding_t *w) {
    float area;
    patch_t *patch;
    dplane_t *pl;
    int32_t i;
    vec3_t color = {1.0f, 1.0f, 1.0f};

    area         = WindingArea(w);
    totalarea += area;

    patch = &patches[num_patches];
    if (use_qbsp) {
        if (num_patches == MAX_PATCHES_QBSP)
            Error("Exceeded MAX_PATCHES_QBSP %i", MAX_PATCHES_QBSP);
    } else if (num_patches == MAX_PATCHES)
        Error("Exceeded MAX_PATCHES %i", MAX_PATCHES);
    patch->next      = face_patches[fn];
    face_patches[fn] = patch;

    patch->winding   = w;

    if (use_qbsp) {
        dface_tx *f;
        dleaf_tx *leaf;

        f = &dfacesX[fn];
        if (f->side)
            patch->plane = &backplanes[f->planenum];
        else
            patch->plane = &dplanes[f->planenum];
        if (face_offset[fn][0] || face_offset[fn][1] || face_offset[fn][2]) {
            // origin offset faces must create new planes
            if (use_qbsp) {
                if (numplanes + fakeplanes >= MAX_MAP_PLANES_QBSP)
                    Error("numplanes + fakeplanes >= MAX_MAP_PLANES_QBSP");
            } else if (numplanes + fakeplanes >= MAX_MAP_PLANES)
                Error("numplanes + fakeplanes >= MAX_MAP_PLANES");

            pl = &dplanes[numplanes + fakeplanes];
            fakeplanes++;

            *pl = *(patch->plane);
            pl->dist += DotProduct(face_offset[fn], pl->normal);
            patch->plane = pl;
        }

        WindingCenter(w, patch->origin);
        VectorAdd(patch->origin, patch->plane->normal, patch->origin);
        leaf           = RadPointInLeafX(patch->origin);
        patch->cluster = leaf->cluster;

        if (patch->cluster == -1) {
            // qprintf ("patch->cluster == -1\n");
            ++cluster_neg_one;
        }

        patch->faceNumber = fn; // qb: for patch sorting
        patch->area       = area;
        if (patch->area <= 1)
            patch->area = 1;
        patch->sky = IsSkyX(f);

        VectorCopy(texture_reflectivity[f->texinfo], patch->reflectivity);

        // non-bmodel patches can emit light
        if (fn < dmodels[0].numfaces) {
            BaseLightForFaceX(f, patch->baselight);

            ColorNormalize(patch->reflectivity, color);

            for (i = 0; i < 3; i++)
                patch->baselight[i] *= color[i];

            VectorCopy(patch->baselight, patch->totallight);
        }
    } else {
        dface_t *f;
        dleaf_t *leaf;

        f = &dfaces[fn];
        if (f->side)
            patch->plane = &backplanes[f->planenum];
        else
            patch->plane = &dplanes[f->planenum];
        if (face_offset[fn][0] || face_offset[fn][1] || face_offset[fn][2]) {
            // origin offset faces must create new planes
            if (use_qbsp) {
                if (numplanes + fakeplanes >= MAX_MAP_PLANES_QBSP)
                    Error("numplanes + fakeplanes >= MAX_MAP_PLANES_QBSP");
            } else if (numplanes + fakeplanes >= MAX_MAP_PLANES)
                Error("numplanes + fakeplanes >= MAX_MAP_PLANES");

            pl = &dplanes[numplanes + fakeplanes];
            fakeplanes++;

            *pl = *(patch->plane);
            pl->dist += DotProduct(face_offset[fn], pl->normal);
            patch->plane = pl;
        }

        WindingCenter(w, patch->origin);
        VectorAdd(patch->origin, patch->plane->normal, patch->origin);
        leaf           = RadPointInLeaf(patch->origin);
        patch->cluster = leaf->cluster;

        if (patch->cluster == -1) {
            // qprintf ("patch->cluster == -1\n");
            ++cluster_neg_one;
        }

        patch->faceNumber = fn; // qb: for patch sorting
        patch->area       = area;
        if (patch->area <= 1)
            patch->area = 1;
        patch->sky = IsSkyI(f);

        VectorCopy(texture_reflectivity[f->texinfo], patch->reflectivity);

        // non-bmodel patches can emit light
        if (fn < dmodels[0].numfaces) {
            BaseLightForFaceI(f, patch->baselight);

            ColorNormalize(patch->reflectivity, color);

            for (i = 0; i < 3; i++)
                patch->baselight[i] *= color[i];

            VectorCopy(patch->baselight, patch->totallight);
        }
    }
    num_patches++;
}

entity_t *EntityForModel(int32_t modnum) {
    int32_t i;
    char *s;
    char name[16];

    sprintf(name, "*%i", modnum);
    // search the entities for one using modnum
    for (i = 0; i < num_entities; i++) {
        s = ValueForKey(&entities[i], "model");
        if (!strcmp(s, name))
            return &entities[i];
    }

    return &entities[0];
}

/*
=============
MakePatches
=============
*/
void MakePatches(void) {
    int32_t i, j, k;
    int32_t fn;
    winding_t *w;
    dmodel_t *mod;
    vec3_t origin;
    entity_t *ent;

    qprintf("%i faces\n", numfaces);

    for (i = 0; i < nummodels; i++) {
        mod = &dmodels[i];
        ent = EntityForModel(i);
        // bmodels with origin brushes need to be offset into their
        // in-use position
        GetVectorForKey(ent, "origin", origin);
        // VectorCopy (vec3_origin, origin);

        for (j = 0; j < mod->numfaces; j++) {
            fn              = mod->firstface + j;
            face_entity[fn] = ent;
            VectorCopy(origin, face_offset[fn]);
            if (use_qbsp) {
                dface_tx *f;
                f = &dfacesX[fn];
                w = WindingFromFaceX(f);
            } else {
                dface_t *f;
                f = &dfaces[fn];
                w = WindingFromFace(f);
            }

            for (k = 0; k < w->numpoints; k++) {
                VectorAdd(w->p[k], origin, w->p[k]);
            }
            MakePatchForFace(fn, w);
        }
    }

    qprintf("%i sqaure feet\n", (int32_t)(totalarea / 64));
}

/*
=======================================================================

SUBDIVIDE

=======================================================================
*/

void FinishSplit(patch_t *patch, patch_t *newp) {
    VectorCopy(patch->baselight, newp->baselight);
    VectorCopy(patch->totallight, newp->totallight);
    VectorCopy(patch->reflectivity, newp->reflectivity);
    newp->plane = patch->plane;
    newp->sky   = patch->sky;

    patch->area = WindingArea(patch->winding);
    newp->area  = WindingArea(newp->winding);

    if (patch->area <= 1)
        patch->area = 1;
    if (newp->area <= 1)
        newp->area = 1;

    if (use_qbsp) {
        dleaf_tx *leaf;
        WindingCenter(patch->winding, patch->origin);
        VectorAdd(patch->origin, patch->plane->normal, patch->origin);
        leaf           = RadPointInLeafX(patch->origin);
        patch->cluster = leaf->cluster;
        if (patch->cluster == -1)
            qprintf("patch->cluster == -1\n");

        WindingCenter(newp->winding, newp->origin);
        VectorAdd(newp->origin, newp->plane->normal, newp->origin);
        leaf          = RadPointInLeafX(newp->origin);
        newp->cluster = leaf->cluster;
        if (newp->cluster == -1)
            qprintf("patch->cluster == -1\n");
    } else {
        dleaf_t *leaf;
        WindingCenter(patch->winding, patch->origin);
        VectorAdd(patch->origin, patch->plane->normal, patch->origin);
        leaf           = RadPointInLeaf(patch->origin);
        patch->cluster = leaf->cluster;
        if (patch->cluster == -1)
            qprintf("patch->cluster == -1\n");

        WindingCenter(newp->winding, newp->origin);
        VectorAdd(newp->origin, newp->plane->normal, newp->origin);
        leaf          = RadPointInLeaf(newp->origin);
        newp->cluster = leaf->cluster;
        if (newp->cluster == -1)
            qprintf("patch->cluster == -1\n");
    }
}

/*
=============
SubdividePatch

Chops the patch only if its local bounds exceed the max size
=============
*/
void SubdividePatch(patch_t *patch) {
    winding_t *w, *o1, *o2;
    vec3_t mins, maxs, total;
    vec3_t split;
    vec_t dist;
    int32_t i, j;
    vec_t v;
    patch_t *newp;

    w       = patch->winding;
    mins[0] = mins[1] = mins[2] = BOGUS_RANGE;
    maxs[0] = maxs[1] = maxs[2] = -BOGUS_RANGE;
    for (i = 0; i < w->numpoints; i++) {
        for (j = 0; j < 3; j++) {
            v = w->p[i][j];
            if (v < mins[j])
                mins[j] = v;
            if (v > maxs[j])
                maxs[j] = v;
        }
    }
    VectorSubtract(maxs, mins, total);
    for (i = 0; i < 3; i++)
        if (total[i] > (subdiv + 1))
            break;
    if (i == 3) {
        // no splitting needed
        return;
    }

    //
    // split the winding
    //
    VectorCopy(vec3_origin, split);
    split[i] = 1;
    dist     = (mins[i] + maxs[i]) * 0.5;
    ClipWindingEpsilon(w, split, dist, ON_EPSILON, &o1, &o2);

    //
    // create a new patch
    //
    if (use_qbsp) {
        if (num_patches == MAX_PATCHES_QBSP)
            Error("Exceeded MAX_PATCHES_QBSP %i", MAX_PATCHES_QBSP);
    } else if (num_patches == MAX_PATCHES)
        Error("Exceeded MAX_PATCHES %i", MAX_PATCHES);

    newp = &patches[num_patches];
    num_patches++;

    newp->next     = patch->next;
    patch->next    = newp;

    patch->winding = o1;
    newp->winding  = o2;

    FinishSplit(patch, newp);

    SubdividePatch(patch);
    SubdividePatch(newp);
}

/*
=============
DicePatch

Chops the patch by a global grid
=============
*/
void DicePatch(patch_t *patch) {
    winding_t *w, *o1, *o2;
    vec3_t mins, maxs;
    vec3_t split;
    vec_t dist;
    int32_t i;
    patch_t *newp;

    w = patch->winding;
    WindingBounds(w, mins, maxs); // 3D AABB for polygon
    for (i = 0; i < 3; i++)
        if (floor((mins[i] + 1) / subdiv) < floor((maxs[i] - 1) / subdiv))
            break;
    if (i == 3) {
        // no splitting needed
        return;
    }

    //
    // split the winding
    //
    VectorCopy(vec3_origin, split);
    split[i] = 1;
    dist     = subdiv * (1 + floor((mins[i] + 1) / subdiv));
    ClipWindingEpsilon(w, split, dist, ON_EPSILON, &o1, &o2);

    //
    // create a new patch
    //
    if (use_qbsp) {
        if (num_patches == MAX_PATCHES_QBSP)
            Error("Exceeded MAX_PATCHES_QBSP %i", MAX_PATCHES_QBSP);
    } else if (num_patches == MAX_PATCHES)
        Error("Exceeded MAX_PATCHES %i", MAX_PATCHES);
    newp = &patches[num_patches];
    num_patches++;

    newp->next     = patch->next;
    patch->next    = newp;

    patch->winding = o1;
    newp->winding  = o2;

    FinishSplit(patch, newp);

    DicePatch(patch);
    DicePatch(newp);
}

/*
=============
SubdividePatches
=============
*/
void SubdividePatches(void) {
    int32_t i, num;

    if (subdiv < 1)
        return;

    num = num_patches; // because the list will grow
    for (i = 0; i < num; i++) {
        if (dicepatches)
            DicePatch(&patches[i]);
        else
            SubdividePatch(&patches[i]);
    }
    for (i = 0; i < num_patches; i++)
        patches[i].nodenum = PointInNodenum(patches[i].origin);
    printf("%i subdiv patches\n", num_patches);
    printf("-------------------------\n");

    qprintf("[? patch->cluster=-1 count is %i  ?in solid leaf?]\n", cluster_neg_one);
}

//=====================================================================
