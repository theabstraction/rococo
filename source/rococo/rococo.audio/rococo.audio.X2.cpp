#include <rococo.audio.h>
#include <rococo.os.h>
#include <rococo.strings.h>
#include <rococo.xaudio2.h>
#include <rococo.time.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::Audio;
using namespace Rococo::Strings;

namespace Rococo::Audio
{
	IAudio3DSupervisor* CreateX3D(float speedOfSoundInMetresPerSecond, IXAudio2MasteringVoice& master);
}

namespace AudioAnon
{
	cstr GetX2Err(HRESULT x2err)
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
			s->left = 0x0000FFFF & rand();
			s->right = 0x0000FFFF & rand();
		}
	}

	struct X2AudioVoice : IOSAudioVoiceSupervisor, IXAudio2VoiceCallback
	{
		IAudioVoiceContext& context;
		AutoVoice<IXAudio2SourceVoice> sourceVoice;
		IOSAudioVoiceCompletionHandler& completionHandler;
		IXAudio2MasteringVoice& masterVoice;

		HRESULT lastError = S_FALSE;

		X2AudioVoice(IXAudio2& x2, IXAudio2MasteringVoice& _masterVoice, const WAVEFORMATEX* format, IOSAudioVoiceCompletionHandler& refCompletionHandler, IAudioVoiceContext& refContext):
			context(refContext),
			completionHandler(refCompletionHandler),
			masterVoice(_masterVoice)
		{
			HRESULT hr;
			VALIDATE(hr, hr = x2.CreateSourceVoice(&sourceVoice.src, (const WAVEFORMATEX*) format, 0, 2.0f, this));
		}

		static X2AudioVoice* Create16bitMono44100kHzVoice(IXAudio2& x2, IXAudio2MasteringVoice& masterVoice, IOSAudioVoiceCompletionHandler& completionHandler, IAudioVoiceContext& context)
		{
			PCMWAVEFORMAT srcFormat;
			srcFormat.wBitsPerSample = 16;
			srcFormat.wf.nSamplesPerSec = 44100;
			srcFormat.wf.nChannels = 1;
			srcFormat.wf.nBlockAlign = sizeof(MonoSample_INT16);
			srcFormat.wf.nAvgBytesPerSec = srcFormat.wf.nBlockAlign * srcFormat.wf.nSamplesPerSec;
			srcFormat.wf.wFormatTag = WAVE_FORMAT_PCM;

			return new X2AudioVoice(x2, masterVoice,(const WAVEFORMATEX*)&srcFormat, completionHandler, context);
		}

		static X2AudioVoice* Create16bitStereo44100kHzVoice(IXAudio2& x2, IXAudio2MasteringVoice& masterVoice, IOSAudioVoiceCompletionHandler& completionHandler, IAudioVoiceContext& context)
		{
			PCMWAVEFORMAT srcFormat;
			srcFormat.wBitsPerSample = 16;
			srcFormat.wf.nSamplesPerSec = 44100;
			srcFormat.wf.nChannels = 2;
			srcFormat.wf.nBlockAlign = sizeof(StereoSample_INT16);
			srcFormat.wf.nAvgBytesPerSec = srcFormat.wf.nBlockAlign * srcFormat.wf.nSamplesPerSec;
			srcFormat.wf.wFormatTag = WAVE_FORMAT_PCM;

			return new X2AudioVoice(x2, masterVoice, (const WAVEFORMATEX*)&srcFormat, completionHandler, context);
		}

		void QueueSample(const uint8* buffer, uint32 nBytesInBuffer, uint32 beginAt, uint32 nSamplesToPlay) override
		{
			XAUDIO2_BUFFER x2buffer = { 0 };
			x2buffer.AudioBytes = nBytesInBuffer;
			x2buffer.pAudioData = buffer;
			x2buffer.PlayBegin = beginAt;
			x2buffer.PlayLength = nSamplesToPlay;

			HRESULT hr;
			VALIDATE(hr, hr = sourceVoice->SubmitSourceBuffer(&x2buffer));
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

		void Set3DParameters(const EmitterDSP& dsp) override
		{
			sourceVoice->SetOutputMatrix(&masterVoice, 1, 2, dsp.pMatrixCoefficients->all.speakers, 0);
			sourceVoice->SetFrequencyRatio(dsp.DopplerFactor);
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
			completionHandler.OnSampleComplete(*this, context);
		};

		STDMETHOD_(void, OnLoopEnd)(void*) override
		{
		};

		STDMETHOD_(void, OnVoiceError)(void*, HRESULT hr) override
		{
			lastError = hr;
		};
	};

	struct X2Audio : IOSAudioAPISupervisor
	{
		AutoRelease<IXAudio2> x2;
		AutoVoice<IXAudio2MasteringVoice> masterVoice;

		X2Audio()
		{
			HRESULT hr;
			VALIDATE(hr, hr = XAudio2Create(&x2, 0, XAUDIO2_DEFAULT_PROCESSOR));
			VALIDATE(hr, hr = x2->CreateMasteringVoice(&masterVoice, 2, 44100));
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

		IOSAudioVoiceSupervisor* Create16bitMono44100kHzVoice(IOSAudioVoiceCompletionHandler& completionHandler, IAudioVoiceContext& context) override
		{
			return X2AudioVoice::Create16bitMono44100kHzVoice(*x2, *masterVoice, completionHandler, context);
		}

		IOSAudioVoiceSupervisor* Create16bitStereo44100kHzVoice(IOSAudioVoiceCompletionHandler& completionHandler, IAudioVoiceContext& context) override
		{
			return X2AudioVoice::Create16bitStereo44100kHzVoice(*x2, *masterVoice, completionHandler, context);
		}

		IAudio3DSupervisor* Create3DAPI(float speedOfSoundInMetresPerSecond) override
		{
			return CreateX3D(speedOfSoundInMetresPerSecond, *masterVoice);
		}
	};
}

namespace Rococo::Audio
{
	ROCOCO_AUDIO_API IOSAudioAPISupervisor* CreateOSAudio()
	{
		return new AudioAnon::X2Audio();
	}
}
