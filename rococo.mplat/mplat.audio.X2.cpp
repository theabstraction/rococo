#include <rococo.mplat.h>
#include <vector>

#include <rococo.strings.h>
#include <mplat.audio.h>
#include <mplat.release.h>

#include <xaudio2.h>

#define VALIDATE(x)\
{ \
	if FAILED(x) \
		Throw(hr, "%s: %s", __FUNCTION__, #x); \
}

using namespace Rococo;
using namespace Rococo::Audio;

namespace
{
	cstr GetX2Err(int x2err)
	{
		switch (x2err)
		{
		case XAUDIO2_E_INVALID_CALL: return "XAUDIO2_E_INVALID_CALL";
		case XAUDIO2_E_XMA_DECODER_ERROR: return "XAUDIO2_E_XMA_DECODER_ERROR";
		case XAUDIO2_E_XAPO_CREATION_FAILED: return "XAUDIO2_E_XAPO_CREATION_FAILED";
		case XAUDIO2_E_DEVICE_INVALIDATED: return "XAUDIO2_E_DEVICE_INVALIDATED";
		default: return "";
		}
	}

	void SetWhiteNoise(I16StereoSample* samples, size_t capacity)
	{
		auto* end = samples + capacity;
		for (auto* s = samples; s < end; ++s)
		{
			s->left = rand();
			s->right = rand();
		}
	}

	template<class T>
	struct AutoVoice
	{
		T* src;

		AutoVoice(): src(nullptr)
		{

		}

		~AutoVoice()
		{
			if (src)
			{
				src->DestroyVoice();
			}
		}

		T** operator& ()
		{
			return &src;
		}

		T* operator ->()
		{
			return src;
		}
	};

	struct AudioPlayer : IAudioSupervisor, OS::IThreadJob
	{
		OS::IThreadSupervisor* thread = nullptr;

		IInstallation& installation;
		const AudioConfig config;

		AutoRelease<IXAudio2> x2;
		AutoVoice<IXAudio2MasteringVoice> masterVoice;
		AutoVoice<IXAudio2SourceVoice> sourceVoice;
		
		AutoFree<IAudioDecoder> decoder;

		bool playMusic = false;

		std::vector<I16StereoSample*> pcm_blocks;

		volatile size_t currentIndex = 0;

		enum { SAMPLES_PER_BLOCK = 4096, BEST_SAMPLE_RATE = 44100, PCM_BLOCK_COUNT = 4 };

		struct StreamingVoiceCallback : public IXAudio2VoiceCallback
		{
			AudioPlayer& player;

			STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32) override
			{
			};

			STDMETHOD_(void, OnVoiceProcessingPassEnd)() override
			{
			};

			STDMETHOD_(void, OnStreamEnd)() override
			{
			};

			STDMETHOD_(void, OnBufferStart)(void*) override
			{
			};

			STDMETHOD_(void, OnBufferEnd)(void*) override
			{
				player.QueueNextBuffer_OnAudioThread();
			};

			STDMETHOD_(void, OnLoopEnd)(void*) override
			{
			};

			STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override
			{
			};

			StreamingVoiceCallback(AudioPlayer& refPlayer):
				player(refPlayer)
			{

			}

			virtual ~StreamingVoiceCallback()
			{

			}
		} srcCallbacks;

		void AudioThreadPopulate(I16StereoSample* samples, size_t capacity)
		{
			if (playMusic)
			{
				STREAM_STATE state;
				decoder->GetOutput(samples, (uint32)capacity, OUT state);
			}
		}

		AudioPlayer(IInstallation& ref_installation, const AudioConfig& cr_config) :
			installation(ref_installation), config(cr_config), srcCallbacks(*this)
		{
			static_assert(sizeof(I16StereoSample) == 4);

			HRESULT hr;
			VALIDATE(hr = XAudio2Create(&x2, 0, XAUDIO2_DEFAULT_PROCESSOR));		
			VALIDATE(hr = x2->CreateMasteringVoice(&masterVoice, 2, BEST_SAMPLE_RATE));

			PCMWAVEFORMAT srcFormat;
			srcFormat.wBitsPerSample = 16;
			srcFormat.wf.nSamplesPerSec = BEST_SAMPLE_RATE;
			srcFormat.wf.nChannels = 2;
			srcFormat.wf.nBlockAlign = sizeof(I16StereoSample);
			srcFormat.wf.nAvgBytesPerSec = srcFormat.wf.nBlockAlign * srcFormat.wf.nSamplesPerSec;
			srcFormat.wf.wFormatTag = WAVE_FORMAT_PCM;

			VALIDATE(hr = x2->CreateSourceVoice(&sourceVoice, (const WAVEFORMATEX*) &srcFormat,0,2.0f,&srcCallbacks));

			size_t nBytesPerBlock = SAMPLES_PER_BLOCK * sizeof(I16StereoSample);
			pcm_blocks.resize(PCM_BLOCK_COUNT);
			for (int i = 0; i < PCM_BLOCK_COUNT; ++i)
			{
				pcm_blocks[i] = (I16StereoSample*) _aligned_malloc(nBytesPerBlock, 64);
				memset(pcm_blocks[i], 0, nBytesPerBlock);
			}

			thread = OS::CreateRococoThread(this, 0);
			thread->Resume();
		}

		~AudioPlayer()
		{
			if (thread) thread->Free();

			for (int i = 0; i < PCM_BLOCK_COUNT; ++i)
			{
				_aligned_free(pcm_blocks[i]);
			}
		}

		void SetMusic(const fstring& filename) override
		{
			if (filename.length == 0)
			{
				Throw(0, "%s blank filename", __FUNCTION__);
			}

			if (!EndsWith(filename, ".mp3"))
			{
				Throw(0, "%s(%s) filename must end with .mp3", __FUNCTION__, filename.buffer);
			}

			ValidateThread();

			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(filename, sysPath);

			if (!decoder)
			{
				decoder = Audio::CreateAudioDecoder_MP3_to_Stereo_16bit_int(SAMPLES_PER_BLOCK);
			}

			decoder->StreamInputFile(sysPath);

			playMusic = true;
		}

		void ValidateThread()
		{
			int err;
			cstr msg = thread->GetErrorMessage(err);
			if (msg)
			{
				Throw(err, "XAudio2: Thread error: %s.\n%s", msg, GetX2Err(err));
			}
		}

		void Free() override
		{
			double dt_max = maxTicks / (double)OS::CpuHz();
			double dt_min = minTicks / (double)OS::CpuHz();
			delete this;
		}

		uint32 ProcessAudio_OnAudioThread(I16StereoSample* outputBuffer)
		{
			if (playMusic)
			{
				STREAM_STATE state;
				return decoder->GetOutput(outputBuffer, SAMPLES_PER_BLOCK, OUT state);
			}

			return SAMPLES_PER_BLOCK;
		}

		OS::ticks maxTicks = 0;
		OS::ticks minTicks = 0x7FFFFFFF0000FFFF;

		void QueueNextBuffer_OnAudioThread()
		{
			OS::ticks start = OS::CpuTicks();

			XAUDIO2_BUFFER x2buffer = { 0 };
			x2buffer.AudioBytes = (UINT32)(SAMPLES_PER_BLOCK * sizeof(I16StereoSample));
			x2buffer.pAudioData = (const BYTE*)pcm_blocks[currentIndex];
			x2buffer.PlayBegin = 0;
			x2buffer.PlayLength = ProcessAudio_OnAudioThread(pcm_blocks[currentIndex]);

			currentIndex = (currentIndex + 1) % PCM_BLOCK_COUNT;

			HRESULT hr;
			VALIDATE(hr = sourceVoice->SubmitSourceBuffer(&x2buffer));

			OS::ticks dt = OS::CpuTicks() - start;

			maxTicks = max(dt, maxTicks);
			minTicks = min(dt, minTicks);
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
			HRESULT hr;

			VALIDATE(hr = sourceVoice->Start());
			QueueNextBuffer_OnAudioThread();

			while (tc.IsRunning())
			{
				tc.SleepUntilAysncEvent(1000);
			}

			VALIDATE(hr = sourceVoice->Stop());

			return 0;
		}
	};
}

namespace Rococo::Audio
{
	IAudioSupervisor* CreateAudioSupervisor(IInstallation& installation, const AudioConfig& config)
	{
		return new AudioPlayer(installation, config);
	}
}