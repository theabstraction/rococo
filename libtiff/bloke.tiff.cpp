#ifndef _WIN32
# include <stddef.h>
#else
# include <malloc.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <rococo.api.h>
#include <rococo.imaging.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <tiffiop.h>
#include <vector>

namespace
{
   using namespace Rococo;

   Rococo::IAllocator* tiffAllocator = nullptr;

   void defaultErrorHandler(const char* module, const char* format, va_list args)
   {
      char msg[1024];
      SafeVFormat(msg, sizeof(msg), format, args);

      char totalmsg[1024];
      SafeFormat(totalmsg, sizeof(totalmsg), "%s:\n\t%s", module, msg);

      if (OS::IsDebugging())
      {
         OS::PrintDebug("%s\n", totalmsg);
         OS::TripDebugger();
      }
   }

   void defaultWarningHandler(const char* module, const char* format, va_list args)
   {
      char msg[1024];
      SafeVFormat(msg, sizeof(msg), format, args);

      char totalmsg[1024];
      SafeFormat(totalmsg, sizeof(totalmsg), "%s:\n\t%s", module, msg);

      if (OS::IsDebugging())
      {
         OS::PrintDebug("%s\n", totalmsg);
      }
   }
}

TIFFErrorHandler _TIFFerrorHandler = defaultErrorHandler;
TIFFErrorHandler _TIFFwarningHandler = defaultWarningHandler;

void _TIFFmemset(tdata_t target, int value, tsize_t capacity)
{
   memset(target, value, capacity);
}

extern void _TIFFmemcpy(tdata_t dest, const tdata_t src, tsize_t capacity)
{
#ifdef _WIN32
   memcpy_s(dest, capacity, src, capacity);
#else
   memcpy(dest, src, capacity);
#endif
}

int _TIFFmemcmp(const tdata_t a, const tdata_t b, tsize_t len)
{
   return memcmp(a, b, len);
}

void* _TIFFmalloc(tsize_t s)
{
   return tiffAllocator ? tiffAllocator->Allocate((size_t)s) : malloc((size_t) s);
}

void _TIFFfree(void* p)
{
   if (tiffAllocator)
   {
      tiffAllocator->FreeData(p);
   }
   else
   {
      free(p);
   }
}

void* _TIFFrealloc(void* p, tsize_t s)
{
   return tiffAllocator ? tiffAllocator->Reallocate(p, (size_t) s) : realloc(p, (size_t) s);
}


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
			SafeVFormat(errorBuffer, errorCapacity, format, args);
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

			TIFF* tif = TIFFClientOpen(filename, "r", (thandle_t)this, Read, Write, Seek, Close, GetFileLength, MapFile, UnmapFile);
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
					RGBAb* raster = (RGBAb*)_TIFFmalloc(nPixels * sizeof(RGBAb));

					if (TIFFReadRGBAImageOriented(tif, width, height, (Rococo::uint32*) raster, ORIENTATION_TOPLEFT))
					{
						uint32* p = (uint32*) raster;
						uint32* rasterEnd = p + (width * height);

						for (; p < rasterEnd; ++p)
						{
							RGBAb colour(TIFFGetR(*p), TIFFGetG(*p), TIFFGetB(*p), TIFFGetA(*p));
							RGBAb *dest = (RGBAb*)p;
							*dest = colour;
						}
						loadEvents.OnRGBAImage(Vec2i{ (Rococo::int32) width, (Rococo::int32) height }, raster);
						isGood = true;
					}
					else
					{
						SafeFormat(errorBuffer, errorCapacity, "Failed to parse TIFF F_A8R8G8B8-32bit memory image");
					}
					_TIFFfree(raster);
				}
				else if (samplesPerPixel == 1 && bitsPerSample == 8)
				{
					Rococo::uint32 len = (Rococo::uint32)TIFFScanlineSize(tif);
					nPixels = width * height;
					char* raster = (char*)_TIFFmalloc(nPixels * sizeof(char));

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
							SafeFormat(errorBuffer, errorCapacity, "Error reading scanline for 8-bit TIFF file");
							break;
						}
					}

					if (j == height)
					{
						loadEvents.OnAlphaImage(Vec2i{ (Rococo::int32) width,(Rococo::int32) height }, (const Rococo::uint8*) raster);
						isGood = true;
					}

					_TIFFfree(raster);
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

	class ImageWriter
	{
	private:
		const char* filename;

		FILE* f = nullptr;

		static tsize_t Read(thandle_t hThis, tdata_t buffer, tsize_t len)
		{
			ImageWriter* This = (ImageWriter*)hThis;
			size_t bytesRead = fread(buffer, 1, len, This->f);
			return bytesRead;
		}

		static tsize_t Write(thandle_t hThis, tdata_t buffer, tsize_t len)
		{
			ImageWriter* This = (ImageWriter*)hThis;
			size_t bytesWritten = fwrite(buffer, 1, len, This->f);
			return bytesWritten;
		}

		static toff_t GetFileLength(thandle_t hThis)
		{
			ImageWriter* This = (ImageWriter*)hThis;
			return ftell(This->f);
		}

		static toff_t Seek(thandle_t hThis, toff_t offset, int whence)
		{
			ImageWriter* This = (ImageWriter*)hThis;
			int result = fseek(This->f, offset, whence);
			long pos = ftell(This->f);
			return pos;
		}

		static int MapFile(thandle_t hThis, tdata_t* pBuffer, toff_t* pOffset)
		{
			return 0;
		}

		static void UnmapFile(thandle_t hThis, tdata_t buffer, toff_t offset)
		{
		}

		static int Close(thandle_t hThis)
		{
			return 0;
		}

		static void _OnError(thandle_t hThis, const char* module, const char* format, va_list args)
		{
			_OnError(module, format, args);
		}

		static void _OnError(const char* module, const char* format, va_list args)
		{
			SafeVFormat(errorBuffer, errorCapacity, format, args);
		}

		static char* errorBuffer;
		static size_t errorCapacity;

	public:
		ImageWriter(const char* _filename) : filename(_filename)
		{
			auto err = fopen_s(&f, _filename, "wb");
			if (err)
			{
				Throw(0, "Could not open %s: %s", filename, strerror(err));
			}
		}

		~ImageWriter()
		{
			if (f) fclose(f);
		}

		bool Write(const uint8* grayscalePixels, int32 width, int32 height, char* errorBuffer, size_t errorCapacity)
		{
			ImageWriter::errorBuffer = errorBuffer;
			ImageWriter::errorCapacity = errorCapacity;

			TIFFErrorHandler standardErrorHandler = TIFFSetErrorHandler(ImageWriter::_OnError);
			TIFFErrorHandlerExt standardErrorHandlerExt = TIFFSetErrorHandlerExt(ImageWriter::_OnError);

			bool isGood = false;

			TIFF* tif = TIFFClientOpen(filename, "wm", (thandle_t)this, Read, Write, Seek, Close, GetFileLength, MapFile, UnmapFile);
			if (tif)
			{
				TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (size_t) width);
				TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (size_t) height);
				TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
				TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
				TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);  
				TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
				TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
				TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
				TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);

				uint32 rowsPerStrip = height;
				rowsPerStrip = TIFFDefaultStripSize(tif, rowsPerStrip);
				TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rowsPerStrip);
				TIFFSetupStrips(tif);

				tsize_t linebytes = sizeof(char) * width; 

				//    Allocating memory to store the pixels of current row
				tsize_t scanLineLength = TIFFScanlineSize(tif);

				tsize_t bufLen = max(linebytes, scanLineLength);

				std::vector<uint8> buf;
				buf.resize(bufLen);

				TIFFWriteEncodedStrip(tif, 0, (tdata_t) grayscalePixels, linebytes * height);

				TIFFFlush(tif);
				TIFFClose(tif);
			}

			ImageWriter::errorBuffer = NULL;
			ImageWriter::errorCapacity = 0;

			TIFFSetErrorHandler(standardErrorHandler);
			TIFFSetErrorHandlerExt(standardErrorHandlerExt);

			return isGood;
		}
	};

	char* ImageWriter::errorBuffer = NULL;
	size_t ImageWriter::errorCapacity = 0;
}

namespace Rococo
{
	namespace Imaging
	{
		bool DecompressTiff(IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes)
		{
			ImageReader reader("Tiff-File", (const char*)sourceBuffer, (toff_t)dataLengthBytes);

			char errMessage[1024];
			errMessage[0] = 0;

			bool isOk = reader.TryRead(errMessage, sizeof(errMessage), loadEvents);

			if (!isOk)
			{
				loadEvents.OnError(errMessage);
			}

			return isOk;
		}

		void SetTiffAllocator(IAllocator* _allocator)
		{
			tiffAllocator = _allocator;
		}

		void SaveAsTiff(const uint8* grayScale, const Vec2i& span, const char* filename)
		{
			ImageWriter writer(filename);

			char errorBuffer[1024];
			if (!writer.Write(grayScale, span.x, span.y, errorBuffer, sizeof(errorBuffer)))
			{
				Throw(0, "Error writing %s: %s", filename, errorBuffer);
			}
		}
	}
}