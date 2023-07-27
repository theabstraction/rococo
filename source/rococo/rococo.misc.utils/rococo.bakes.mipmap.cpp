#include <rococo.bakes.h>
#include <rococo.strings.h>
#include <rococo.imaging.h>
#include <rococo.os.h>
#include <vector>
#include <rococo.functional.h>
#include <rococo.imaging.h>

namespace Rococo::Bakes
{
	using TByteArray = std::vector<uint8>;

	using namespace Rococo::Strings;

	struct ImageLoader : Imaging::IImageLoadEvents
	{
		uint32 expectedSpan = 0;
		std::vector<RGBAb> decompressedImage;
		HString err;

		void OnError(const char* message) override
		{
			err = message;
		}

		void OnRGBAImage(const Vec2i& span, const RGBAb* data) override
		{
			if (span.x != span.y)
			{
				err = "Not a square image";
				return;
			}

			if (span.x != (int) expectedSpan)
			{
				err = "Not the expected span";
				return;
			}

			decompressedImage.resize(sizeof RGBAb * span.x * span.y);
			memcpy(decompressedImage.data(), data, decompressedImage.size());
		}

		void OnAlphaImage(const Vec2i& /* span */, const uint8* /* data */) override
		{
			err = "Image was alpha 8-bit, not an RGB / RGBA";
		}
	};

	enum { RAW_MAX_SPAN = 8 };

	void LoadBufferAsTiff(TByteArray& scratchBuffer, TByteArray& targetArray, uint32 span, cstr tifPath)
	{
		size_t len = OS::LoadAsciiTextFile((char*)scratchBuffer.data(), scratchBuffer.size(), tifPath);

		ImageLoader onLoad;
		onLoad.expectedSpan = span;

		Imaging::DecompressTiff(onLoad, scratchBuffer.data(), len);

		if (onLoad.err.length() > 0)
		{
			Throw(0, "%s: Error loading %s [%s]", __FUNCTION__, tifPath, onLoad.err.c_str());
		}

		if (span <= RAW_MAX_SPAN)
		{
			size_t nBytes = onLoad.decompressedImage.size() * sizeof(RGBAb);
			targetArray.resize(nBytes);
			memcpy(targetArray.data(), onLoad.decompressedImage.data(), nBytes);
		}
		else
		{
			targetArray.resize(len);
			memcpy(targetArray.data(), scratchBuffer.data(), len);
		}
	}

	void LoadBufferAsJPG(TByteArray& scratchBuffer, TByteArray& targetArray, uint32 span, cstr jpgPath)
	{
		size_t len = OS::LoadAsciiTextFile((char*)scratchBuffer.data(), scratchBuffer.size(), jpgPath);

		ImageLoader onLoad;
		onLoad.expectedSpan = span;

		Imaging::DecompressJPeg(onLoad, scratchBuffer.data(), len);

		if (onLoad.err.length() > 0)
		{
			Throw(0, "%s: Error loading %s [%s]", __FUNCTION__, jpgPath, onLoad.err.c_str());
		}

		if (span <= RAW_MAX_SPAN)
		{
			size_t nBytes = onLoad.decompressedImage.size() * sizeof(RGBAb);
			targetArray.resize(nBytes);
			memcpy(targetArray.data(), onLoad.decompressedImage.data(), nBytes);
		}
		else
		{
			targetArray.resize(len);
			memcpy(targetArray.data(), scratchBuffer.data(), len);
		}
	}

	enum class SxyMipMapsSpec: uint32
	{
		SPEC_RGBAb = 0
	};

#pragma pack(push,1)
	struct SXY_MIPMAPS_HEADER
	{
		uint32 sexyMipMapCode;
		SxyMipMapsSpec specification;
		uint16 version; // 0x<major>.<minor>
		uint16 compressedBits; // bit N = span of 2^N. Each set bit indicates that the image of given span is compressed, otherwise raw RGBAb data is used.
		uint16 offsets0To3[4];	// The offset starting from the beginning of the header to the data for mipmap with spans 1,2,4,8. 0 => missing
		uint32 offsets4To15[12]; // The offset starting from the beginning of the header to the data for mipmap with span 2^offset number. 0 => missing
	};

	struct SXY_MIPMAPS_COMPRESSED_HEADER
	{
		uint32 length;
		CompressionType compressionType;
	};

	void SaveMipMapsIntoBuffer(TByteArray& destination, const uint16 compressedBits, const std::vector<TByteArray>& mipMapLevels, const std::vector<CompressionType>& mipMapLevelCompression)
	{
		destination.clear();

		SXY_MIPMAPS_HEADER header;
		header.sexyMipMapCode = FourCC<'S', 'X', 'M', 'M'>().value;
		header.version = 0x0100;
		header.specification = SxyMipMapsSpec::SPEC_RGBAb; // RGBAb
		header.compressedBits = compressedBits;

		size_t totalOffset = sizeof header;

		for (int i = 0; i < 4; i++)
		{
			size_t len = mipMapLevels[i].size();
			if (!len)
			{
				header.offsets0To3[i] = 0;
			}
			else
			{
				header.offsets0To3[i] = (uint16) totalOffset;
				totalOffset += len;
			}
		}

		for (int j = 0; j < 12; j++)
		{
			size_t len = j + 4 < mipMapLevels.size() ? mipMapLevels[j + 4].size() : 0;
			if (!len)
			{
				header.offsets4To15[j] = 0;
			}
			else
			{
				header.offsets4To15[j] = (uint32) totalOffset;
				totalOffset += len + sizeof SXY_MIPMAPS_COMPRESSED_HEADER;
			}
		}

		destination.resize(totalOffset);

		memcpy(destination.data(), &header, sizeof header);

		for (int i = 0; i < 4; i++)
		{
			size_t len = mipMapLevels[i].size();
			if (len)
			{
				memcpy(destination.data() + header.offsets0To3[i], mipMapLevels[i].data(), len);
				_CrtCheckMemory();
			}
		}

		for (int j = 0; j < 12; j++)
		{
			size_t len = j  + 4< mipMapLevels.size() ? mipMapLevels[j + 4].size() : 0;
			if (len)
			{
				SXY_MIPMAPS_COMPRESSED_HEADER compressedHeader;
				compressedHeader.compressionType = mipMapLevelCompression[j + 4];
				compressedHeader.length = (uint32) len;
				memcpy(destination.data() + header.offsets4To15[j], &compressedHeader, sizeof compressedHeader);
				memcpy(destination.data() + header.offsets4To15[j] + sizeof SXY_MIPMAPS_COMPRESSED_HEADER, mipMapLevels[j + 4].data(), len);
				_CrtCheckMemory();
			}
		}
	};

#pragma pack(pop)

	ROCOCO_MISC_UTILS_API void BakeMipMappedTextureDirectory(cstr mipMapDir, uint32 maxSpan, cstr targetFilename)
	{
		if (mipMapDir == nullptr || *mipMapDir == 0)
		{
			Throw(0, "%s: The first argument, [mipMapDir], was blank.", __FUNCTION__);
		}

		if (!Strings::EndsWith(mipMapDir, ".mipmaps"))
		{
			Throw(0, "%s: The first argument, [mipMapDir='%s'], did not end with extension .mipmaps. Also, there should be no trailing slash", __FUNCTION__, mipMapDir);
		}

		if (!IO::IsDirectory(mipMapDir))
		{
			Throw(0, "%s: The first argument, [mipMapDir='%s'], does not appear to be a directory.", __FUNCTION__, mipMapDir);
		}

		U8FilePath targetPath;
		if (targetFilename)
		{
			Assign(targetPath, targetFilename);
		}
		else
		{
			Assign(targetPath, mipMapDir);
			if (!IO::TrySwapExtension(targetPath, ".mipmaps", ".23x")) // .23x was chosen because the year of writing is 2023, and I could not find a clash with the name on the Internet
			{
				Throw(0, "%s: Could not swap extension to %s", __FUNCTION__, mipMapDir);
			}
		}

		maxSpan = min(maxSpan, 8192U);

		std::vector<TByteArray> mipMapLevels;
		std::vector<CompressionType> mipMapLevelCompression;

		TByteArray scratchBuffer;
		scratchBuffer.resize(400_megabytes);

		uint16 compressedBits = 0;

		for (uint32 span = 1; span < maxSpan; span *= 2)
		{
			TByteArray entry;
			mipMapLevels.emplace_back(entry);

			mipMapLevelCompression.push_back(CompressionType::RAW);

			U8FilePath tifPath;
			Format(tifPath, "%s\\%ux%u.tif", mipMapDir, span, span);

			if (OS::IsFileExistant(tifPath))
			{
				LoadBufferAsTiff(scratchBuffer, mipMapLevels.back(), span, tifPath);

				if (span > RAW_MAX_SPAN)
				{
					mipMapLevelCompression.back() = CompressionType::TIFF;
					compressedBits |= span;
				}
			}
			else
			{
				U8FilePath jpgPath;
				Format(jpgPath, "%s\\%ux%u.jpg", mipMapDir, span, span);

				if (OS::IsFileExistant(jpgPath))
				{
					LoadBufferAsJPG(scratchBuffer, mipMapLevels.back(), span, jpgPath);

					if (span > RAW_MAX_SPAN)
					{
						mipMapLevelCompression.back() = CompressionType::JPG;
						compressedBits |= span;
					}
				}
			}
		}

		SaveMipMapsIntoBuffer(scratchBuffer, compressedBits, mipMapLevels, mipMapLevelCompression);

		OS::SaveBinaryFile(targetPath, scratchBuffer.data(), scratchBuffer.size());
	}

	ROCOCO_MISC_UTILS_API MapMapDesc FindMipMapInBakedFile(const uint8* headerAndContent, size_t length, uint32 mipMapLevel)
	{
		if (length < sizeof SXY_MIPMAPS_HEADER)
		{
			Throw(0, "%s: Insufficient data in buffer.", __FUNCTION__);
		}

		SXY_MIPMAPS_HEADER& header = *(SXY_MIPMAPS_HEADER*) headerAndContent;
		
		if (header.sexyMipMapCode != FourCC<'S', 'X', 'M', 'M'>().value)
		{
			Throw(0, "%s: unknown data block. Expecting FourCC(SXMM)", __FUNCTION__);
		}

		if (header.specification != SxyMipMapsSpec::SPEC_RGBAb)
		{
			Throw(0, "%s: unknown specification.", __FUNCTION__);
		}

		MapMapDesc desc = { 0 };

		desc.span = 1 << mipMapLevel;

		if (mipMapLevel <= 3)
		{
			// Always raw data, as raw data is smaller than compressed data below this threshold
			uint16 offset = header.offsets0To3[mipMapLevel];
			if (offset == 0)
			{
				return desc;
			}

			size_t longOffset = (size_t)(offset);
			desc.image = (const RGBAb*)(headerAndContent + longOffset);
			desc.compressedData = nullptr;
			desc.nPixels = Sq(desc.span);
			desc.lengthInBytes = (sizeof RGBA) * desc.nPixels;
			desc.compressionType = CompressionType::RAW;

			if (longOffset < sizeof header || longOffset + desc.lengthInBytes >= length)
			{
				Throw(0, "%s: bad or corrupt bake file", __FUNCTION__);
			}

			return desc;
		}
		else if (mipMapLevel < 16)
		{
			// Always raw data, as raw data is smaller than compressed data below this threshold
			uint32 offset = header.offsets4To15[mipMapLevel - 4];
			size_t longOffset = (size_t)(offset);
			if (offset == 0)
			{
				return desc;
			}

			uint32 compressionBit = 1 << mipMapLevel;

			if (HasFlag(compressionBit, header.compressedBits))
			{
				if (longOffset < sizeof header || longOffset + sizeof SXY_MIPMAPS_COMPRESSED_HEADER >= length)
				{
					Throw(0, "%s: bad or corrupt bake file", __FUNCTION__);
				}

				const SXY_MIPMAPS_COMPRESSED_HEADER& cHeader = *(const SXY_MIPMAPS_COMPRESSED_HEADER*) (headerAndContent + longOffset);
				desc.compressionType = cHeader.compressionType;

				switch (cHeader.compressionType)
				{
				case CompressionType::RAW:
					Throw(0, "%s: bad or corrupt compression section in bake file", __FUNCTION__);
					break;
				}

				desc.lengthInBytes = cHeader.length;
				desc.compressedData = headerAndContent + longOffset + sizeof(SXY_MIPMAPS_COMPRESSED_HEADER);
				desc.nPixels = Sq(desc.span);

				if (longOffset + sizeof(SXY_MIPMAPS_COMPRESSED_HEADER) + cHeader.length > length)
				{
					Throw(0, "%s: bad or corrupt compression section in bake file", __FUNCTION__);
				}

				return desc;
			}
			else
			{
				desc.image = (const RGBAb*)(headerAndContent + longOffset);
				desc.compressedData = nullptr;
				desc.nPixels = Sq(desc.span);
				desc.lengthInBytes = (sizeof RGBA) * desc.nPixels;
				desc.compressionType = CompressionType::RAW;

				if (longOffset < sizeof header || longOffset + desc.lengthInBytes >= length)
				{
					Throw(0, "%s: bad or corrupt bake file", __FUNCTION__);
				}

				return desc;
			}
		}
		else
		{
			Throw(0, "%s: bad argument. Maximum mipMapLevel index is 15", __FUNCTION__);
		}
	}

	void ExportBakeFile(const uint8* data, size_t lengthInBytes, uint32 span, cstr targetFile, cstr extension)
	{
		U8FilePath u8FinalTarget;
		Assign(u8FinalTarget, targetFile);

		if (!IO::TrySwapExtension(u8FinalTarget, ".23x", ".mipmaps"))
		{
			Throw(0, "Could not swap extension for %s", targetFile);
		}

		IO::CreateDirectoryFolder(u8FinalTarget);

		U8FilePath item;
		Format(item, "%s\\%ux%u%s", u8FinalTarget.buf, span, span, extension);

		OS::SaveBinaryFile(item, data, lengthInBytes);
	}

	void CompressAndExportBakeFile(const RGBAb* data, uint32 span, cstr targetPath)
	{
		U8FilePath u8FinalTarget;
		Assign(u8FinalTarget, targetPath);

		if (!IO::TrySwapExtension(u8FinalTarget, ".23x", ".mipmaps"))
		{
			Throw(0, "Could not swap extension for %s", targetPath);
		}

		IO::CreateDirectoryFolder(u8FinalTarget);

		U8FilePath item;
		Format(item, "%s\\%ux%u.tif", u8FinalTarget.buf, span, span);

		Imaging::CompressTiff((const RGBAb*) data, Vec2i { (int32) span, (int32) span }, item);
	}

	ROCOCO_MISC_UTILS_API void ExtractAsImageListFromBakedFile(cstr bakeFile23x)
	{
		if (bakeFile23x == nullptr || *bakeFile23x == 0) Throw(0, "%s: blank bakefile", __FUNCTION__);

		cstr ext = GetFileExtension(bakeFile23x);
		if (!Eq(ext, ".23x"))
		{
			Throw(0, "Expecting bakefile to have extension .23x");
		}
	
		auto onLoad = [bakeFile23x](uint8* data, size_t length)
		{
			for (uint32 i = 0; i < 16; i++)
			{
				MapMapDesc desc = FindMipMapInBakedFile(data, length, i);
				switch (desc.compressionType)
				{
				case CompressionType::JPG:
					ExportBakeFile(desc.compressedData, desc.lengthInBytes, desc.span, bakeFile23x, ".jpg");
					break;
				case CompressionType::TIFF:
					ExportBakeFile(desc.compressedData, desc.lengthInBytes, desc.span, bakeFile23x, ".tif");
					break;
				case CompressionType::RAW:
					if (desc.image) CompressAndExportBakeFile(desc.image, desc.span, bakeFile23x);
				}
			}
		};

		OS::LoadBinaryFile(onLoad, bakeFile23x);
	}
}