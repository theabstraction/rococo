#pragma once

#include <rococo.types.h>
#include <../rococo.mplat/mplat.sxh.h>

namespace Rococo::Audio
{
	ROCOCO_INTERFACE IAudioSample
	{
		// The key or filename associated with this sample
		virtual cstr Name() const = 0;
	};

	ROCOCO_INTERFACE IAudioSampleDatabase
	{
		// Bind a sample to the database by ping path. The returned reference is valid until the database is cleared with a call to IAudioSampleDatabase::Clear()
		virtual IAudioSample & Bind(cstr pingPath) = 0;
		virtual void Clear() = 0;
	};

	ROCOCO_INTERFACE IAudioSampleDatabaseSupervisor : IAudioSampleDatabase
	{
		virtual void Free() = 0;
	};

	IAudioSampleDatabaseSupervisor* CreateAudioSampleDatabase(IInstallation& installation, int nChannels);

	struct AudioBufferDescriptor
	{
		uint32 nChannels;
		uint32 bitsPerChannel;
	};

	struct PCMAudioLoadingMetrics
	{
		int32 resizeCount;
	};

	ROCOCO_INTERFACE IPCMAudioBufferManager
	{
		// Requests that the audio buffer accepts the data of type specified in the descriptor
		virtual bool Accept(const AudioBufferDescriptor & descriptor) = 0;
		virtual IExpandingBuffer& PCMBuffer() = 0;
		virtual void Finalize(const PCMAudioLoadingMetrics& metrics) = 0;
	};

	ROCOCO_INTERFACE IMP3LoaderSupervisor
	{
		// Loads the sample, fills in the decoded buffer via the audioBufferManager and returns the length of the buffer.
		virtual uint32 DecodeMP3(cstr pingPath, IPCMAudioBufferManager& audioBufferManager) = 0;
		virtual void Free() = 0;
	};

	IMP3LoaderSupervisor* CreateMP3Loader(IInstallation& installation, int32 nChannels);

	ROCOCO_INTERFACE IAudioSampleSupervisor : IAudioSample
	{
		// cache is called in the sample database thread to load the sample asynchronously from the main thread
		virtual void Cache(IMP3LoaderSupervisor & loader) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IAudioSupervisor : public IAudio
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IOSAudioVoice
	{
		virtual void QueueSample(const uint8* buffer, uint32 nBytesInBuffer, uint32 beginAt, uint32 nSamplesToPlay) = 0;
		virtual void StartPulling() = 0;
		virtual void Stop() = 0;
	};

	ROCOCO_INTERFACE IOSAudioVoiceCompletionHandler
	{
		virtual void OnSampleComplete(IOSAudioVoice& voice) = 0;
	};

	ROCOCO_INTERFACE IOSAudioVoiceSupervisor : IOSAudioVoice
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IOSAudioAPI
	{
		// Turn an audio API error code passed in audio exceptions into an error message. The buffer is filled up to the capacity, truncated and terminated will a nul character
		virtual void TranslateErrorCode(int errCode, char* msg, size_t capacity) = 0;
		virtual IOSAudioVoiceSupervisor* Create16bitStereo44100kHzVoice(IOSAudioVoiceCompletionHandler& completionHandler) = 0;
	};

	ROCOCO_INTERFACE IOSAudioAPISupervisor : IOSAudioAPI
	{
		virtual void Free() = 0;
	};

	IOSAudioAPISupervisor* CreateOSAudio();

	struct AudioConfig
	{
		int unused;
	};

	IAudioSupervisor* CreateAudioSupervisor(IInstallation& installation, IOSAudioAPI& osAPI, const AudioConfig& config);

#pragma pack(push,1)
	struct StereoSample_INT16
	{
		int16 left;
		int16 right;
	};
#pragma pack(pop)

	struct PCMStereo
	{
		const StereoSample_INT16* samples;
		const uint32 sampleCount;
	};

	enum STREAM_STATE
	{
		STREAM_STATE_CONTINUE,
		STREAM_STATE_SILENCE,
		STREAM_STATE_FINISHED,
		STREAM_STATE_ERROR
	};

	ROCOCO_INTERFACE IAudioDecoder
	{
		// Consumer periodically calls GetOutput, fills in the sample buffer and returns the number of samples written
		virtual uint32 GetOutput(StereoSample_INT16* samples, uint32 nSamples, OUT STREAM_STATE& state) = 0;
		virtual bool HasOutput() const = 0;
		virtual void StreamInputFile(const wchar_t* sysPath) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IAudioStreamer
	{
		virtual void Start() = 0;
		virtual void Stop() = 0;
		virtual void StreamCurrentBlock() = 0;
	};

	ROCOCO_INTERFACE IAudioStreamerSupervisor : IAudioStreamer
	{
		virtual void Free() = 0;
	};

	IAudioStreamerSupervisor* CreateStereoStreamer(IOSAudioAPI& osAudio, IAudioDecoder& refDecoder);

	IAudioDecoder* CreateAudioDecoder_MP3_to_Stereo_16bit_int(uint32 nSamplesInOutput);
}