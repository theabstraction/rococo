extern "C"
{
	#include <jinclude.h>
	#include <jpeglib.h>
	#include <jerror.h>

	#include <setjmp.h>

	#include <jdatastream.h>
}

#define ROCOCO_API // we want to avoid linking in the Rococo.util lib, so will define functions in this source code
#include <rococo.types.h>
#include <rococo.imaging.h>

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

namespace
{
   Rococo::IAllocator* allocator = nullptr;

   void* Allocate(size_t capacity)
   {
      if (allocator)
      {
         return allocator->Allocate(capacity);
      }
      else
      {
         return malloc(capacity);
      }
   }

   void Delete(void* ptr)
   {
      if (allocator)
      {
         allocator->FreeData(ptr);
      }
      else
      {
         free(ptr);
      }
   }
}

extern "C"
{
   JPEG_GLOBAL_API void *
      jpeg_get_small(j_common_ptr cinfo, size_t sizeofobject)
   {
      return (void *)Allocate(sizeofobject);
   }

   JPEG_GLOBAL_API void
      jpeg_free_small(j_common_ptr cinfo, void * object, size_t sizeofobject)
   {
      Delete(object);
   }

   JPEG_GLOBAL_API void *
      jpeg_get_large(j_common_ptr cinfo, size_t sizeofobject)
   {
      return (void *)Allocate(sizeofobject);
   }

   JPEG_GLOBAL_API void
      jpeg_free_large(j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
   {
      Delete(object);
   }
}

namespace Rococo::Imaging
{
	ROCOCO_JPEG_API void SetJpegAllocator(IAllocator* _allocator)
	{
		allocator = _allocator;
	}

	ROCOCO_JPEG_API bool DecompressJPeg(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes)
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
			if (contigBuffer) Delete(contigBuffer);
			jpeg_destroy_decompress(&cinfo);
			loadEvents.OnError("JPEG was bad or unrecognized format");
			return false;
		}

		jpeg_create_decompress(&cinfo);
		jpeg_inmemory_source(&cinfo, &stream);
		jpeg_read_header(&cinfo, TRUE);

		if (cinfo.image_height < 1 || cinfo.image_height > 8192)
		{
			loadEvents.OnError("Image height was outside range 1...8192 pixels");
			return false;
		}

		if (cinfo.image_width < 1 || cinfo.image_width > 8192)
		{
			loadEvents.OnError("Image width was outside range 1...8192 pixels");
			return false;
		}

		if (cinfo.num_components != 3)
		{
			loadEvents.OnError("Image colour components was neither 3 or 4");
			return false;
		}

		if (cinfo.out_color_space != JCS_RGB)
		{
			loadEvents.OnError("Image colour space was not RGB");
			return false;
		}

		jpeg_start_decompress(&cinfo);

		int stride = cinfo.image_width * 4;
		JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr)&cinfo, JPOOL_IMAGE, cinfo.image_width * cinfo.num_components, 1);

		contigBuffer = (uint8*)Allocate(cinfo.image_height * stride);

		while (cinfo.output_scanline < cinfo.output_height)
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
			RGBAb* outputBuffer = (RGBAb*)(contigBuffer + stride * line);

			for (JDIMENSION i = 0; i < cinfo.image_width; ++i)
			{
				outputBuffer->red = inputBuffer->r;
				outputBuffer->green = inputBuffer->g;
				outputBuffer->blue = inputBuffer->b;
				outputBuffer->alpha = 255;

				outputBuffer++;
				inputBuffer++;
			}
		}

		loadEvents.OnRGBAImage(Vec2i{ (int32)cinfo.image_width,(int32)cinfo.image_height }, (const RGBAb*)contigBuffer);

		Delete(contigBuffer);

		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);

		return true;
	}
}