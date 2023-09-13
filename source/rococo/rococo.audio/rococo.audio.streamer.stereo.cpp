#include <rococo.audio.h>
#include <rococo.strings.h>
#include <rococo.audio.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Audio;

namespace AudioAnon
{
	struct NullContext : IAudioVoiceContext
	{

	};

	class Stereo_Streamer : public IAudioStreamerSupervisor, public IOSAudioVoiceCompletionHandler
	{
		IAudioDecoder& decoder;

		// Stereo 44.1kHz 16-bit per channel stereo voice
		AutoFree<IOSAudioVoiceSupervisor> stereoVoice;

		std::vector<StereoSample_INT16*> pcm_blocks;
		volatile size_t currentIndex = 0;

		NullContext nowt;

		enum { SAMPLES_PER_BLOCK = 4096, BEST_SAMPLE_RATE = 44100, PCM_BLOCK_COUNT = 4 };
	public:
		Stereo_Streamer(IOSAudioAPI& osAudio, IAudioDecoder& refDecoder) : decoder(refDecoder)
		{
			size_t nBytesPerBlock = SAMPLES_PER_BLOCK * sizeof(StereoSample_INT16);
			pcm_blocks.resize(PCM_BLOCK_COUNT);
			for (int i = 0; i < PCM_BLOCK_COUNT; ++i)
			{
				pcm_blocks[i] = (StereoSample_INT16*) AudioAlignedAlloc(nBytesPerBlock, 64);
				memset(pcm_blocks[i], 0, nBytesPerBlock);
			}

			// Stereo 44.1kHz 16-bit per channel stereo voice
			stereoVoice = osAudio.Create16bitVoice(44100, 2, *this, nowt);
		}

		~Stereo_Streamer()
		{
			for (int i = 0; i < PCM_BLOCK_COUNT; ++i)
			{
				AudioAlignedFree(pcm_blocks[i]);
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

			STREAM_STATE state;
			nSamples = decoder.GetOutput(sampleBuffer, SAMPLES_PER_BLOCK, OUT state);

			stereoVoice->QueueSample((uint8*)sampleBuffer, SAMPLES_PER_BLOCK * sizeof(StereoSample_INT16), 0, nSamples);
		}

		void OnSampleComplete(IOSAudioVoice&, IAudioVoiceContext&) override
		{
			StreamCurrentBlock();
		}
	};
}

namespace Rococo::Audio
{
	ROCOCO_AUDIO_API IAudioStreamerSupervisor* CreateStereoStreamer(IOSAudioAPI& osAudio, IAudioDecoder& refDecoder)
	{
		return new AudioAnon::Stereo_Streamer(osAudio, refDecoder);
	}
}