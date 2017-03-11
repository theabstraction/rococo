#ifndef ROCOCO_IMAGING_H
#define ROCOCO_IMAGING_H

#ifndef Rococo_TYPES_H
#error include <rococo.types.h> before including this file
#endif

namespace Rococo
{
	namespace Imaging
	{
		struct F_A8R8G8B8
		{
			unsigned char b;
			unsigned char g;
			unsigned char r;
			unsigned char a;
		};

		struct IImageLoadEvents
		{
			virtual void OnError(const char* message) = 0;
			virtual void OnARGBImage(const Vec2i& span, const F_A8R8G8B8* data) = 0;
			virtual void OnAlphaImage(const Vec2i& span, const uint8* data) = 0;
		};
		
		bool CompressJPeg(const unsigned char* data, const Vec2i& span, const char* filename, int quality);
		bool CompressJPeg(const unsigned char* data, const Vec2i& span, const wchar_t* filename, int quality);

		bool DecompressJPeg(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes);
		bool DecompressTiff(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes);

      void SetTiffAllocator(IAllocator* _allocator);
      void SetJpegAllocator(IAllocator* _allocator);
	}
}

#endif