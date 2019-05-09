#include <rococo.mplat.h>
#include <array>
#include <vector>

#include <al.h>
#include <alc.h>

#include <rococo.strings.h>

#pragma comment(lib, "OpenAL32.lib")

using namespace Rococo;
using namespace Rococo::Audio;

struct StereoSample
{
	int16 left;
	int16 right;
};

struct StereoSampleF32
{
	float left;
	float right;
};

inline float clamp_abs_1(float x)
{
	return x > 1.0f ? 1.0f : (x < -1.0f ? -1.0f : x);
}

inline int16 float_to_int16(float x)
{
	return (int16)(x * 32767.0f);
}

struct LegacySoundControl : public ILegacySoundControlSupervisor, public OS::IThreadJob
{
	float masterVolume = 1.0f;
	enum { CHANNEL_COUNT = 16 };
	ALCdevice* device = nullptr;
	ALCcontext* context = nullptr;
	ALuint primarySource = 0;
	OS::IThreadSupervisor* thread = nullptr;

	std::array <ALuint,3> bufferCycle;

	std::array<StereoSampleF32, 4410> f32raw;

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
		if (thread)
		{
			auto* msg = thread->GetErrorMessage();
			if (msg)
			{
				Throw(0, "LegacySoundThread terminated: %s", msg);
			}
		}

		auto err = alGetError();
		if (err != 0)
		{
			ClearAL();
			Throw(0, "Error with openAL (@%s): %u", helper, err);
		}

		if (context)
		{
			auto err = alcGetError(device);
			if (err != 0)
			{
				ClearAL();
				Throw(0, "Error with openAL context (@%s): %u", helper, err);
			}
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
		auto* devices = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
		int index = 0;
		for (auto* subDev = devices; *subDev != 0; subDev = subDev + strlen(subDev) + 1)
		{
			char spec[64];
			SafeFormat(spec, 64, "Device %d", index++);
			cb.OnEvent(StringKeyValuePairArg{ spec, subDev });
		}

		auto* vendor = alGetString(AL_VENDOR);
		cb.OnEvent(StringKeyValuePairArg{ "vendor", vendor });

		auto* version = alGetString(AL_VERSION);
		cb.OnEvent(StringKeyValuePairArg{ "version", version });

		auto* renderer = alGetString(AL_RENDERER);
		cb.OnEvent(StringKeyValuePairArg{ "renderer", renderer });

		auto* extensions = alGetString(AL_EXTENSIONS);
		cb.OnEvent(StringKeyValuePairArg{ "extensions", extensions });

		auto *defaultOutputDeviceSpecifier = alcGetString(device, ALC_DEFAULT_DEVICE_SPECIFIER);
		cb.OnEvent(StringKeyValuePairArg{ "default output device", defaultOutputDeviceSpecifier });

		auto *defaultCaptureDevice = alcGetString(device, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
		cb.OnEvent(StringKeyValuePairArg{ "default capture device", defaultCaptureDevice });

		auto *outputDeviceSpecifier = alcGetString(device, ALC_DEVICE_SPECIFIER);
		cb.OnEvent(StringKeyValuePairArg{ "output device", outputDeviceSpecifier });

		auto *captureDeviceSpecifier = alcGetString(device, ALC_CAPTURE_DEVICE_SPECIFIER);
		cb.OnEvent(StringKeyValuePairArg{ "capture device", captureDeviceSpecifier });

		auto *cextensions = alcGetString(device, ALC_CAPTURE_DEVICE_SPECIFIER);
		cb.OnEvent(StringKeyValuePairArg{ "c_extensions", cextensions });
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

	float cycle = 0;

	void RenderSound(StereoSampleF32* samples, int32 nSamples)
	{
		StereoSampleF32* s = samples;

		// playing samples at R samples per second
		// A sinewave of 1Hz takes R samples
		// A sinewave of f Hz takes R / f samples

		const float Middle_C = 261.625565f; // Hz

		float dt = Middle_C * 1.0f / 44100.0f; // sample time step
			
		const float TwoPi = 2.0f * 3.14159265358979f;

		for (int32 i = 0; i < nSamples; ++i)
		{
			s->left = s->right = sinf(TwoPi * cycle);
			cycle += dt;
			s++;
		}

		cycle = fmodf(cycle, 1.0f);
	}

	uint32 RunThread(OS::IThreadControl& tc) override
	{
		tc.SetRealTimePriority();

		std::vector<StereoSample> raw_int16(4410);
		for (auto& s : raw_int16)
		{
			s.left = s.right = 0;
		}

		alcMakeContextCurrent(context);
		AssertValidState("alcMakeContextCurrent - audio thread");

		alSourceQueueBuffers(primarySource, (ALsizei)bufferCycle.size(), bufferCycle.data());
		AssertValidState("alSourceQueueBuffers");

		alSourcePlay(primarySource);
		AssertValidState("primarySource");

		alListenerf(AL_GAIN, 0.25f);

		alSourcef(primarySource, AL_GAIN, 0.25f);
		alSourcef(primarySource, AL_PITCH, 1);

		while (tc.IsRunning())
		{
			tc.SleepUntilAysncEvent(5);

			ALint buffersProcessed;
			alGetSourcei(primarySource, AL_BUFFERS_PROCESSED, &buffersProcessed);
			AssertValidState("alGetSourcei AL_BUFFERS_PROCESSED");

			while (buffersProcessed > 0)
			{
				ALuint id;
				alSourceUnqueueBuffers(primarySource, 1, &id);
				AssertValidState("alSourceUnqueueBuffers");

				RenderSound(f32raw.data(), (int32)f32raw.size());

				auto* s = raw_int16.data();
				for (auto i = f32raw.begin(); i != f32raw.end(); ++i)
				{
					s->left = float_to_int16(clamp_abs_1(i->left));
					s->right = float_to_int16(clamp_abs_1(i->right));
					s++;
				}

				alBufferData(id, AL_FORMAT_STEREO16, raw_int16.data(), (ALsizei)(raw_int16.size() * sizeof(StereoSample)), 44100);
				AssertValidState("alBufferData");

				alSourceQueueBuffers(primarySource, 1, &id);
				AssertValidState("alSourceQueueBuffers");

				buffersProcessed--;
			}

			ALint isPlaying;
			alGetSourcei(primarySource, AL_SOURCE_STATE, &isPlaying);
			if (isPlaying != AL_PLAYING)
			{
				alSourcePlay(primarySource);
				AssertValidState("alSourcePlay");
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