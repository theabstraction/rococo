#pragma once

#include <rococo.types.h>
#include <../rococo.audio/rococo.audio.types.h>

namespace Rococo::Audio
{
	using IdInstrumentIndexType = uint8; // This gives up to 256 instruments.
	using IdInstrumentSaltType = uint32;

	ROCOCO_ID(IdSample, uint64, 0);

#pragma pack(push,1)
	class IdInstrument
	{
	private:
		IdInstrumentIndexType index;
		IdInstrumentSaltType salt;

	public:
		IdInstrument() : index(0), salt(0)
		{

		}

		static IdInstrument None()
		{
			return IdInstrument();
		}

		explicit IdInstrument(IdInstrumentIndexType l_index, IdInstrumentSaltType l_salt) : index(l_index), salt(l_salt)
		{
		}

		IdInstrumentIndexType Index() const
		{
			return index;
		}

		IdInstrumentSaltType Salt() const
		{
			return salt;
		}

		operator bool() const
		{
			return index > 0;
		}
};
#pragma pack(pop)
}

#include "../rococo.audio/rococo.audio.sxh.h"

#ifndef ROCOCO_AUDIO_API
# define ROCOCO_AUDIO_API __declspec(dllimport)
#endif

namespace Rococo::Script
{
	struct IPublicScriptSystem;
}

namespace Rococo::Audio
{
	/****  Internal API, not for external code  ****/
	void* AudioAlignedAlloc(size_t nBytes, int32 alignment);
	void AudioAlignedFree(void* buffer);
	/***********************************************/
}

namespace Rococo::Audio
{
	ROCOCO_AUDIO_API void SetAudioAllocator(IAllocator* allocator);

	ROCOCO_INTERFACE IAudioSample
	{
		// The unique id that can be used to quickly lookup a sample from a sample database
		virtual IdSample Id() const = 0;

		// The key or filename associated with this sample
		virtual cstr Name() const = 0;

		// Atomic and thread safe operation that returns whether the sample is ready to be played
		virtual bool IsLoaded() const = 0;

		virtual uint32 ReadSamples(int16* outputBuffer, uint32 outputBufferSampleCapacity, uint32 startAtSampleIndex) = 0;
	};

	ROCOCO_INTERFACE IAudioSampleDatabase
	{
		// Bind a sample to the database by ping path. The returned reference is valid until the database is cleared with a call to IAudioSampleDatabase::Clear()
		virtual IAudioSample & Bind(cstr pingPath) = 0;
		virtual IAudioSample* Find(IdSample id) const = 0;
		virtual uint32 NumberOfChannels() const = 0;
		virtual void Clear() = 0;
	};

	ROCOCO_INTERFACE IAudioSampleDatabaseSupervisor : IAudioSampleDatabase
	{
		virtual void Free() = 0;
	};

	ROCOCO_AUDIO_API IAudioSampleDatabaseSupervisor* CreateAudioSampleDatabase(IInstallation& installation, int nChannels, IEventCallback<IAudioSample&>& onSampleLoaded);

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

	// Creates an MP3 loader - optimized for a single thread access. Every loading thread should create its own instance
	ROCOCO_AUDIO_API IMP3LoaderSupervisor* CreateSingleThreadedMP3Loader(IInstallation& installation, uint32 nChannels);

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

	ROCOCO_INTERFACE IAudioVoiceContext
	{

	};

	ROCOCO_INTERFACE IOSAudioVoiceCompletionHandler
	{
		virtual void OnSampleComplete(IOSAudioVoice& voice, IAudioVoiceContext& context) = 0;
	};

	ROCOCO_INTERFACE IOSAudioVoiceSupervisor : IOSAudioVoice
	{
		virtual void Free() = 0;
	};

	struct Audio3DObjectFrame
	{
		Vec3 facingDirection;
		Vec3 upDirection; // Must be orthnormal with the facing direction
		Vec3 position; // world co-ordinates
		Vec3 dopplerVelocity;
	};

	ROCOCO_INTERFACE IAudio3DEmitter
	{
		virtual void SetFrame(const Audio3DObjectFrame& frame) = 0;
	};

	ROCOCO_INTERFACE IAudio3DEmitterSupervisor : IAudio3DEmitter
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IAudio3D
	{
		virtual IAudio3DEmitterSupervisor* CreateEmitter() = 0;
		virtual void UpdateSpeakerConfig(float speedOfSoundInMetresPerSecond) = 0;
	};

	ROCOCO_INTERFACE IAudio3DSupervisor : IAudio3D
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IOSAudioAPI
	{
		// Turn an audio API error code passed in audio exceptions into an error message. The buffer is filled up to the capacity, truncated and terminated will a nul character
		virtual void TranslateErrorCode(int errCode, char* msg, size_t capacity) = 0;
		virtual IOSAudioVoiceSupervisor* Create16bitMono44100kHzVoice(IOSAudioVoiceCompletionHandler& completionHandler, IAudioVoiceContext& context) = 0;
		virtual IOSAudioVoiceSupervisor* Create16bitStereo44100kHzVoice(IOSAudioVoiceCompletionHandler& completionHandler, IAudioVoiceContext& context) = 0;
		virtual IAudio3DSupervisor* Create3DAPI(float speedOfSoundInMetresPerSecond) = 0;
	};

	ROCOCO_INTERFACE IOSAudioAPISupervisor : IOSAudioAPI
	{
		virtual void Free() = 0;
	};

	ROCOCO_AUDIO_API IOSAudioAPISupervisor* CreateOSAudio();

	struct AudioConfig
	{
		int unused;
	};

	ROCOCO_AUDIO_API IAudioSupervisor* CreateAudioSupervisor(IInstallation& installation, IOSAudioAPI& osAPI, const AudioConfig& config);

#pragma pack(push,1)
	struct StereoSample_INT16
	{
		int16 left;
		int16 right;
	};

	struct MonoSample_INT16
	{
		int16 data;
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

	// Concertgoer - the listener or the observer. Placed at the origin of the concert, he faces North with +ve y, his right ear faces East with +ve x, and upwards being +ve z
	ROCOCO_INTERFACE IConcertGoer
	{
		// Orientates world co-ordinates into the frame of the goer. If X is a position supplied to the concert, then X' = worldToGoer.X, where X' is the position co-ordinate in the concertgoer space
		virtual void SetOrientation(cr_m4x4 worldToGoer) = 0;
	};

	ROCOCO_INTERFACE IConcertInstrument
	{

	};

	ROCOCO_INTERFACE IConcert3D
	{
		virtual IConcertGoer & Goer() = 0;

		// Get the next free instrument. Can fail if nothing is free
		virtual IdInstrument AssignFreeInstrument(IdSample sampleId, const Rococo::Audio::AudioSource3D& source) = 0;

		// Claim an instrument. Invalidates some existing id and assigns a new id. Can fail if everything else has higher priority
		virtual IdInstrument AssignInstrumentByPriority(IdSample sampleId, const Rococo::Audio::AudioSource3D& source) = 0;

		// Forceably claim an instrument. Invalidates some existing id and assigns a new id. It will take over an instrument slot with the least priority, or the earliest slot if all priorities match
		virtual IdInstrument AssignInstrumentAlways(IdSample sampleId, const Rococo::Audio::AudioSource3D& sourcey) = 0;

		// Frees the instrument slot when the current sample has completed
		virtual bool FreeAtEnd(IdInstrument id) = 0;

		// Frees the instrument if the current output is silent
		virtual bool FreeIfSilent(IdInstrument id) = 0;

		// Retrieves the friendly name/path supplied at assignment
		virtual bool GetName(IdInstrument id, U8FilePath& name) = 0;

		// Retrieves the priority at assignment
		virtual bool GetPriority(IdInstrument id, OUT int& priority) = 0;

		// Begins playing the instrument
		virtual bool Play(IdInstrument id) = 0;

		// Pause the playing
		virtual bool Pause(IdInstrument id) = 0;

		// Stop the playing and resets the play cursor to the start
		virtual bool Stop(IdInstrument id, bool andFree) = 0;

		// Sets the next sample to play
		virtual bool SetSample(IdInstrument id, IdSample sample) = 0;

		// Sets the volume of the source material
		virtual bool SetVolume(IdInstrument id, float volume) = 0;

		// Sets the doppler velocity of the source instrument's sound. Note that the position of the instrument itself does not update over time 
		virtual bool SetDopplerVelocity(IdInstrument id, cr_vec3 velocity) = 0;

		// Sets the location of the source instrument in world space
		virtual bool SetPosition(IdInstrument id, cr_vec3 position) = 0;
	};

	ROCOCO_INTERFACE IConcert3DSupervisor : IConcert3D
	{
		virtual void Free() = 0;
		virtual void OnSampleLoaded(IAudioSample& sample) = 0;
	};

	ROCOCO_AUDIO_API IConcert3DSupervisor* CreateConcert(IAudioSampleDatabase& database, IOSAudioAPI& audio);

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

	ROCOCO_AUDIO_API IAudioStreamerSupervisor* CreateStereoStreamer(IOSAudioAPI& osAudio, IAudioDecoder& refDecoder);

	ROCOCO_AUDIO_API IAudioDecoder* CreateAudioDecoder_MP3_to_Stereo_16bit_int(uint32 nSamplesInOutput);

	ROCOCO_AUDIO_API void DLL_AddNativeCalls_RococoAudioIAudio(Rococo::Script::IPublicScriptSystem& ss, IAudio* nceContext);
} // Rococo::Audio