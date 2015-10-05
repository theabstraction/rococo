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

namespace
{
	struct ErrorManager
	{
		struct jpeg_error_mgr publicFields;
		jmp_buf setjmpBuffer;
	};

	static void OnErrorExit(j_common_ptr cinfo)
	{
		struct ErrorManager* myerr = (struct ErrorManager*) cinfo->err;

		/* Always display the message. */
		/* We could postpone this until after returning, if we chose. */
		(*cinfo->err->output_message) (cinfo);

		/* Return control to the setjmp point */
		longjmp(myerr->setjmpBuffer, 1);
	}

	class AutoFileHandle
	{
		FILE* file;
	public:
		AutoFileHandle(const char* filename)
		{
			errno_t status = fopen_s(&file, (const char*)filename, "wb");
			if (status != 0)
			{
				file = NULL;
			}
		}

		AutoFileHandle(const wchar_t* filename)
		{
			errno_t status = _wfopen_s(&file, (const wchar_t*)filename, L"wb");
			if (status != 0)
			{
				file = NULL;
			}
		}

		~AutoFileHandle()
		{
			if (file != NULL) fclose(file);
		}

		FILE* operator * () { return file; }
	};

	bool CompressJPeg(const unsigned char* data, unsigned int width, unsigned int height, int quality, FILE* output)
	{
		jpeg_compress_struct cinfo;
		ErrorManager errorManager;

		cinfo.err = jpeg_std_error(&errorManager.publicFields);
		errorManager.publicFields.error_exit = OnErrorExit;

		if (setjmp(errorManager.setjmpBuffer))
		{
			jpeg_destroy_compress(&cinfo);
			return false;
		}

		jpeg_create_compress(&cinfo);
		jpeg_stdio_dest(&cinfo, output);

		cinfo.image_width = width;
		cinfo.image_height = height;
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;

		jpeg_set_defaults(&cinfo);

		cinfo.dct_method = JDCT_FLOAT;
		jpeg_set_quality(&cinfo, quality, TRUE);

		jpeg_start_compress(&cinfo, TRUE);

		JSAMPARRAY imgData = (JSAMPARRAY)data;

		for (unsigned int i = 0; i < height; ++i)
		{
			JSAMPROW row = (JSAMPROW)(data + i * width * 3);
			jpeg_write_scanlines(&cinfo, &row, 1);
		}

		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);

		return true;
	}
}

namespace Bloke { namespace Imaging
{
	bool CompressJPeg(const unsigned char* data, const Vec2i& span, const wchar_t* filename, int quality)
	{
		AutoFileHandle output(filename);
		if (!*output) return false;

		return ::CompressJPeg(data, span.x, span.y, quality, *output);
	}

	bool CompressJPeg(const unsigned char* data, const Vec2i& span, const char* filename, int quality)
	{
		AutoFileHandle output(filename);
		if (!*output) return false;

		return ::CompressJPeg(data, span.x, span.y, quality, *output);
	}
}}