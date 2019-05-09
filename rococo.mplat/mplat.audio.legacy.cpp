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

typedef int64 SampleTime;

struct ADSR
{
	float attack;
	float decay;
	float sustain;
	float release;
	SampleTime startTime;
	bool sustained;
	bool released;
	SampleTime sustainEnded;
};

void GetADSRModulation_ForAttackPhase(const ADSR& adsr, float& volume0, float& volume1, float tStart, float tEnd)
{
	volume0 = tStart / adsr.attack;
	volume1 = tEnd >= adsr.attack ? 1.0f : tEnd / adsr.attack;
}

void GetADSRModulation_ForDecayPhase(const ADSR& adsr, float& volume0, float& volume1, float tStart, float tEnd)
{
	volume0 = Lerp(1.0f, adsr.sustain, (tStart - adsr.attack) / adsr.decay);
	if (tEnd > adsr.attack + adsr.decay)
	{
		volume1 = adsr.sustain;
	}
	else
	{
		volume1 = Lerp(1.0f, adsr.sustain, (tEnd - adsr.attack) / adsr.decay);
	}
}

void GetADSRModulation_ForSustainAndReleasePhase(const ADSR& adsr, float& volume0, float& volume1, float tStart, float tEnd, float sustainPeriod)
{
	if (adsr.sustained)
	{
		volume0 = volume1 = adsr.sustain;
		return;
	}

	if (sustainPeriod >= adsr.release)
	{
		volume0 = volume1 = 0.0f;
		return;
	}
	
	volume0 = Lerp(adsr.sustain, 0.0f, sustainPeriod / adsr.release);

	float sustainPeriodAtSampleEnd = sustainPeriod + tEnd - tStart;

	if (sustainPeriodAtSampleEnd > adsr.release)
	{
		volume1 = 0.0f;
	}
	else
	{
		volume1 = Lerp(adsr.sustain, 0.0f, sustainPeriodAtSampleEnd / adsr.release);
	}
}

void GetADSRModulation(const ADSR& adsr, float& volume0, float& volume1, SampleTime cursorTime, int64 nSamples)
{
	volume0 = volume1 = 1.0f;
	return;

	auto samplesPlayedSinceStart = cursorTime - adsr.startTime;

	float tStart = (float) samplesPlayedSinceStart;
	float tEnd = tStart + nSamples;

	if (tStart < adsr.attack)
	{
		GetADSRModulation_ForAttackPhase(adsr, volume0, volume1, tStart, tEnd);
	}
	else if (tStart < (adsr.attack + adsr.decay))
	{
		GetADSRModulation_ForDecayPhase(adsr, volume0, volume1, tStart, tEnd);
	}
	else
	{
		float sustainPeriod = (float)(cursorTime - adsr.sustainEnded);
		GetADSRModulation_ForSustainAndReleasePhase(adsr, volume0, volume1, tStart, tEnd, sustainPeriod);
	}
}

struct LegacySoundControl : public ILegacySoundControlSupervisor, public OS::IThreadJob
{
	enum { CHANNEL_COUNT = 16 };
	ALCdevice* device = nullptr;
	ALCcontext* context = nullptr;
	ALuint primarySource = 0;
	OS::IThreadSupervisor* thread = nullptr;

	std::array<ALuint,3> bufferCycle;

	std::array<StereoSampleF32, 4410> f32raw;

	struct Channel
	{
		float leftVolume = 0;
		float rightVolume = 0;
		float freqHz = 0;
		float dutyCycle = 0;
		ADSR adsr = { 0 };
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

		for (auto c : channels)
		{
			c.adsr.released = true;
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

	void SetMasterVolumne(float gain) override
	{
		alSourcef(primarySource, AL_GAIN, gain);
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

	void SetWave(int32 channel, Rococo::Audio::ELegacySoundShape waveShape, float freqHz, float dutyCycle)  override
	{
		AssertChannelValid(channel, __FUNCTION__);
		auto& c = channels[channel];
		c.freqHz = freqHz;
		c.dutyCycle = dutyCycle;
		c.waveShape = waveShape;
	}

	void SetEnvelope(int32 channel, float attackSecs, float decaySecs, float sustainLevel, float releaseSecs) override
	{
		AssertChannelValid(channel, __FUNCTION__);
		auto& c = channels[channel];
		c.adsr.attack = attackSecs * 44100.0f;
		c.adsr.decay = decaySecs * 44100.0f;
		c.adsr.sustain = sustainLevel;
		c.adsr.release = releaseSecs * 44100.0f;
	}

	void PlayWave(int32 channel) override
	{
		AssertChannelValid(channel, __FUNCTION__);
		auto& c = channels[channel];
		c.adsr.startTime = cursorTime;
		c.adsr.sustained = false;
		c.adsr.released = true;
		c.adsr.sustainEnded = (SampleTime) (cursorTime + c.adsr.attack + c.adsr.decay);
	}

	void Sustain(int32 channel) override
	{
		AssertChannelValid(channel, __FUNCTION__);
		auto& c = channels[channel];

		if (c.adsr.released)
		{
			c.adsr.startTime = cursorTime;
			c.adsr.released = false;
		}

		SampleTime leadTime = (SampleTime)(c.adsr.attack + c.adsr.decay);
		
		c.adsr.sustained = true;
		c.adsr.sustainEnded = (cursorTime - c.adsr.startTime > leadTime) ? cursorTime : c.adsr.startTime + leadTime;
	}

	void Release(int32 channel) override
	{
		AssertChannelValid(channel, __FUNCTION__);
		auto& c = channels[channel];

		if (!c.adsr.released)
		{
			c.adsr.sustained = false;
			c.adsr.released = true;
			c.adsr.sustainEnded = cursorTime;
		}
	}

	float cycleTime = 0;

	SampleTime cursorTime = 0;

	void AddSineWave(StereoSampleF32* samples, int32 nSamples, float freqHz, float volume0, float volume1, float leftVolume, float rightVolume)
	{
		StereoSampleF32* s = samples;

		// playing samples at R samples per second
		// A sinewave of 1Hz takes R samples
		// A sinewave of f Hz takes R / f samples

		float dt = 1.0f / 44100.0f; // sample time step

		const float TwoPi = 2.0f * 3.14159265358979f;

		const float w = TwoPi * freqHz;

		float t = cycleTime;

		float volume = volume0;

		float dv = (volume1 - volume0) * dt;

		for (int32 i = 0; i < nSamples; ++i)
		{
			float delta = volume * sinf(w * t);
			s->left += leftVolume * delta;
			s->right += rightVolume * delta;
			t += dt;
			volume += dv;
			s++;
		}
	}

	void MakeSilent(StereoSampleF32* samples, int32 nSamples)
	{
		memset(samples, 0, nSamples * sizeof(StereoSampleF32));
	}

	void AddChannelToBuffer(Channel& channel, StereoSampleF32* samples, int32 nSamples)
	{
		if (channel.adsr.startTime == 0)
		{
			return;
		}

		float volume0, volume1;
		GetADSRModulation(channel.adsr, volume0, volume1, cursorTime, nSamples);

		if ((channel.leftVolume <= 0 && channel.rightVolume <= 0) | (volume0 <= 0 && volume1 <= 0))
		{
			return;
		}

		switch (channel.waveShape)
		{
		case ELegacySoundShape_Sine:
			AddSineWave(samples, nSamples, channel.freqHz, volume0, volume1, channel.leftVolume, channel.rightVolume);
			break;
		case ELegacySoundShape_Square:
		case ELegacySoundShape_Triangle:
		case ELegacySoundShape_Saw:
		case ELegacySoundShape_Noise:
		default:
			break;
		}
	}

	void RenderSound(StereoSampleF32* samples, int32 nSamples)
	{
		MakeSilent(samples, nSamples);

		for (auto& channel : channels)
		{
			AddChannelToBuffer(channel, samples, nSamples);
		}

		float dt = 1.0f / 44100.0f; // sample time step
		cycleTime = fmodf(cycleTime + nSamples * dt, 1.0f);

		cursorTime += nSamples;
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

		alSourcef(primarySource, AL_PITCH, 1);

		cursorTime = 0;

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