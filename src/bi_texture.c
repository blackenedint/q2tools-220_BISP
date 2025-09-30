#include "bi_texture.h"

size_t colorformatsize( btf_format format, int32_t width, int32_t height ) {
	switch(format) {
		default:
		case FMT_RGBA: return 4 * width * height;
		case FMT_RGB: return 3 * width * height;
	}

	// uh oh.
	return 0;
}

qboolean BTFLoadFromBuffer( byte* buffer, size_t buffersize, qboolean include_colordata, btf_texture_t* outTexture)
{
	byte* pbuf = buffer;
	const btf_header_t* hdr = (const btf_header_t*)pbuf;
	if ( LittleLong(hdr->ident) != BTF_IDENT ) {
		qprintf("invalid btf header %i\n", hdr->ident );
		return false;
	}
	const int16_t vmaj = LittleShort(hdr->ver_major);
	const int16_t vmin = LittleShort(hdr->ver_minor);

	if ( BTFVersion(vmaj, vmin) > BTFHighest() ) {
		qprintf("unsupported btf version: %i.%i\n", vmaj, vmin);
		return false;
	}
	pbuf += sizeof(btf_header_t);

	// get the texture info now.
	if ((size_t)(pbuf - buffer) + sizeof(btf_texinfo_t) > buffersize)
		return false;

	const btf_texinfo_t* tnfo = (const btf_texinfo_t*)pbuf;
	const int32_t width         = LittleLong(tnfo->width);
	const int32_t height        = LittleLong(tnfo->height);
	const int16_t compression   = LittleShort(tnfo->compressiontype);
	const int16_t format        = LittleShort(tnfo->format);
	const int16_t animType      = LittleShort(tnfo->animType);
	const int16_t frame_count   = LittleShort(tnfo->frame_count);

	const uint32_t fds = (uint32_t)LittleLong(tnfo->framedatasize);
	const uint32_t fdo = (uint32_t)LittleLong(tnfo->framedataoffset);
	const uint32_t mds = (uint32_t)LittleLong(tnfo->metadatasize);
	const uint32_t mdo = (uint32_t)LittleLong(tnfo->metadataoffset);
	//printf("fds: %i, fdo: %i, mds: %i, mdo: %i\n", tnfo->framedatasize, tnfo->framedataoffset, tnfo->metadatasize, tnfo->metadataoffset);

	if (fds && ((size_t)fdo + (size_t)fds > buffersize)) {
		qprintf("bad frame size\n");
		return false;
	}
	if (fdo && ((size_t)fdo > buffersize)) {
		qprintf("bad frame offset\n");
		return false;
	}
	if (mds && ((size_t)mdo + (size_t)mds > buffersize)) {
		qprintf("bad meta size\n");
		return false;
	}
	if (mdo && ((size_t)mdo > buffersize)) {
		qprintf("bad meta offset\n");
		return false;
	}

	outTexture->width = width;
	outTexture->height = height;
	outTexture->surfaceflags = 0;
	outTexture->contents = 0;
	outTexture->value = 0;
	
	if (mds > 0 && mdo > 0) {
		const unsigned char* mp = buffer + mdo;
		const unsigned char* mend = mp + mds;

		if ((size_t)(mp - buffer) + sizeof(uint32_t) <= buffersize) {
			const uint32_t ident = LittleLong(*(const uint32_t*)mp);
			if ( ident== BTF_METAQ2 ) {
				if ((size_t)(mp - buffer) + sizeof(btf_q2meta_t) <= buffersize) {
					const btf_q2meta_t* q2 = (const btf_q2meta_t*)mp;
					outTexture->surfaceflags = LittleLong(q2->surfaceflags);
					outTexture->contents     = LittleLong(q2->contents);
					outTexture->value        = (int32_t)LittleFloat(q2->value);					
				}
			}
		}
	} 

	if ( include_colordata ) {
		if (fds == 0 || fdo == 0) { 
			qprintf("no frame data\n"); 
			return false; 
		}
        const byte* fp   = buffer + fdo;
        const byte* fend = fp + fds;

		if ((size_t)(fp - buffer) + sizeof(btf_frame_t) > buffersize) {
			qprintf("frame header out of bounds\n"); 
			return false; 
		}

        const btf_frame_t* fr = (const btf_frame_t*)fp;
        const uint32_t frIdent = LittleLong(fr->ident);
		if ( frIdent!= BTF_FRAMEID ) {
			printf( "invalid frame ident: %i\n", frIdent );
			return false;
		}
		fp += sizeof(btf_frame_t);

		const size_t frameBytes = colorformatsize( tnfo->format, tnfo->width, tnfo->height );
        if ((size_t)(fp - buffer) + frameBytes > buffersize || fp + frameBytes > fend) {
            qprintf("truncated frame payload\n"); 
			return false;
        }		
		outTexture->colorformat = tnfo->format;
		outTexture->colordatasize = frameBytes;
		outTexture->colordata = (byte*)malloc(frameBytes);
		memcpy(outTexture->colordata, pbuf, frameBytes);
	}

	return true;
}