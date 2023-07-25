#ifndef ROCOCO_IMAGING_H
#define ROCOCO_IMAGING_H

#ifndef Rococo_TYPES_H
#error include <rococo.types.h> before including this file
#endif

#ifndef ROCOCO_JPEG_API
# define ROCOCO_JPEG_API ROCOCO_API_IMPORT
#endif

#ifndef ROCOCO_TIFF_API
# define ROCOCO_TIFF_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Imaging
{
	struct IImageLoadEvents
	{
		virtual void OnError(const char* message) = 0;
		virtual void OnRGBAImage(const Vec2i& span, const RGBAb* data) = 0;
		virtual void OnAlphaImage(const Vec2i& span, const uint8* data) = 0;
	};
		
	ROCOCO_TIFF_API void SaveAsTiff(const uint8* grayScale, const Vec2i& span, const char* filename);
	ROCOCO_JPEG_API bool CompressJPeg(const unsigned char* data, const Vec2i& span, cstr filename, int quality);

	// Can throw if the loadEvents handlers can throw
	ROCOCO_JPEG_API bool DecompressJPeg(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes);

	// Can throws if the loadEvents handlers can throw
	ROCOCO_TIFF_API bool DecompressTiff(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes);

	ROCOCO_TIFF_API void SetTiffAllocator(IAllocator* _allocator);
	ROCOCO_JPEG_API void SetJpegAllocator(IAllocator* _allocator);
}

#endif