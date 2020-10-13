#include <rococo.mplat.h>
#include <array>

#include <al.h>
#include <alc.h>

#include <rococo.strings.h>
#include <mplat.audio.h>

using namespace Rococo;
using namespace Rococo::Audio;

namespace
{
	struct StereoSampleF32
	{
		float left;
		float right;
	};

	void SetWhiteNoise(I16StereoSample* samples, size_t capacity)
	{
		auto* end = samples + capacity;
		for (auto* s = samples; s < end; ++s)
		{
			s->left = rand();
			s->right = rand();
		}
	}

	struct AudioPlayer: IAudioSupervisor, OS::IThreadJob
	{
		ALCdevice* device = nullptr;
		ALCcontext* context = nullptr;
		OS::IThreadSupervisor* thread = nullptr;
		ALuint primarySource = 0;

		IInstallation& installation;
		const AudioConfig config;

		std::array<ALuint, 3> bufferCycle;

		AutoFree<IAudioDecoder> decoder;

		bool playMusic = false;

		void AudioThreadPopulate(I16StereoSample* samples, size_t capacity)
		{
			if (playMusic)
			{
			//	STREAM_STATE state;
//				decoder->GetOutput(samples, (uint32) capacity, OUT state);
			}
		}

		void AssertValidState(cstr helper)
		{
			if (thread)
			{
				int err;
				auto* msg = thread->GetErrorMessage(err);
				if (msg)
				{
					Throw(err, "AudioPlayer terminated: %s", msg);
				}
			}

			auto err = alGetError();
			if (err != 0)
			{
				char msg[32];
				auto serr = alGetString(err);
				SafeFormat(msg, "%s", serr);
				ClearAL();
				Throw(0, "Error with openAL (@%s): %u %s", helper, err, msg);
			}

			if (context)
			{
				auto err = alcGetError(device);
				if (err != 0)
				{
					char msg[32];
					auto serr = alGetString(err);
					SafeFormat(msg, "%s", serr);
					ClearAL();
					Throw(0, "Error with openAL context (@%s): %u", helper, err, serr);
				}
			}
		}


		AudioPlayer(IInstallation& ref_installation, const AudioConfig& cr_config):
			installation(ref_installation), config(cr_config)
		{
			static_assert(sizeof(I16StereoSample) == 4);

			device = alcOpenDevice(nullptr);
			if (device == nullptr) Throw(0, "Could not create OpenAL sound device");

			AssertValidState("alcOpenDevice"); // doc says to call this before creating buffers;

			context = alcCreateContext(device, nullptr);
			if (context == nullptr)
			{
				ClearAL();
				Throw(0, "alcCreateContext failed");
			}

			alcMakeContextCurrent(context);

			AssertValidState("alcMakeContextCurrent");
			alGenBuffers(3, bufferCycle.data());
			AssertValidState("alGenBuffers");

			std::array<I16StereoSample, 4410> raw;
			std::fill(raw.begin(), raw.end(), I16StereoSample{ 0,0 });

			for (auto bufferId : bufferCycle)
			{
				alBufferData(bufferId, AL_FORMAT_STEREO16, raw.data(), (ALsizei)(sizeof(I16StereoSample) * raw.size()), 44100);
				AssertValidState("alBufferData");
			}

			alGenSources(1, &primarySource);
			AssertValidState("alGenSources");

	//		thread = OS::CreateRococoThread(this, 0);
	//		thread->Resume();
		}

		~AudioPlayer()
		{
			if (thread) thread->Free();
			ClearAL();
		}

		void ClearAL()
		{
			alcMakeContextCurrent(nullptr);
			if (context) alcDestroyContext(context);
			if (device) alcCloseDevice(device);
		}

		AutoFree<IExpandingBuffer> musicMP3Buffer = CreateExpandingBuffer(128_kilobytes);

		void SetMusic(const fstring& filename) override
		{
			AssertValidState("SetMusic");

			if (filename.length == 0)
			{
				Throw(0, "%s blank filename", __FUNCTION__);
			}

			if (!EndsWith(filename, ".mp3"))
			{
				Throw(0, "%s(%s) filename must end with .mp3", __FUNCTION__, filename.buffer);
			}

			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(filename, sysPath);

			if (!decoder)
			{
				decoder = CreateAudioDecoder_MP3_to_Stereo_16bit_int(4410);
			}

		//	decoder->StreamInputFile(sysPath);

			playMusic = true;
		}

		void Free() override
		{
			double dt = maxTicks / (double)OS::CpuHz();
			delete this;
		}

		std::array<I16StereoSample, 4410> raw_int16;

		OS::ticks maxTicks = 0;

		uint32 RunThread(OS::IThreadControl& tc) override
		{
			tc.SetRealTimePriority();

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

			while (tc.IsRunning())
			{
				tc.SleepUntilAysncEvent(10);

				ALint buffersProcessed;
				alGetSourcei(primarySource, AL_BUFFERS_PROCESSED, &buffersProcessed);
			//	AssertValidState("alGetSourcei AL_BUFFERS_PROCESSED");

				while (buffersProcessed > 0)
				{
					ALuint id;
					alSourceUnqueueBuffers(primarySource, 1, &id);
				//	AssertValidState("alSourceUnqueueBuffers");

					auto start = OS::CpuTicks();
					AudioThreadPopulate(raw_int16.data(), raw_int16.size());
					auto end = OS::CpuTicks();

					auto dt = end - start;
					
					maxTicks = max(maxTicks, dt);

					alBufferData(id, AL_FORMAT_STEREO16, raw_int16.data(), (ALsizei)(raw_int16.size() * sizeof(I16StereoSample)), 44100);
				//	AssertValidState("alBufferData");

					alSourceQueueBuffers(primarySource, 1, &id);
				//	AssertValidState("alSourceQueueBuffers");

					buffersProcessed--;
				}

				ALint isPlaying;
				alGetSourcei(primarySource, AL_SOURCE_STATE, &isPlaying);
				if (isPlaying != AL_PLAYING)
				{
					alSourcePlay(primarySource);
				//	AssertValidState("alSourcePlay");
				}
			}

			return 0;
		}
	};
}

namespace Rococo::Audio
{
	// This OpenAL implementation seems to bug the IAudioDecoder transform
	// Not sure why...so stick with X2 until this is needed
	IAudioSupervisor* CreateAudioSupervisor_OpenAL(IInstallation& installation, const AudioConfig& config)
	{
		return new AudioPlayer(installation, config);
	}
}