#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <mplat.audio.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Audio;

namespace
{
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

	class Stereo_Streamer: public IAudioStreamerSupervisor, public IOSAudioVoiceCompletionHandler
	{
		IAudioDecoder& decoder;

		// Stereo 44.1kHz 16-bit per channel stereo voice
		AutoFree<IOSAudioVoiceSupervisor> stereoVoice;

		std::vector<StereoSample_INT16*> pcm_blocks;
		volatile size_t currentIndex = 0;

		bool isPlaying = false;

		enum { SAMPLES_PER_BLOCK = 4096, BEST_SAMPLE_RATE = 44100, PCM_BLOCK_COUNT = 4 };
	public:
		Stereo_Streamer(IOSAudioAPI& osAudio, IAudioDecoder& refDecoder): decoder(refDecoder)
		{
			size_t nBytesPerBlock = SAMPLES_PER_BLOCK * sizeof(StereoSample_INT16);
			pcm_blocks.resize(PCM_BLOCK_COUNT);
			for (int i = 0; i < PCM_BLOCK_COUNT; ++i)
			{
				pcm_blocks[i] = (StereoSample_INT16*)_aligned_malloc(nBytesPerBlock, 64);
				memset(pcm_blocks[i], 0, nBytesPerBlock);
			}

			// Stereo 44.1kHz 16-bit per channel stereo voice
			stereoVoice = osAudio.Create16bitStereo44100kHzVoice(*this);
		}

		~Stereo_Streamer()
		{
			for (int i = 0; i < PCM_BLOCK_COUNT; ++i)
			{
				_aligned_free(pcm_blocks[i]);
			}
		}

		void Free() override
		{
			delete this;
		}

		void Start() override
		{
			stereoVoice->StartPulling();
		}

		void Stop() override
		{
			stereoVoice->Stop();
		}

		void StreamCurrentBlock() override
		{
			uint32 nSamples = SAMPLES_PER_BLOCK;

			StereoSample_INT16* sampleBuffer = pcm_blocks[currentIndex];

			currentIndex = (currentIndex + 1) % PCM_BLOCK_COUNT;

			if (isPlaying)
			{
				STREAM_STATE state;
				nSamples = decoder.GetOutput(sampleBuffer, SAMPLES_PER_BLOCK, OUT state);
			}

			stereoVoice->QueueSample((uint8*)sampleBuffer, SAMPLES_PER_BLOCK * sizeof(StereoSample_INT16), 0, nSamples);
		}

		void OnSampleComplete(IOSAudioVoice& voice) override
		{
			StreamCurrentBlock();
		}
	};

	class AudioPlayer: public IAudioSupervisor, OS::IThreadJob
	{
		IInstallation& installation;
		IOSAudioAPI& osAPI;

		AutoFree<OS::IThreadSupervisor> thread;
		const AudioConfig config;
		bool playMusic = false;

		// Mp3 to Stereo 44.1kHz 16-bit per channel decoder
		AutoFree<IAudioDecoder> mp3musicStereoDecoder;

		AutoFree<IAudioStreamerSupervisor> musicStreamer;
	public:
		AudioPlayer(IInstallation& refInstallation, IOSAudioAPI& ref_osAPI, const AudioConfig& refConfig): installation(refInstallation), osAPI(ref_osAPI), config(refConfig)
		{
			static_assert(sizeof(StereoSample_INT16) == 4);

			enum { SAMPLES_PER_BLOCK = 4096 };
			mp3musicStereoDecoder = Audio::CreateAudioDecoder_MP3_to_Stereo_16bit_int(SAMPLES_PER_BLOCK);

			musicStreamer = new Stereo_Streamer(osAPI, *mp3musicStereoDecoder);

			thread = OS::CreateRococoThread(this, 0);
			thread->Resume();
		}

		~AudioPlayer()
		{
			thread = nullptr;
		}

		void Free() override
		{
			delete this;
		}

		void SetMP3Music(const fstring& mp3pingPath)
		{
			if (mp3pingPath.length == 0)
			{
				Throw(0, "%s blank filename", __FUNCTION__);
			}

			if (!EndsWith(mp3pingPath, ".mp3"))
			{
				Throw(0, "%s: filename '%s' must end with .mp3", __FUNCTION__, mp3pingPath.buffer);
			}

			int err;
			cstr msg = thread->GetErrorMessage(err);
			if (msg)
			{
				char osErr[256];
				osAPI.TranslateErrorCode(err, osErr, sizeof osErr);
				Throw(err, "%s: Thread error: %s.\n%s", __FUNCTION__, msg, osErr);
			}

			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(mp3pingPath, sysPath);
			mp3musicStereoDecoder->StreamInputFile(sysPath);

			playMusic = true;
		}

		uint32 RunThread(OS::IThreadControl& tc) override
		{
			tc.SetRealTimePriority();

			try
			{
				return RunThreadProtected(tc);
			}
			catch (IException&)
			{
				throw;
			}
		}

		uint32 RunThreadProtected(OS::IThreadControl& tc)
		{
			musicStreamer->Start();
			musicStreamer->StreamCurrentBlock();
		
			while (tc.IsRunning())
			{
				tc.SleepUntilAysncEvent(1000);
			}

			musicStreamer->Stop();

			return 0;
		}
	};
}

namespace Rococo::Audio
{
	IAudioSupervisor* CreateAudioSupervisor(IInstallation& installation, IOSAudioAPI& osAPI, const AudioConfig& config)
	{
		return new AudioPlayer(installation, osAPI, config);
	}
}