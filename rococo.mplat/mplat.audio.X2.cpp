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
using namespace Rococo::Strings;


namespace
{
	cstr GetX2Err(int x2err)
	{
		switch (x2err)
		{
		case XAUDIO2_E_INVALID_CALL:		 return "XAUDIO2_E_INVALID_CALL";
		case XAUDIO2_E_XMA_DECODER_ERROR:	 return "XAUDIO2_E_XMA_DECODER_ERROR";
		case XAUDIO2_E_XAPO_CREATION_FAILED: return "XAUDIO2_E_XAPO_CREATION_FAILED";
		case XAUDIO2_E_DEVICE_INVALIDATED:	 return "XAUDIO2_E_DEVICE_INVALIDATED";
		default:							 return "";
		}
	}

	void SetWhiteNoise(StereoSample_INT16* samples, size_t capacity)
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

		AutoVoice(T* pSrc) : src(pSrc)
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

	struct X2AudioVoice : IOSAudioVoiceSupervisor, IXAudio2VoiceCallback
	{
		AutoVoice<IXAudio2SourceVoice> sourceVoice;
		IOSAudioVoiceCompletionHandler& completionHandler;

		X2AudioVoice(IXAudio2& x2, const WAVEFORMATEX* format, IOSAudioVoiceCompletionHandler& refCompletionHandler): completionHandler(refCompletionHandler)
		{
			HRESULT hr;
			VALIDATE(hr = x2.CreateSourceVoice(&sourceVoice.src, (const WAVEFORMATEX*) format, 0, 2.0f, this));
		}

		static X2AudioVoice* Create16bitStereo44100kHzVoice(IXAudio2& x2, IOSAudioVoiceCompletionHandler& completionHandler)
		{
			PCMWAVEFORMAT srcFormat;
			srcFormat.wBitsPerSample = 16;
			srcFormat.wf.nSamplesPerSec = 44100;
			srcFormat.wf.nChannels = 2;
			srcFormat.wf.nBlockAlign = sizeof(StereoSample_INT16);
			srcFormat.wf.nAvgBytesPerSec = srcFormat.wf.nBlockAlign * srcFormat.wf.nSamplesPerSec;
			srcFormat.wf.wFormatTag = WAVE_FORMAT_PCM;

			return new X2AudioVoice(x2, (const WAVEFORMATEX*)&srcFormat, completionHandler);
		}

		void QueueSample(const uint8* buffer, uint32 nBytesInBuffer, uint32 beginAt, uint32 nSamplesToPlay) override
		{
			OS::ticks start = OS::CpuTicks();

			XAUDIO2_BUFFER x2buffer = { 0 };
			x2buffer.AudioBytes = nBytesInBuffer;
			x2buffer.pAudioData = buffer;
			x2buffer.PlayBegin = 0;
			x2buffer.PlayLength = nSamplesToPlay;

			HRESULT hr;
			VALIDATE(hr = sourceVoice->SubmitSourceBuffer(&x2buffer));
		}

		void StartPulling()
		{
			sourceVoice->Start();
		}

		void Stop()
		{
			sourceVoice->Stop();
		}

		~X2AudioVoice()
		{


		}

		void Free() override
		{
			delete this;
		}

		STDMETHOD_(void, OnVoiceProcessingPassStart)(uint32) override
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
			completionHandler.OnSampleComplete(*this);
		};

		STDMETHOD_(void, OnLoopEnd)(void*) override
		{
		};

		STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override
		{
		};
	};

	struct X2Audio : IOSAudioAPISupervisor
	{
		AutoRelease<IXAudio2> x2;
		AutoVoice<IXAudio2MasteringVoice> masterVoice;

		X2Audio()
		{
			HRESULT hr;
			VALIDATE(hr = XAudio2Create(&x2, 0, XAUDIO2_DEFAULT_PROCESSOR));
			VALIDATE(hr = x2->CreateMasteringVoice(&masterVoice, 2, 44100));
		}

		void Free() override
		{
			delete this;
		}

		void TranslateErrorCode(int errCode, char* msg, size_t capacity) override
		{
			cstr err = GetX2Err(errCode);
			CopyString(msg, capacity, err);
		}

		IOSAudioVoiceSupervisor* Create16bitStereo44100kHzVoice(IOSAudioVoiceCompletionHandler& completionHandler) override
		{
			return X2AudioVoice::Create16bitStereo44100kHzVoice(*x2, completionHandler);
		}
	};
}

namespace Rococo::Audio
{
	IOSAudioAPISupervisor* CreateOSAudio()
	{
		return new X2Audio();
	}
}
