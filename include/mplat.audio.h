#pragma once

namespace Rococo::Audio
{
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
		virtual void StreamInputFile(const wchar_t* sysPath) = 0;
		virtual void Free() = 0;
	};

	IAudioDecoder* CreateAudioDecoder_MP3_to_Stereo_16bit_int(uint32 nSamplesInOutput);
}