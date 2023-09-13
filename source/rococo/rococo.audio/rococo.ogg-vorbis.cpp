#include <rococo.audio.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.release.h>
#include <rococo.strings.h>
#include <rococo.time.h>

#include <vorbis/vorbisfile.h>

#include <vector>

#pragma comment(lib, "lib-vorbis-file.lib")

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Audio;
using namespace Rococo::Strings;

namespace AudioAnon
{
	class OggVorbisFile
	{
		size_t encodedLength;
		const uint8* encodedData;
		mutable OggVorbis_File apiHandle;
		int64 readIndex = 0;

		size_t Read(void* data, size_t sizeofEachElement, size_t numberOfElements)
		{
			if (readIndex < 0)
			{
				return 0;
			}
			else if	(readIndex >= (int64) encodedLength)
			{
				return 0;
			}

			size_t nBytesToRead = sizeofEachElement * numberOfElements;
			if (nBytesToRead == 0)
			{
				return 0;
			}

			size_t nElementsLeft = numberOfElements;
			if (nBytesToRead + readIndex >= encodedLength)
			{
				size_t nBytesLeft = encodedLength - readIndex;
				nElementsLeft = nBytesLeft / sizeofEachElement;
				nBytesToRead = nElementsLeft * sizeofEachElement;
			}

			if (data)
			{
				memcpy(data, encodedData + readIndex, nBytesToRead);
				readIndex += nBytesToRead;
			}

			return nElementsLeft;
		}

		NOT_INLINE	int Seek(ogg_int64_t offset, int whence)
		{
			switch (whence)
			{
			case SEEK_SET:
				readIndex = offset;
				break;
			case SEEK_CUR:
				readIndex += offset;
				break;
			case SEEK_END:
				readIndex = encodedLength + offset;
				break;
			}

			return 0;
		}

		int Close()
		{
			readIndex = 0;
			return 0;
		}

		NOT_INLINE long Tell()
		{
			return (long) readIndex;
		}

		static size_t StaticRead(void* data, size_t size, size_t nmemb, void* This)
		{
			return ((OggVorbisFile*)This)->Read(data, size, nmemb);
		}

		static int StaticSeek(void* This, ogg_int64_t offset, int whence)
		{
			return ((OggVorbisFile*)This)->Seek(offset, whence);
		}

		static int StaticClose(void* This)
		{
			return ((OggVorbisFile*)This)->Close();
		}

		static long StaticTell(void* This)
		{
			return ((OggVorbisFile*)This)->Tell();
		}
	public:
		OggVorbisFile(cstr utf8Path, const uint8* _encodedData, size_t _encodedLength):
			encodedData(_encodedData),
			encodedLength(_encodedLength)
		{
			ov_callbacks callbacks;
			callbacks.read_func = OggVorbisFile::StaticRead;
			callbacks.seek_func = OggVorbisFile::StaticSeek;
			callbacks.close_func = OggVorbisFile::StaticClose;
			callbacks.tell_func = OggVorbisFile::StaticTell;

			int err = ov_open_callbacks(this, &apiHandle, (cstr) _encodedData, (long) encodedLength, callbacks);
			if (err != 0)
			{
				cstr msg;
				switch (err)
				{
				case OV_EREAD: msg = "A read from media returned an error."; break;
				case OV_ENOTVORBIS: msg = "Bitstream does not contain any Vorbis data."; break;
				case OV_EVERSION: msg = "Vorbis version mismatch."; break;
				case OV_EBADHEADER: msg = "Invalid Vorbis bitstream header."; break;
				case OV_EFAULT: msg = "Internal logic fault; indicates a bug or heap / stack corruption."; break;
				default: Throw(0, "Unknown Ogg Vorbis Error ov_open_callbacks(%s) returned [%d]", utf8Path, err);
				}

				Throw(0, "Ogg Vorbis Error [%s] ov_open_callbacks(%s)", msg, utf8Path);
			}
		}

		~OggVorbisFile()
		{
			ov_clear(&apiHandle);
		}

		int StreamCount() const
		{
			return (int) ov_streams(&apiHandle);
		}

		struct StreamInfo
		{
			int channels;
			int sampleRate;
		};

		StreamInfo GetStreamInfo(int streamIndex) const
		{
			vorbis_info* pInfo = ov_info(&apiHandle, streamIndex);
			if (pInfo)
			{
				return { pInfo->channels, pInfo->rate };
			}
			else
			{
				return { 0, 0 };
			}
		}

		int Decode(uint8* decompressedBuffer, int decompressedCapacity)
		{
			enum { USING_SIGNED_SAMPLES = 1 };
			int bitStream = 0;
			long result = ov_read(&apiHandle, (char*) decompressedBuffer, decompressedCapacity, !OS::IsEndianLittle(), sizeof int16, USING_SIGNED_SAMPLES, &bitStream);
			switch (result)
			{
			case OV_HOLE:
				// Corrupt data
				Throw(0, "Hole: Corrupt file data");
			case OV_EBADLINK:
				Throw(0, "Bad link: Corrupt file data");
			case OV_EINVAL:
				Throw(0, "Invalid read: Corrupt file data");
			case 0: /* EOF */
				break;
			default:
				break;
			}

			int bytesRead = result;
			return bytesRead;
		}
	};

	void CopyData(StereoSample_INT16* __restrict output, const uint8* __restrict input, uint32 nSamples)
	{
		memcpy(output, input, nSamples * sizeof StereoSample_INT16);
	}

	void PopulateMediaBufferWithFileData(IAudioInstallationSupervisor& installation, cstr utf8Path, std::vector<uint8>& encodedData)
	{
		struct OnLoad : IO::ILoadEventsCallback
		{
			std::vector<uint8>& encodedData;
	
			OnLoad(std::vector<uint8>& _encodedData): encodedData(_encodedData)
			{

			}

			~OnLoad()
			{
			}

			void OnFileOpen(int64 fileLength) override
			{
				if (fileLength > (int64) 128_megabytes)
				{
					Throw(0, "The Ogg-Vorbis decoder has a limit of 128 megabytes");
				}

				encodedData.resize(fileLength);
			}

			void OnDataAvailable(ILoadEventReader& reader) override
			{
				uint32 bytesRead = 0;
				reader.ReadData(encodedData.data(), (uint32) encodedData.size(), OUT bytesRead);
				
				if (bytesRead != (uint32)encodedData.size())
				{
					Throw(0, "reader.ReadData(...) did not read the expected buffer size");
				}
			}
		} cb(encodedData);

		try
		{
			installation.LoadResource(utf8Path, cb);
		}
		catch (IException& ex)
		{
			Throw(ex.ErrorCode(), "%s: error loading %s: %s", __FUNCTION__, utf8Path, ex.Message());
		}
	}

	struct OggVorbisLoader : ILoaderSupervisor
	{
		IAudioInstallationSupervisor& installation;
		std::vector<uint8> scratchBuffer;
		uint32 nChannels;

		std::vector<uint8> encodedDataSingleThreadedPrivateHeap;

		OggVorbisLoader(IAudioInstallationSupervisor& _installation, uint32 localChannels, uint32 reserveSeconds = 120) : installation(_installation), nChannels(localChannels)
		{
			scratchBuffer.reserve(44'100 * (size_t) reserveSeconds * sizeof(int16) * nChannels);
		}

		uint32 DecodeAudio(cstr utf8Path, IAudioSample& sample, IAudioSampleEvents& events, IPCMAudioBufferManager& audioBufferManager) override
		{
			try
			{
				size_t length = DecodeAudioProtected(utf8Path, audioBufferManager);
				if (length == 0)
				{
					events.MarkBadSample(sample, "Sample 0 length");					
				}
				
				return (uint32) length;
			}
			catch (IException& ex)
			{
				events.MarkBadSample(sample, ex.Message());
				return 0;
			}
		}

		uint32 DecodeAudioProtected(cstr utf8Path, IPCMAudioBufferManager& audioBufferManager) 
		{
			// Our algorithm expands the mp3 into a scratch buffer. This grows to accomodate the largest decoded MP3 put through the system
			// The sample we extract copies the relevant portion of the scratch buffer - this way we ensure the minimum of memory fragmentation.

			try
			{
				PopulateMediaBufferWithFileData(installation, utf8Path, encodedDataSingleThreadedPrivateHeap);
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "%s: %s", __FUNCTION__, ex.Message());
			}

			size_t length = encodedDataSingleThreadedPrivateHeap.size();
			if (length == 0)
			{
				Throw(0, "%s: the file was of zero length", __FUNCTION__);
			}

			OggVorbisFile f(utf8Path, encodedDataSingleThreadedPrivateHeap.data(), encodedDataSingleThreadedPrivateHeap.size());

			int streamCount = f.StreamCount();
			if (streamCount != 1)
			{
				Throw(0, "Only OGG files with one stream can be decoded. The file had %d.", streamCount);
			}

			auto info = f.GetStreamInfo(0);

			if (info.sampleRate == 0)
			{
				Throw(0, "The OGG file had an unknown sample rate");
			}

			if (info.channels != (int) nChannels)
			{
				Throw(0, "The OGG file had %d channels. The decoder has %u. Bad karma at line infinity.", info.channels, nChannels);
			}

			size_t scaleFactor = 1;

			switch(info.sampleRate)
			{
			case 44100:
				scaleFactor = 1;
				break;
			case 22050:
				scaleFactor = 2;
				break;
			case 11025:
				scaleFactor = 4;
				break;
			default:
				Throw(0, "The OGG file sample rate was %d Hz. The sample rate needs to be 44100 Hz, 22050 Hz or 11025 Hz for playback.", info.sampleRate);
			}

			// Assume 11:1 compression ratio. Increase ratio to 12 to try to minimize resizing
			size_t approxDecompressedSize = (length * 12) & (0xFFFFFF00);

			scratchBuffer.resize(approxDecompressedSize * scaleFactor);
			
			if (!audioBufferManager.Accept(AudioBufferDescriptor{ nChannels, 16 }))
			{
				Throw(0, "%s: The audio buffer manager rejected the request to transcode", __FUNCTION__);
			}
			
			uint32 cursor = 0;
			int32 resizeCount = 0;

			enum { CHUNK_SIZE = 512 };
			char chunk[CHUNK_SIZE];

			size_t expandedChunkSize = scaleFactor * CHUNK_SIZE;

			while (true)
			{
				if (expandedChunkSize + cursor > scratchBuffer.size())
				{
					scratchBuffer.resize(cursor + expandedChunkSize);
					resizeCount++;
				}

				char* target = scaleFactor != 1 ? chunk : (char*)scratchBuffer.data();
				int bytesToRead = (int)CHUNK_SIZE;

				int nBytesRead = f.Decode((uint8*) target, bytesToRead);

				if (nBytesRead == 0)
				{
					// EOF
					break;
				}

				if ((nBytesRead & 1) != 0)
				{
					Throw(0, "Expecting int16s in decoded output, but the number of bytes returned from the ready operation was odd");
				}

				if (scaleFactor == 1)
				{
					cursor += nBytesRead;
					continue;
				}

				int16* end = (int16*) (chunk + nBytesRead);
				int16* reader = (int16*)chunk;

				int16* scratchWriter = (int16*)(scratchBuffer.data() + cursor);

				switch (scaleFactor)
				{
				case 2:
					while (reader < end)
					{
						*scratchWriter++ = *reader;
						*scratchWriter++ = *reader++;
					}
					break;
				case 4:
					while (reader < end)
					{
						*scratchWriter++ = *reader;
						*scratchWriter++ = *reader;
						*scratchWriter++ = *reader;
						*scratchWriter++ = *reader++;
					}
					break;
				default:
					Throw(0, "Bad scalefactor");
				}

				cursor += nBytesRead;
			}

		
			PCMAudioLoadingMetrics metrics{ resizeCount };
			audioBufferManager.Finalize(metrics);

			auto& target = audioBufferManager.PCMBuffer();
			target.Resize(cursor);

			memcpy(target.GetData(), scratchBuffer.data(), cursor);
			
			return cursor;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Audio
{
	ILoaderSupervisor* CreateSingleThreadedOggVorbisLoader(IAudioInstallationSupervisor& installation, uint32 nChannels)
	{
		return new AudioAnon::OggVorbisLoader(installation, nChannels);
	}
}