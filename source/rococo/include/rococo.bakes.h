#pragma once
#include <rococo.types.h>
#include <rococo.io.h>

namespace Rococo::Bakes
{
	template <int a, int b, int c, int d>
	struct FourCC
	{
		static const unsigned int value = (((((d << 8) | c) << 8) | b) << 8) | a;
	};

	enum class CompressionType : uint32
	{
		RAW,
		TIFF,
		JPG
	};

	// Attempts to bake the directory at [mipMapDir] to a compressed mip-map chain archive-file.
	// The directory is expected to have extension '.mipmaps' Example 'rocks.mipmaps'
	// This consists of a number of images of square geometry (width = height) in which the span is a power of 2.
	// Only TIFF and JPG files are read. The height and width of files are validated, the span must match that taken from
	// the filenames. The format of the file names is %ux%u.%s where %s is one of {jpg,tif} and %u is an unsigned integer
	// power of 2. Example: 512x512.tif. Tif files take precedence over JPEG files.
	// 
	// These files are automatically generated by the rococo.texture.tool. Spans above the maximum are ignored. 8192 is the
	// upper limit. The [targetFilename] is optional and overrides the default target name.
	// 
	// The target name by default is generated by taking the source directory's path name, stripping the extension and 
	// adding a '.23x' 
	// On error the function throws an IException.
	ROCOCO_MISC_UTILS_API void BakeMipMappedTextureDirectory(cstr mipMapDir, uint32 maxSpan = 8192, cstr targetFilename = nullptr);

	struct MapMapDesc
	{
		// Gives the uncompressed data buffer
		const RGBAb* image;

		// Gives the compressed data buffer if not null and compressionType gives the file format
		const uint8* compressedData;

		// Gives the length of the image if uncompressed, else the length of the compressed data
		uint32 lengthInBytes;

		// Gives the span in pixels
		uint32 span;

		// Gives the number of pixels in the image buffer or the number of expected pixels in the compressed data.
		// Note that malicious attacker could stick a rogue TIFF or JPEG with something different than expected.
		uint32 nPixels;

		// Should be Raw, JPG or TIFF. use lib-jpg or lib-tiff to decode accordingly
		CompressionType compressionType;
	};

	// After loading a bake file, or part of a bake file, the caller passes a copy of the file data to this method and extracts a mip map level
	ROCOCO_MISC_UTILS_API MapMapDesc FindMipMapInBakedFile(const uint8* headerAndContent, size_t length, uint32 mipMapLevel);

	// Takes a .23x mip map file and extracts the images to a directory that has the file name sans extenson of the bakeFile23x name, with the extensiion .mipmaps
	ROCOCO_MISC_UTILS_API void ExtractAsImageListFromBakedFile(cstr bakeFile23x);
}