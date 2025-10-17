// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once
#ifndef ROCOCO_IMAGING_H
#define ROCOCO_IMAGING_H

#ifndef Rococo_TYPES_H
#error include <rococo.types.h> before including this file
#endif

#ifndef ROCOCO_JPEG_API
# error "define ROCOCO_JPEG_API in your compile environment"
#endif

#ifndef ROCOCO_TIFF_API
# error "define ROCOCO_TIFF_API in your compile environment"
#endif

#include <rococo.types.h>

namespace Rococo::Imaging
{
	template<typename COLOUR_STRUCT>
	ROCOCO_INTERFACE IImagePopulator
	{
		virtual void OnImage(const COLOUR_STRUCT * pixelBuffer, int width, int height) = 0;
	};

	struct IImageLoadEvents
	{
		virtual void OnError(const char* message) = 0;
		virtual void OnRGBAImage(const Vec2i& span, const RGBAb* data) = 0;
		virtual void OnAlphaImage(const Vec2i& span, const uint8* data) = 0;
	};
		
	ROCOCO_TIFF_API void CompressTiff(const RGBAb* data, Vec2i span, const char* filename);
	ROCOCO_TIFF_API void CompressTiff(const uint8* grayScale, Vec2i span, const char* filename);

	/* quality: 1 to 100*/
	ROCOCO_JPEG_API void CompressJPeg(const RGBAb* data, Vec2i span, cstr filename, int quality);

	/* quality: 1 to 100*/
	ROCOCO_JPEG_API void CompressJPeg(const RGBb* data, Vec2i span, cstr filename, int quality);

	// Can throw if the loadEvents handlers can throw
	ROCOCO_JPEG_API bool DecompressJPeg(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes);

	// Can throws if the loadEvents handlers can throw
	ROCOCO_TIFF_API bool DecompressTiff(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes);

	ROCOCO_TIFF_API void SetTiffAllocator(IAllocator* _allocator);
	ROCOCO_JPEG_API void SetJpegAllocator(IAllocator* _allocator);
}

#endif