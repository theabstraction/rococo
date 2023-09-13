/*
 * jdatastream.c
 *
 * Copyright (C) 2013, Mark Anthony Taylor.
 */

/* this is not a core library module, so it doesn't define JPEG_INTERNALS */
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"

#include "jdatastream.h"

typedef struct 
{
	struct jpeg_source_mgr publicFields;
	struct DataStream* dataStream;
} InMemorySourceManager;

METHODDEF(void) init_source (j_decompress_ptr cinfo)
{
	InMemorySourceManager* src = (InMemorySourceManager*) cinfo->src;
	src->dataStream->readPosition = src->dataStream->sourceData;
}

METHODDEF(boolean) fill_input_buffer (j_decompress_ptr cinfo)
{
	static unsigned char s_endEOI[2] = { 0xFF, 0xD9 };

	InMemorySourceManager* src = (InMemorySourceManager*) cinfo->src;

	if (src->dataStream->dataLengthBytes == 0)
	{
		ERREXIT(cinfo, JERR_INPUT_EMPTY);
		WARNMS(cinfo, JWRN_JPEG_EOF);

		src->publicFields.next_input_byte = s_endEOI;
		src->publicFields.bytes_in_buffer = 2;	
	}
	else if (src->dataStream->readPosition >= src->dataStream->end)
	{
		WARNMS(cinfo, JWRN_JPEG_EOF);

		src->publicFields.next_input_byte = s_endEOI;
		src->publicFields.bytes_in_buffer = 2;	
	}
	else
	{
		size_t nBytesToRead = src->dataStream->end - src->dataStream->readPosition;
		if (nBytesToRead > 65536) nBytesToRead = 65536;

		src->publicFields.bytes_in_buffer = nBytesToRead;
		src->publicFields.next_input_byte = src->dataStream->readPosition;

		src->dataStream->readPosition += nBytesToRead;
	}

	return TRUE;
}


METHODDEF(void) skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	InMemorySourceManager* src = (InMemorySourceManager*) cinfo->src;

	if (num_bytes > 0)
	{
		while (num_bytes > (long) src->publicFields.bytes_in_buffer)
		{
			num_bytes -= (long) src->publicFields.bytes_in_buffer;
			fill_input_buffer(cinfo);
		}
		src->publicFields.next_input_byte += (size_t) num_bytes;
		src->publicFields.bytes_in_buffer -= (size_t) num_bytes;
	}
}

METHODDEF(void) term_source (j_decompress_ptr cinfo)
{
}

GLOBAL(void) jpeg_inmemory_source (j_decompress_ptr cinfo, struct DataStream *dataStream)
{
	InMemorySourceManager* src;

	if (cinfo->src == NULL)
	{
		cinfo->src = (struct jpeg_source_mgr *) (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, SIZEOF(InMemorySourceManager));
	}

	src = (InMemorySourceManager*) cinfo->src;

	src->publicFields.init_source = init_source;
	src->publicFields.fill_input_buffer = fill_input_buffer;
	src->publicFields.skip_input_data = skip_input_data;
	src->publicFields.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->publicFields.term_source = term_source;
	src->dataStream = dataStream;
	src->publicFields.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->publicFields.next_input_byte = NULL; /* until buffer loaded */
}

