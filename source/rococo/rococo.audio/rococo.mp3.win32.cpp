#include <rococo.audio.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <initguid.h>
#include <windows.h>
#include <Wmcodecdsp.h>
#include <mftransform.h>
#include <mfobjects.h>
#include <mfapi.h>
#include <mferror.h>

#include <rococo.release.h>
#include <rococo.strings.h>
#include <rococo.time.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Audio;
using namespace Rococo::Strings;


#define VALIDATE(x)\
{ \
	if FAILED(x) \
		Throw(hr, "%s: %s", __FUNCTION__, #x); \
}

namespace AudioAnon
{
	void WriteSilence(Audio::StereoSample_INT16* samples, const uint32 sampleCount)
	{
		memset(samples, 0, sizeof(StereoSample_INT16) * sampleCount);
	}

	void CopyData(StereoSample_INT16* __restrict output, const LPBYTE __restrict input, uint32 nSamples)
	{
		memcpy(output, input, nSamples * sizeof StereoSample_INT16);
	}

	// https://docs.microsoft.com/en-us/windows/win32/medfound/uncompressed-audio-media-types
	void CreatePCMAudioType(
		UINT32 sampleRate,        // Samples per second
		UINT32 bitsPerSample,     // Bits per sample
		UINT32 cChannels,         // Number of channels
		IMFMediaType** ppType     // Receives a pointer to the media type.
	)
	{
		HRESULT hr = S_OK;
		AutoRelease<IMFMediaType> pType;

		// Calculate derived values.
		UINT32 blockAlign = cChannels * (bitsPerSample / 8);
		UINT32 bytesPerSecond = blockAlign * sampleRate;

		// Create the empty media type.
		VALIDATE(hr = MFCreateMediaType(&pType));

		// Set attributes on the type.
		hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)", __FUNCTION__);
		}

		hr = pType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, cChannels);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, cChannels)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, blockAlign);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, blockAlign)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, bytesPerSecond);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, bytesPerSecond)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE)", __FUNCTION__);
		}

		// Return the type to the caller.
		*ppType = pType;
		(*ppType)->AddRef();
	}

	void CreateMP3AudioType(
		UINT32 sampleRate, //44100 or 48000
		UINT32 cChannels,         // Number of channels
		IMFMediaType** ppType     // Receives a pointer to the media type.
	)
	{
		HRESULT hr = S_OK;
		AutoRelease<IMFMediaType> pType;

		// Create the empty media type.
		hr = MFCreateMediaType(&pType);
		if (FAILED(hr))
		{
			Throw(hr, "%s: MFCreateMediaType", __FUNCTION__);
		}

		// Set attributes on the type.
		hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)", __FUNCTION__);
		}

		hr = pType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_MP3);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetGUID(MF_MT_SUBTYPE, MFAudioFormat_MP3)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, cChannels);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, cChannels)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate)", __FUNCTION__);
		}

		/*
		hr = pType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, blockAlign);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, blockAlign)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, bytesPerSecond);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, bytesPerSecond)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample)", __FUNCTION__);
		}

		hr = pType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
		if (FAILED(hr))
		{
			Throw(hr, "%s: SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE)", __FUNCTION__);
		}

		*/

		// Return the type to the caller.
		* ppType = pType;
		(*ppType)->AddRef();
	}

	void PopulateMediaBufferWithFileData(IAudioInstallationSupervisor& installation, cstr utf8Path, IMFMediaBuffer& buffer)
	{
		struct OnLoad : IO::ILoadEventsCallback
		{
			IMFMediaBuffer& buffer;
			DWORD maxLength = 0;
			DWORD currentLength = 0;
			BYTE* data = NULL;
			int64 fileLength = -1;
			bool isLocked = false;

			OnLoad(IMFMediaBuffer& _buffer): buffer(_buffer)
			{

			}

			~OnLoad()
			{
				if (isLocked) buffer.Unlock();
			}

			void OnFileOpen(int64 fileLength) override
			{
				if (fileLength > (int64) 128_megabytes)
				{
					Throw(0, "The MP3 decoder has a limit of 128 megabytes");
				}

				this->fileLength = fileLength;

				HRESULT hr;
				VALIDATE(hr = buffer.SetCurrentLength((DWORD)fileLength));
				VALIDATE(hr = buffer.Lock(&data, &maxLength, &currentLength));
				isLocked = true;
			}

			void OnDataAvailable(ILoadEventReader& reader) override
			{
				uint32 bytesRead = 0;
				reader.ReadData(data, currentLength, OUT bytesRead);
				
				if (bytesRead != currentLength)
				{
					Throw(0, "reader.ReadData(...) did not read the expected buffer size");
				}
			}
		} cb(buffer);

		ErrorCode errorCode;
		if (!installation.TryLoadResource(utf8Path, cb, OUT errorCode))
		{
			ThrowMissingResourceFile(errorCode, "Could not load MP3 file", utf8Path);
		}
	}

	bool IsPCMWithSpecifiedChannelCount(IMFMediaType& type, uint32 matchSampleRate, int channelCount)
	{
		// Set attributes on the type.
		GUID majorType;
		HRESULT hr = type.GetGUID(MF_MT_MAJOR_TYPE, &majorType);
		if (FAILED(hr))
		{
			Throw(hr, "%s: GetGUID(MF_MT_MAJOR_TYPE)", __FUNCTION__);
		}

		if (MFMediaType_Audio != majorType)
		{
			return false;
		}

		GUID subType;
		hr = type.GetGUID(MF_MT_SUBTYPE, &subType);
		if (FAILED(hr))
		{
			Throw(hr, "%s: GetGUID(MF_MT_SUBTYPE)", __FUNCTION__);
		}

		if (MFAudioFormat_PCM != subType)
		{
			return false;
		}

		UINT32 nChannels = 0;
		hr = type.GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &nChannels);
		if (FAILED(hr))
		{
			Throw(hr, "%s: type.GetUINT32(MF_MT_AUDIO_NUM_CHANNELS)", __FUNCTION__);
		}

		if (nChannels != (UINT32) channelCount)
		{
			return false;
		}

		UINT bitsPerSample;
		hr = type.GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
		if (FAILED(hr))
		{
			Throw(hr, "%s: GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample)", __FUNCTION__);
		}

		if (bitsPerSample != 16)
		{
			return false;
		}

		UINT32 sampleRate;
		hr = type.GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
		if (hr == MF_E_ATTRIBUTENOTFOUND)
		{
			sampleRate = 0;
		}
		else if (FAILED(hr))
		{
			Throw(hr, "%s: type.GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate)", __FUNCTION__);
		}

		if (sampleRate != 0 && sampleRate != matchSampleRate)
		{
			return false;
		}

		return true;
	}

	IMFTransform* CreateMP32TransformerWith1InputAnd1OutputStream()
	{
		HRESULT hr;

		IMFTransform* transform = nullptr;

		VALIDATE(hr = CoCreateInstance(
			CLSID_CMP3DecMediaObject,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IMFTransform,
			(LPVOID*)&transform
		));

		DWORD inputStreamCount, outputStreamCount;
		VALIDATE(hr = transform->GetStreamCount(&inputStreamCount, &outputStreamCount));

		if (inputStreamCount != 1 || outputStreamCount != 1)
		{
			transform->Release();
			Throw(hr, "%s: IMFTransform->GetStreamCount() expected {1,1} result ", __FUNCTION__);
		}

		return transform;
	}

	struct AudioDecoder : IAudioDecoder, OS::IThreadJob
	{
		enum { MAX_MP3_SIZE = 128_megabytes };
		IAudioInstallationSupervisor& installation;
		AutoRelease<IMFMediaBuffer> mp3Buffer;
		AutoRelease<IMFSample> mp3Sample;
		AutoRelease<IMFSample> outputSample;
		AutoRelease<IMFMediaBuffer> pcmBuffer;
		AutoRelease<IMFTransform> transform;

		DWORD inputStreamCount = 0;
		DWORD outputStreamCount = 0;

		DWORD inputId = 0;
		DWORD outputId = 0;

		volatile bool isStreaming = false;

		double lastLoadCostMS = 0;

		AutoFree<OS::IThreadSupervisor> thread;

		U8FilePath nextMusicFile;
		volatile int64 currentIndex = 0;
		volatile int64 nextIndex = 0;

		volatile int64 blockLock = 0;

		AudioDecoder(IAudioInstallationSupervisor& _installation, uint32 outputSampleDelta): installation(_installation)
		{
			transform = CreateMP32TransformerWith1InputAnd1OutputStream();
			
			HRESULT hr = transform->GetStreamIDs(1, &inputId, 1, &outputId);
			if (hr == E_NOTIMPL)
			{
				inputId = 0;
				outputId = 0;
			}
			else if FAILED(hr)
			{
				Throw(hr, "%s: transform->GetStreamIDs(1, &inputId, 1, &outputId);", __FUNCTION__);
			}

			VALIDATE(hr = MFCreateMemoryBuffer(MAX_MP3_SIZE, &mp3Buffer));
			VALIDATE(hr = MFCreateSample(&mp3Sample));
			VALIDATE(hr = mp3Sample->AddBuffer(mp3Buffer));
			VALIDATE(hr = MFCreateMemoryBuffer(outputSampleDelta * sizeof(StereoSample_INT16), &pcmBuffer));
			VALIDATE(hr = MFCreateSample(&outputSample));
			VALIDATE(hr = outputSample->AddBuffer(pcmBuffer));

			AutoRelease<IMFMediaType> outputType;
			CreatePCMAudioType(44100, 16, 2, &outputType);

			AutoRelease<IMFMediaType> inputType;
			CreateMP3AudioType(44100, 2, &inputType);

			VALIDATE(hr = transform->SetInputType(inputId, inputType, 0));

			for (UINT typeIndex = 0; ; ++typeIndex)
			{
				AutoRelease<IMFMediaType> optType;
				hr = transform->GetOutputAvailableType(outputId, typeIndex, &optType);
				if FAILED(hr)
				{
					Throw(hr, "%s: (MP3 to PCM) transform->GetOutputAvailableType(outputId, typeIndex, optType) failed", __FUNCTION__);
				}

				if (IsPCMWithSpecifiedChannelCount(*optType, 44100, 2))
				{
					hr = transform->SetOutputType(outputId, optType, 0);
					if FAILED(hr)
					{
						Throw(hr, "%s: (MP3 to PCM) transform->SetOutputType(0, outputType, 0)", __FUNCTION__);
					}

					break;
				}
			}

			MFT_INPUT_STREAM_INFO inputInfo;
			VALIDATE(hr = transform->GetInputStreamInfo(inputId, &inputInfo));

			MFT_OUTPUT_STREAM_INFO outputInfo;
			VALIDATE(hr = transform->GetOutputStreamInfo(outputId, &outputInfo));

			thread = OS::CreateRococoThread(this, 0);
			thread->Resume();
		}

		void StreamInputFile(cstr path) override
		{
			Format(nextMusicFile, "%s", path);
			nextIndex++;
			
			OS::WakeUp(*thread);
		}

		void StreamInputFile_DecoderThread(cstr utf8Path)
		{
			try
			{
				Time::ticks start = Time::TickCount();
				StreamInputFileProtected(utf8Path);
				Time::ticks duration = Time::TickCount() - start;
				double dt = duration / (double)Time::TickHz();
				lastLoadCostMS =  1000.0 * dt;
			}
			catch (IException& ex)
			{
				if (ex.StackFrames())
				{
					// Assume we were given an exception with detailed stack info because a serious problem occured, in which case it may be deep API and missing the resource name
					Throw(ex.ErrorCode(), "AudioDecoder(MP3).StreamInputFile(%s) failed:\n %s", utf8Path, ex.Message());
				}
				else
				{
					// stack frames are missing, because we just a missing resource, and we assume that is covered by the exception message
					throw;
				}
			}
		}
		
		void StreamInputFileProtected(cstr utf8Path)
		{
			try
			{
				PopulateMediaBufferWithFileData(installation, utf8Path, *mp3Buffer);
			}
			catch (IException& ex)
			{
				if (ex.StackFrames())
				{
					// Assume we were given an exception with detailed stack info because a serious problem occured, in which case it may be deep API and missing the resource name
					Throw(ex.ErrorCode(), "%s:\n%s: %s", __FUNCTION__, utf8Path, ex.Message());
				}
				else
				{
					// stack frames are missing, because we just a missing resource, and we assume that is covered by the exception message
					isStreaming = true;
					return;
				}
			}

			HRESULT hr;
			VALIDATE(hr = transform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0));
			VALIDATE(hr = transform->ProcessInput(inputId, mp3Sample, 0));

			isStreaming = true;
		}

		virtual ~AudioDecoder()
		{
			thread = nullptr;
		}

		uint32 RunThread(OS::IThreadControl& tc) override
		{
			tc.SetRealTimePriority();

			while (tc.IsRunning())
			{
				U8FilePath currentPath;
				U8FilePath nextPath;

				if (nextIndex > currentIndex)
				{
					// Edge case: nextIndex might be updated during the copy of this->nextMusicFile
					// So after the copy, check the index is what it was at the start of the copy

					for (;;)
					{
						int64 n = nextIndex;			
						nextPath = this->nextMusicFile;
						currentIndex = nextIndex;
						if (n == nextIndex) break;
					}

					if (!Eq(currentPath, nextPath))
					{
						// We have a new file
						currentPath = nextPath;

						if (isStreaming)
						{
							isStreaming = false;

							while ((blockLock % 2) == 1)
							{
								// GetOutput is processing
							}
						}
						StreamInputFile_DecoderThread(currentPath);
					}
				}

				tc.SleepUntilAysncEvent(1000);
			}

			return 0;
		}

		uint32 Write_MP3ToPCM_AudioThread(StereoSample_INT16* output, uint32 nSamples, STREAM_STATE& state)
		{
			MFT_OUTPUT_DATA_BUFFER buffer;
			buffer.dwStreamID = outputId;
			buffer.pSample = outputSample;
			buffer.dwStatus = 0;
			buffer.pEvents = nullptr;

			DWORD status;
			HRESULT hr = transform->ProcessOutput(0, 1, &buffer, &status);
			if FAILED(hr)
			{
				WriteSilence(output, nSamples);
				isStreaming = false;
				state = STREAM_STATE_ERROR;
			}

			if (buffer.pEvents)
			{
				buffer.pEvents->Release();
			}

			BYTE* pPCMData;
			DWORD maxLen, currentLen;
			pcmBuffer->Lock(&pPCMData, &maxLen, &currentLen);

			DWORD nSamplesToRead = currentLen / sizeof(StereoSample_INT16);

			CopyData(output, pPCMData, min(nSamplesToRead, nSamples));

			pcmBuffer->Unlock();

			if (nSamplesToRead < nSamples)
			{
				WriteSilence(output + nSamplesToRead, nSamples - nSamplesToRead);

				if (nSamplesToRead == 0)
				{
					isStreaming = false;
					state = STREAM_STATE_FINISHED;
					return 0;
				}
			}

			state = STREAM_STATE_CONTINUE;
			return nSamplesToRead;
		}

		uint32 GetOutput(StereoSample_INT16* output, uint32 nSamples, STREAM_STATE& state) override
		{
			struct AutoInc
			{
				volatile int64& i;
				~AutoInc()
				{
					i++;
				}
			};

			AutoInc inc{ blockLock };
			blockLock++; // lock is odd

			// Since this function will be called by an audio thread, blocking objects are prohibited
			if (isStreaming)
			{	
				auto nSamplesWritten = Write_MP3ToPCM_AudioThread(output, nSamples, state);
				return nSamplesWritten;
			}
			else
			{
				WriteSilence(output, nSamples);
				state = STREAM_STATE_SILENCE;
				return nSamples;
			}

			// AutoInc => lock is now even
		}

		bool HasOutput() const override
		{
			return isStreaming;
		}

		void Free() override
		{
			delete this;
		}
	};

	struct MP3Loader : ILoaderSupervisor
	{
		IAudioInstallationSupervisor& installation;
		AutoRelease<IMFTransform> transformer; 
		AutoRelease<IMFMediaBuffer> mp3Buffer;
		AutoRelease<IMFSample> mp3Sample;
		AutoRelease<IMFSample> outputSample;
		AutoRelease<IMFMediaBuffer> pcmBuffer;

		DWORD inputId;
		DWORD outputId;

		uint32 nChannels;

		std::vector<uint8> scratchBuffer;

		MP3Loader(IAudioInstallationSupervisor& _installation, uint32 localChannels, uint32 reserveSeconds = 120) : installation(_installation), nChannels(localChannels)
		{
			scratchBuffer.reserve(44'100 * reserveSeconds * sizeof(int16) * nChannels);
			transformer = CreateMP32TransformerWith1InputAnd1OutputStream();

			HRESULT hr;

			enum { MAX_MP3_SIZE = 4_megabytes };
			VALIDATE(hr = MFCreateMemoryBuffer(MAX_MP3_SIZE, &mp3Buffer));

			hr = transformer->GetStreamIDs(1, &inputId, 1, &outputId);
			if (hr == E_NOTIMPL)
			{
				inputId = 0;
				outputId = 0;
			}
			else if FAILED(hr)
			{
				Throw(hr, "%s: transform->GetStreamIDs(1, &inputId, 1, &outputId);", __FUNCTION__);
			}

			VALIDATE(hr = MFCreateSample(&mp3Sample));
			VALIDATE(hr = mp3Sample->AddBuffer(mp3Buffer));

			AutoRelease<IMFMediaType> outputType;
			CreatePCMAudioType(44100, 16, nChannels, &outputType);

			AutoRelease<IMFMediaType> inputType;
			CreateMP3AudioType(44100, nChannels, &inputType);

			VALIDATE(hr = transformer->SetInputType(inputId, inputType, 0));

			for (UINT typeIndex = 0; ; ++typeIndex)
			{
				AutoRelease<IMFMediaType> optType;
				hr = transformer->GetOutputAvailableType(outputId, typeIndex, &optType);
				if FAILED(hr)
				{
					Throw(hr, "%s: (MP3 to PCM) transform->GetOutputAvailableType(outputId, typeIndex, optType) failed", __FUNCTION__);
				}

				if (IsPCMWithSpecifiedChannelCount(*optType, 44100, nChannels))
				{
					hr = transformer->SetOutputType(outputId, optType, 0);
					if FAILED(hr)
					{
						Throw(hr, "%s: (MP3 to PCM) transform->SetOutputType(0, outputType, 0)", __FUNCTION__);
					}

					break;
				}
			}

			MFT_INPUT_STREAM_INFO inputInfo;
			VALIDATE(hr = transformer->GetInputStreamInfo(inputId, &inputInfo));

			MFT_OUTPUT_STREAM_INFO outputInfo;
			VALIDATE(hr = transformer->GetOutputStreamInfo(outputId, &outputInfo));

			VALIDATE(hr = MFCreateMemoryBuffer(4410 * sizeof(int16) * nChannels, &pcmBuffer));
			VALIDATE(hr = MFCreateSample(&outputSample));
			VALIDATE(hr = outputSample->AddBuffer(pcmBuffer));
		}

		uint32 DecodeAudio(cstr utf8Path, IAudioSample& sample, IAudioSampleEvents& eventHandler, IPCMAudioBufferManager& audioBufferManager) override
		{
			UNUSED(sample);
			UNUSED(eventHandler);
			// Our algorithm expands the mp3 into a scratch buffer. This grows to accomodate the largest decoded MP3 put through the system
			// The sample we extract copies the relevant portion of the scratch buffer - this way we ensure the minimum of memory fragmentation.

			try
			{
				PopulateMediaBufferWithFileData(installation, utf8Path, *mp3Buffer);
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "%s:\n%s: %s", __FUNCTION__, utf8Path, ex.Message());
			}

			DWORD length = 0;
			mp3Buffer->GetCurrentLength(&length);

			// Assume 11:1 compression ratio. Increase ratio to 12 to try to minimize resizing
			DWORD approxDecompressedSize = (length * 12) & (0xFFFFFF00);

			scratchBuffer.resize(approxDecompressedSize);

			HRESULT hr;
			VALIDATE(hr = transformer->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0));
			VALIDATE(hr = transformer->ProcessInput(inputId, mp3Sample, 0));

			audioBufferManager.Accept(AudioBufferDescriptor{ nChannels, 16 });
			
			uint32 cursor = 0;

			int32 resizeCount = 0;

			while (true)
			{
				MFT_OUTPUT_DATA_BUFFER buffer;
				buffer.dwStreamID = outputId;
				buffer.pSample = outputSample;
				buffer.dwStatus = 0;
				buffer.pEvents = nullptr;

				DWORD status;
				hr = transformer->ProcessOutput(0, 1, &buffer, &status);

				if (buffer.pEvents)
				{
					buffer.pEvents->Release();
					buffer.pEvents = nullptr;
				}

				if FAILED(hr)
				{
					break;
				}

				BYTE* pPCMData;
				DWORD maxLen, currentLen;
				pcmBuffer->Lock(&pPCMData, &maxLen, &currentLen);

				uint32 targetWriteLength = cursor + currentLen;
				if (targetWriteLength > scratchBuffer.size())
				{
					scratchBuffer.resize(targetWriteLength);
					resizeCount++;
				}

				memcpy(scratchBuffer.data() + cursor, pPCMData, currentLen);
				cursor += currentLen;

				pcmBuffer->Unlock();	
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
	IAudioDecoder* CreateAudioDecoder_MP3_to_Stereo_16bit_int(IAudioInstallationSupervisor& installation, uint32 nSamplesInOutput)
	{
		return new AudioAnon::AudioDecoder(installation, nSamplesInOutput);
	}

	ILoaderSupervisor* CreateSingleThreadedMP3Loader(IAudioInstallationSupervisor& installation, uint32 nChannels)
	{
		return new AudioAnon::MP3Loader(installation, nChannels);
	}
}