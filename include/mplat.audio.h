#pragma once

namespace Rococo::Audio
{
	ROCOCOAPI IAudioSupervisor : public IAudio
	{
		virtual void Free() = 0;
	};

	struct AudioConfig
	{
		int unused;
	};

	IAudioSupervisor* CreateAudioSupervisor(IInstallation& installation, const AudioConfig& config);

#pragma pack(push,1)
	struct I16StereoSample
	{
		int16 left;
		int16 right;
	};
#pragma pack(pop)

	struct PCMStereo
	{
		const I16StereoSample* samples;
		const uint32 sampleCount;
	};

	enum STREAM_STATE
	{
		STREAM_STATE_CONTINUE,
		STREAM_STATE_SILENCE,
		STREAM_STATE_FINISHED,
		STREAM_STATE_ERROR
	};

	ROCOCOAPI IAudioDecoder
	{
		// Consumer periodically calls GetOutput, fills in the sample buffer and returns the number of samples written
		virtual uint32 GetOutput(I16StereoSample* samples, uint32 nSamples, OUT STREAM_STATE& state) = 0;
		virtual void StreamInputFile(const wchar_t* sysPath) = 0;
		virtual void Free() = 0;
	};

	IAudioDecoder* CreateAudioDecoder_MP3_to_Stereo_16bit_int(uint32 nSamplesInOutput);
}