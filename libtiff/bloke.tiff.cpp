#include <rococo.types.h>
#include <rococo.imaging.h>

#include "libtiff\tiffiop.h"

#include <stdio.h>
#include <stdlib.h>

#include <malloc.h>

namespace
{
	using namespace Rococo;
	using namespace Rococo::Imaging;

	class ImageReader
	{
	private:
		const char* filename;
		const char* data;
		toff_t length;
		toff_t readPos;

		static tsize_t Read(thandle_t hThis, tdata_t buffer, tsize_t len)
		{
			ImageReader* This = (ImageReader*)hThis;
			return This->Read((char*)buffer, len);
		}

		static tsize_t Write(thandle_t hThis, tdata_t buffer, tsize_t len)
		{
			return 0;
		}

		static toff_t GetFileLength(thandle_t hThis)
		{
			ImageReader* This = (ImageReader*)hThis;
			return This->length;
		}

		static toff_t Seek(thandle_t hThis, toff_t offset, int whence)
		{
			ImageReader* This = (ImageReader*)hThis;
			return This->Seek(offset, whence);
		}

		static int MapFile(thandle_t hThis, tdata_t* pBuffer, toff_t* pOffset)
		{
			ImageReader* This = (ImageReader*)hThis;
			*pBuffer = (tdata_t)This->data;
			*pOffset = (toff_t)This->length;
			return 1;
		}

		static void UnmapFile(thandle_t hThis, tdata_t buffer, toff_t offset)
		{
		}

		static int Close(thandle_t)
		{
			return 0;
		}

		static void _OnError(thandle_t hThis, const char* module, const char* format, va_list args)
		{
			_OnError(module, format, args);
		}

		static void _OnError(const char* module, const char* format, va_list args)
		{
			_vsnprintf_s(errorBuffer, errorCapacity, errorCapacity, format, args);
		}

		static char* errorBuffer;
		static size_t errorCapacity;

		tsize_t Read(char* buffer, tsize_t len)
		{
			if (readPos >= length) return 0;
			tsize_t nBytesToRead = min((tsize_t)(length - readPos), len);
			memcpy(buffer, data + readPos, nBytesToRead);
			readPos += nBytesToRead;
			return nBytesToRead;
		}

		toff_t Seek(toff_t offset, int whence)
		{
			switch (whence)
			{
			case SEEK_CUR:
				readPos += offset;
				break;
			case SEEK_END:
				if (offset > length)
				{
					readPos = 0;
				}
				else
				{
					readPos = length - offset;
				}
				break;
			case SEEK_SET:
			default:
				readPos = offset;
				break;
			}

			return readPos;
		}

	public:
		ImageReader(const char* _filename, const char* _data, toff_t _length) : filename(_filename), data(_data), length(_length), readPos(0)
		{
			typedef void(*TIFFErrorHandlerExt)(thandle_t, const char*, const char*, va_list);
		}

		bool TryRead(char* errorBuffer, size_t errorCapacity, IImageLoadEvents& loadEvents)
		{
			ImageReader::errorBuffer = errorBuffer;
			ImageReader::errorCapacity = errorCapacity;

			TIFFErrorHandler standardErrorHandler = TIFFSetErrorHandler(ImageReader::_OnError);
			TIFFErrorHandlerExt standardErrorHandlerExt = TIFFSetErrorHandlerExt(ImageReader::_OnError);

			bool isGood = false;

			TIFF* tif = TIFFClientOpen(filename, "r", this, Read, Write, Seek, Close, GetFileLength, MapFile, UnmapFile);
			if (tif)
			{
				Rococo::uint32 width, height;
				size_t nPixels;

				TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
				TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

				Rococo::uint16 samplesPerPixel, bitsPerSample, photoMetric;
				TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
				TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
				TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photoMetric);

				if (samplesPerPixel == 3 || samplesPerPixel == 4)
				{
					nPixels = width * height;
					F_A8R8G8B8* raster = (F_A8R8G8B8*)_malloca(nPixels * sizeof (F_A8R8G8B8));

					if (TIFFReadRGBAImageOriented(tif, width, height, (Rococo::uint32*) raster, ORIENTATION_TOPLEFT))
					{
						F_A8R8G8B8* engineFormatBuffer = (F_A8R8G8B8*)_malloca(nPixels * sizeof (F_A8R8G8B8));
						F_A8R8G8B8* source = (F_A8R8G8B8*)raster;
						F_A8R8G8B8* dest = engineFormatBuffer;

						for (size_t i = 0; i < nPixels; ++i)
						{
							*dest++ = *source++;
						}

						loadEvents.OnARGBImage(Vec2i(width, height), engineFormatBuffer);
						isGood = true;

						_freea(engineFormatBuffer);
					}
					else
					{
						_snprintf_s(errorBuffer, errorCapacity, errorCapacity, "Failed to parse TIFF F_A8R8G8B8-32bit memory image");
					}
					_freea(raster);
				}
				else if (samplesPerPixel == 1 && bitsPerSample == 8)
				{
					Rococo::uint32 len = (Rococo::uint32)TIFFScanlineSize(tif);
					nPixels = width * height;
					char* raster = (char*)_malloca(nPixels * sizeof (char));

					Rococo::uint32 j;
					for (j = 0; j < height; j++)
					{
						char* pTarget = &raster[width * j];
						if (TIFFReadScanline(tif, pTarget, j))
						{
							pTarget += len;
						}
						else
						{
							_snprintf_s(errorBuffer, errorCapacity, errorCapacity, "Error reading scanline for 8-bit TIFF file");
							break;
						}
					}

					if (j == height)
					{
						loadEvents.OnAlphaImage(Vec2i(width, height), (const Rococo::uint8*) raster);
						isGood = true;
					}

					_freea(raster);
				}

				TIFFClose(tif);
			}

			ImageReader::errorBuffer = NULL;
			ImageReader::errorCapacity = 0;

			TIFFSetErrorHandler(standardErrorHandler);
			TIFFSetErrorHandlerExt(standardErrorHandlerExt);

			return isGood;
		}
	};

	char* ImageReader::errorBuffer = NULL;
	size_t ImageReader::errorCapacity = 0;
}

namespace Rococo
{
	namespace Imaging
	{
		bool DecompressTiff(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes)
		{
			ImageReader reader("Tiff-File", (const char*) sourceBuffer, (toff_t) dataLengthBytes);

			char errMessage[1024];
			errMessage[0] = 0;

			bool isOk = reader.TryRead(errMessage, sizeof(errMessage), loadEvents);

			if (!isOk)
			{
				loadEvents.OnError(errMessage);
			}

			return isOk;
		}
	}
}