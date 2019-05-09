#include <rococo.mplat.h>
#include <array>
#include <vector>

#include <al.h>
#include <alc.h>

#pragma comment(lib, "OpenAL32.lib")

using namespace Rococo;
using namespace Rococo::Audio;

struct StereoSample
{
	int16 left;
	int16 right;
};

struct LegacySoundControl : public ILegacySoundControlSupervisor, public OS::IThreadJob
{
	float masterVolume = 1.0f;
	enum { CHANNEL_COUNT = 16 };
	ALCdevice* device = nullptr;
	ALCcontext* context = nullptr;
	ALuint primarySource = 0;
	OS::IThreadSupervisor* thread = nullptr;

	std::array <ALuint,3> bufferCycle;

	struct Channel
	{
		float leftVolume;
		float rightVolume;
		float freqHz;
		float dutyCycle;
		float attack;
		float decay;
		float sustain;
		float release;
		Rococo::Audio::ELegacySoundShape waveShape;
	};
	
	std::array<Channel, CHANNEL_COUNT> channels;

	void ClearAL()
	{
		alcMakeContextCurrent(nullptr);
		if (context) alcDestroyContext(context);
		if (device) alcCloseDevice(device);
	}

	void AssertValidState(cstr helper)
	{
		auto err = alGetError();
		if (err != 0)
		{
			ClearAL();
			Throw(0, "Error creating openAL buffer (@%s): %u", helper, err);
		}
	}

	LegacySoundControl()
	{
		device = alcOpenDevice(nullptr);
		if (device == nullptr) Throw(0, "Could not create OpenAL sound device");

		AssertValidState("alcOpenDevice"); // doc says to call this before creating buffers;

		context = alcCreateContext(device, nullptr); 
		if (context == nullptr)
		{
			ClearAL();
		}

		alcMakeContextCurrent(context);

		AssertValidState("alcMakeContextCurrent");
		alGenBuffers(3, bufferCycle.data());
		AssertValidState("alGenBuffers");

		std::vector<StereoSample> raw(4410);
		for (auto& s : raw)
		{
			s.left = s.right = 0;
		}

		for (auto bufferId : bufferCycle)
		{
			alBufferData(bufferId, AL_FORMAT_STEREO16, raw.data(), (ALsizei) (sizeof(StereoSample) * raw.size()), 44100);
			AssertValidState("alBufferData");
		}
		
		alGenSources(1, &primarySource);
		AssertValidState("alGenSources");

		alSourcei(primarySource, AL_LOOPING, AL_TRUE);
		AssertValidState("alSourcei LOOPING");

		thread = OS::CreateRococoThread(this, 0);
		thread->Resume();
	}

	~LegacySoundControl()
	{
		if (thread) thread->Free();
		ClearAL();
	}

	void EnumerateDeviceDesc(IEventCallback<StringKeyValuePairArg>& cb) override
	{
		auto* vendor = alGetString(AL_VENDOR);
		cb.OnEvent(StringKeyValuePairArg{ "vendor", vendor });

		auto* version = alGetString(AL_VERSION);
		cb.OnEvent(StringKeyValuePairArg{ "version", version });

		auto* renderer = alGetString(AL_RENDERER);
		cb.OnEvent(StringKeyValuePairArg{ "renderer", renderer });

		auto* extensions = alGetString(AL_EXTENSIONS);
		cb.OnEvent(StringKeyValuePairArg{ "extensions", extensions });
	}

	void Free() override
	{
		delete this;
	}

	void SetMasterVolumne(float volume) override
	{
		masterVolume = volume;
	}

	void AssertChannelValid(int32 channel, cstr function)
	{
		if (channel < 0 || channel >= CHANNEL_COUNT)
		{
			Throw(0, "%s: Channel %d out of bounds: 0 to %d-1", function, channel, CHANNEL_COUNT);
		}
	}

	void SetChannelVolume(int32 channel, float leftVolume, float rightVolume)  override
	{
		AssertChannelValid(channel, __FUNCTION__);
		auto& c = channels[channel];
		c.leftVolume = leftVolume;
		c.rightVolume = rightVolume;
	}

	void PlayWave(int32 channel, Rococo::Audio::ELegacySoundShape waveShape, float freqHz, float dutyCycle)  override
	{
		AssertChannelValid(channel, __FUNCTION__);
		auto& c = channels[channel];
		c.freqHz = freqHz;
		c.dutyCycle = dutyCycle;
	}

	void SetEnvelope(int32 channel, float attack, float decay, float sustain, float release) override
	{
		AssertChannelValid(channel, __FUNCTION__);
		auto& c = channels[channel];
		c.attack = attack;
		c.decay = decay;
		c.sustain = sustain;
		c.release = release;
	}

	void RenderSound(StereoSample* samples, size_t nSamples)
	{

	}

	uint32 RunThread(OS::IThreadControl& tc) override
	{
		tc.SetRealTimePriority();

		std::vector<StereoSample> raw(4410);
		for (auto& s : raw)
		{
			s.left = s.right = 0;
		}

		alSourceQueueBuffers(primarySource, (ALsizei) bufferCycle.size(), bufferCycle.data());
		alSourcePlay(primarySource);

		while (tc.IsRunning())
		{
			tc.SleepUntilAysncEvent(5);

			ALint buffersProcessed;
			alGetSourcei(primarySource, AL_BUFFERS_PROCESSED, &buffersProcessed);

			while (buffersProcessed > 0)
			{
				ALuint id;
				alSourceUnqueueBuffers(primarySource, 1, &id);

				RenderSound(raw.data(), raw.size());
				alBufferData(primarySource, AL_FORMAT_STEREO16, raw.data(),  (ALsizei)(raw.size() * sizeof(StereoSample)), 44100);

				alSourceQueueBuffers(primarySource, 1, &id);

				buffersProcessed--;
			}

			ALint isPlaying;
			alGetSourcei(primarySource, AL_SOURCE_STATE, &isPlaying);
			if (isPlaying != AL_PLAYING)
			{
				alSourcePlay(primarySource);
			}
		}

		return 0;
	}
};

namespace Rococo
{
	namespace Audio
	{
		ILegacySoundControlSupervisor* CreateLegacySoundControl()
		{
			return new LegacySoundControl();
		}
	}
}