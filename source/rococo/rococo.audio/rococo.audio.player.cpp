#include <rococo.audio.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.strings.h>
#include <rococo.debugging.h>

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Audio;
using namespace Rococo::Strings;

namespace AudioAnon
{
	class AudioPlayer: public IAudioSupervisor, OS::IThreadJob, IAudioSampleEvents
	{
		IAudioInstallationSupervisor& installation;
		IOSAudioAPI& osAPI;

		AutoFree<OS::IThreadSupervisor> thread;
		const AudioConfig config;

		// Mp3 to Stereo 44.1kHz 16-bit per channel decoder
		AutoFree<IAudioDecoder> mp3musicStereoDecoder;

		AutoFree<IAudioStreamerSupervisor> musicStreamer;

		OS::IdThread managementThreadId;

		AutoFree<IAudio3DSupervisor> audio3D;
		AutoFree<IConcert3DSupervisor> concert;
		AutoFree<IAudioSampleDatabaseSupervisor> monoSamples;
	public:
		AudioPlayer(IAudioInstallationSupervisor& _installation, IOSAudioAPI& ref_osAPI, const AudioConfig& refConfig): installation(_installation), osAPI(ref_osAPI), config(refConfig)
		{
			static_assert(sizeof(StereoSample_INT16) == 4);

			managementThreadId = OS::GetCurrentThreadIdentifier();

			enum { SAMPLES_PER_BLOCK = 4096 };
			mp3musicStereoDecoder = Audio::CreateAudioDecoder_MP3_to_Stereo_16bit_int(_installation, SAMPLES_PER_BLOCK);

			musicStreamer = CreateStereoStreamer(osAPI, *mp3musicStereoDecoder);

			float speedOfSoundMetresPerSecond = 343.0f;
			audio3D = osAPI.Create3DAPI(speedOfSoundMetresPerSecond);

			monoSamples = CreateAudioSampleDatabase(_installation, 1, *this);
			concert = CreateConcert(*monoSamples, ref_osAPI, *audio3D);
			
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

		void ThrowOnThreadError() override
		{
			concert->ThrowOnThreadError();
		}

		// Implementation of IAudioSampleDatabaseEvents::OnSampleLoaded
		void OnSampleLoaded(IAudioSample& sampleLoaded) override
		{
			concert->OnSampleLoaded(sampleLoaded);
		}

		// Implementation of IAudioSampleDatabaseEvents::MarkBadSample
		void MarkBadSample(IAudioSample& badSample, cstr reason) override
		{
			char msg[512];
			SafeFormat(msg, "Bad audio file: %s (%s)", badSample.Name(), reason);
			Rococo::Debugging::Log("%s", msg);
			OS::AddThreadError(0, msg);
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

			mp3musicStereoDecoder->StreamInputFile(mp3pingPath);
		}

		IdSample Bind3DSample(const fstring& mp3fxPingPath) override
		{
			auto& sample = monoSamples->Bind(mp3fxPingPath);
			return sample.Id();
		}

		IdInstrument Play3DSound(IdSample id, const Rococo::Audio::AudioSource3D& source, int32 forceLevel) override
		{
			if (forceLevel <= 0)
			{
				return concert->AssignFreeInstrument(id, source);
			}
			else if (forceLevel == 1)
			{
				return concert->AssignInstrumentByPriority(id, source);
			}
			else // > 1
			{
				return concert->AssignInstrumentAlways(id, source);
			}
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

	struct RococoAudioInstallation : IAudioInstallationSupervisor
	{
		IInstallation& installation;

		RococoAudioInstallation(IInstallation& _installation): installation(_installation)
		{

		}

		void NormalizePath(cstr pingPath, U8FilePath& expandedPingPath) override
		{
			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(pingPath, sysPath);
			installation.ConvertSysPathToPingPath(sysPath, expandedPingPath);
		}

		void LoadResource(cstr utf8Path, ILoadEventsCallback& onLoaded) override
		{	
			installation.LoadResource(utf8Path, onLoaded);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Audio
{
	ROCOCO_AUDIO_API IAudioInstallationSupervisor* CreateAudioInstallation(IInstallation& installation)
	{
		return new AudioAnon::RococoAudioInstallation(installation);
	}

	ROCOCO_AUDIO_API IAudioSupervisor* CreateAudioSupervisor(IAudioInstallationSupervisor& installation, IOSAudioAPI& osAPI, const AudioConfig& config)
	{
		return new AudioAnon::AudioPlayer(installation, osAPI, config);
	}
}