#ifndef ROCOCO_IMAGING_H
#define ROCOCO_IMAGING_H

#ifndef Rococo_TYPES_H
#error include <rococo.types.h> before including this file
#endif

namespace Rococo
{
	namespace Imaging
	{
		struct IImageLoadEvents
		{
			virtual void OnError(const char* message) = 0;
			virtual void OnRGBAImage(const Vec2i& span, const RGBAb* data) = 0;
			virtual void OnAlphaImage(const Vec2i& span, const uint8* data) = 0;
		};
		
		void SaveAsTiff(const uint8* grayScale, const Vec2i& span, const char* filename);
		bool CompressJPeg(const unsigned char* data, const Vec2i& span, const char* filename, int quality);
		bool CompressJPeg(const unsigned char* data, const Vec2i& span, cstr filename, int quality);

		bool DecompressJPeg(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes);
		bool DecompressTiff(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes);

		void SetTiffAllocator(IAllocator* _allocator);
		void SetJpegAllocator(IAllocator* _allocator);
	}
}

#endif