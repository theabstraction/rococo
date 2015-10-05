extern "C"
{
	#include "jinclude.h"
	#include "jpeglib.h"
	#include "jerror.h"

	#include <setjmp.h>

	#include "jdatastream.h"
}

#include <bloke.types.h>
#include <bloke.imaging.h>

struct ErrorManager
{
	struct jpeg_error_mgr publicFields;
	jmp_buf setjmpBuffer;
};

static void OnErrorExit (j_common_ptr cinfo)
{
	struct ErrorManager* myerr = (struct ErrorManager*) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmpBuffer, 1);
}

namespace Bloke
{
	namespace Imaging
	{
		bool DecompressJPeg(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes)
		{
			DataStream stream;
			stream.dataLengthBytes = dataLengthBytes;
			stream.sourceData = sourceBuffer;
			stream.readPosition = sourceBuffer;
			stream.end = sourceBuffer + dataLengthBytes;

			unsigned char* contigBuffer = NULL;
			jpeg_decompress_struct cinfo;
			ErrorManager errorManager;

			cinfo.err = jpeg_std_error(&errorManager.publicFields);
			errorManager.publicFields.error_exit = OnErrorExit;

			if (setjmp(errorManager.setjmpBuffer))
			{
				if (contigBuffer) delete[] contigBuffer;
				jpeg_destroy_decompress(&cinfo);
				return false;
			}

			bool isOK = true;

			jpeg_create_decompress(&cinfo);
			jpeg_inmemory_source(&cinfo, &stream);
			jpeg_read_header(&cinfo, TRUE);

			if (cinfo.image_height < 1 || cinfo.image_height > 4096)
			{
				loadEvents.OnError("Image height was outside range 1...2048 pixels");
				isOK = false;
			}

			if (cinfo.image_width < 1 || cinfo.image_width > 4096)
			{
				loadEvents.OnError("Image width was outside range 1...2048 pixels");
				isOK = false;
			}

			if (cinfo.num_components != 3)
			{
				loadEvents.OnError("Image colour components was neither 3 or 4");
				isOK = false;
			}

			if (cinfo.out_color_space != JCS_RGB)
			{
				loadEvents.OnError("Image colour space was not RGB");
				isOK = false;
			}

			if (isOK)
			{
				jpeg_start_decompress(&cinfo);

				int stride = cinfo.image_width * sizeof(__int32);
				JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr)&cinfo, JPOOL_IMAGE, cinfo.image_width * cinfo.num_components, 1);

				unsigned char* contigBuffer = new unsigned char[cinfo.image_height * stride];

				while (isOK && cinfo.output_scanline < cinfo.output_height)
				{
					int line = cinfo.output_scanline;
					jpeg_read_scanlines(&cinfo, buffer, 1);

#pragma pack(push,1)
					struct RGB
					{
						unsigned char r;
						unsigned char g;
						unsigned char b;
					};
#pragma pack(pop)

					RGB* inputBuffer = (RGB*)*buffer;
					F_A8R8G8B8* outputBuffer = (F_A8R8G8B8*)(contigBuffer + stride * line);

					for (JDIMENSION i = 0; i < cinfo.image_width; ++i)
					{
						outputBuffer->r = inputBuffer->r;
						outputBuffer->g = inputBuffer->g;
						outputBuffer->b = inputBuffer->b;
						outputBuffer->a = 255;

						outputBuffer++;
						inputBuffer++;
					}
				}

				loadEvents.OnARGBImage(Vec2i(cinfo.image_width, cinfo.image_height), (const F_A8R8G8B8*)contigBuffer);

				if (contigBuffer) delete[] contigBuffer;

				jpeg_finish_decompress(&cinfo);
			}
			jpeg_destroy_decompress(&cinfo);

			return isOK;
		}
	}
}