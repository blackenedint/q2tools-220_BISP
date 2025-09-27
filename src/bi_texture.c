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
	byte* p = buffer;
	btf_header_t* hdr = (btf_header_t*)p;
	if ( LittleLong(hdr->ident) != BTF_IDENT ) {
		qprintf("invalid btf header %i\n", hdr->ident );
		return false;
	}

	if ( LittleShort(hdr->ver_major) != BTF_VER_MAJOR ||
		LittleShort(hdr->ver_minor) != BTF_VER_MINOR ) {
			qprintf("unsupported btf version: %i.%i\n", hdr->ver_major, hdr->ver_minor);
			return false;
	}
	p += sizeof(btf_header_t);

	// get the texture info now.
	btf_texinfo_t* tnfo = (btf_texinfo_t*)p;
	outTexture->width = LittleLong(tnfo->width);
	outTexture->height = LittleLong(tnfo->height);
	p += sizeof( btf_texinfo_t);

	btf_q2meta_t* q2Meta = NULL;
	if (tnfo->metadatasize > 0 && tnfo->metadatatype == BTF_METAQ2 ) {
		q2Meta = (btf_q2meta_t*)p;
		p += sizeof(btf_q2meta_t);

		// skip alternate texture names, we don't care.
		if ( q2Meta->alternate_count > 0 ) {
			p += q2Meta->alternate_count * MAX_BITEXTURE_NAME;
		}

		outTexture->surfaceflags = LittleLong(q2Meta->surfaceflags);
		outTexture->contents = LittleLong(q2Meta->contents);
		outTexture->value = (int32_t)LittleFloat(q2Meta->value);

	} else {
		outTexture->surfaceflags = 0;
		outTexture->contents = 0;
		outTexture->value = 0;
	}

	if ( include_colordata ) {
		btf_frame_t* frame = (btf_frame_t*)p;
		p+= sizeof(btf_frame_t);

		if ( frame->ident != BTF_FRAMEID ) {
			qprintf( "invalid frame ident: %i\n", frame->ident );
			return false;
		}
		outTexture->colorformat = tnfo->format;
		outTexture->colordatasize = colorformatsize( tnfo->format, tnfo->width, tnfo->height );
		outTexture->colordata = (byte*)malloc(outTexture->colordatasize);
		memcpy(outTexture->colordata, p, outTexture->colordatasize);
	}

	return true;
}