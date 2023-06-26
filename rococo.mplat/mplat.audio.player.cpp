#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <mplat.audio.h>

using namespace Rococo;
using namespace Rococo::Audio;

namespace
{
	class AudioPlayer: public IAudioSupervisor, OS::IThreadJob
	{
		IInstallation& installation;
		IOSAudioAPI& osAPI;

		AutoFree<OS::IThreadSupervisor> thread;
		const AudioConfig config;

		// Mp3 to Stereo 44.1kHz 16-bit per channel decoder
		AutoFree<IAudioDecoder> mp3musicStereoDecoder;

		AutoFree<IAudioStreamerSupervisor> musicStreamer;

		OS::IdThread managementThreadId;
	public:
		AudioPlayer(IInstallation& refInstallation, IOSAudioAPI& ref_osAPI, const AudioConfig& refConfig): installation(refInstallation), osAPI(ref_osAPI), config(refConfig)
		{
			static_assert(sizeof(StereoSample_INT16) == 4);

			managementThreadId = OS::GetCurrentThreadIdentifier();

			enum { SAMPLES_PER_BLOCK = 4096 };
			mp3musicStereoDecoder = Audio::CreateAudioDecoder_MP3_to_Stereo_16bit_int(SAMPLES_PER_BLOCK);

			musicStreamer = CreateStereoStreamer(osAPI, *mp3musicStereoDecoder);

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
		}

		uint32 RunThread(OS::IThreadControl& tc) override
		{
			tc.SetRealTimePriority();

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