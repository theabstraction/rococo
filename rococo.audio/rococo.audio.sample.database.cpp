#include <rococo.audio.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>
#include <vector>
#include <list>

using namespace Rococo;
using namespace Rococo::Audio;

namespace
{
	struct AudioSample: IAudioSampleSupervisor, IPCMAudioBufferManager
	{
		HString name;
		AudioBufferDescriptor descriptor = { 0,0 };
		OS::IdThread managementThreadId = 0;

		AutoFree<IExpandingBuffer> sampleData;
		uint32 sampleLength = 0;

		AudioSample(cstr refName): name(refName), sampleData(CreateExpandingBuffer(0))
		{
		}

		cstr Name() const override
		{
			return name;
		}

		bool Accept(const AudioBufferDescriptor& descriptor) override
		{
			this->descriptor = descriptor;

			if (descriptor.bitsPerChannel == 16 && (descriptor.nChannels == 1 || descriptor.nChannels == 2))
			{
				return true;
			}

			return false;
		}

		IExpandingBuffer& PCMBuffer() override
		{
			return *sampleData;
		}

		void Cache(IMP3LoaderSupervisor& loader) override
		{
			sampleLength = loader.DecodeMP3(name, *this);
		}

		void Finalize(const PCMAudioLoadingMetrics& metrics) override
		{

		}

		void Free() override
		{
			delete this;
		}
	};

	struct AudioSampleDatabase: IAudioSampleDatabaseSupervisor, OS::IThreadJob
	{
		IInstallation& installation;

		stringmap<IAudioSampleSupervisor*> samples;
		std::list<IAudioSampleSupervisor*> uncachedSamples;

		// Here we only use one thread for  loading samples, but we could add more and have them use the same synchronization section and run the same threadjob
		// in which case the threads would each poll the uncachedSamples list and pop the front as needed
		AutoFree<OS::IThreadSupervisor> thread;

		AutoFree<OS::ICriticalSection> sync;

		OS::IdThread managementThreadId;

		int32 nChannels;

		AudioSampleDatabase(IInstallation& refInstallation, int nChannels): installation(refInstallation)
		{
			this->nChannels = nChannels;

			managementThreadId = OS::GetCurrentThreadIdentifier();

			thread = OS::CreateRococoThread(this, 0);
			sync = thread->CreateCriticalSection();
			thread->Resume();
		}

		~AudioSampleDatabase()
		{
			StopAndClean();
		}

		IAudioSample& Bind(cstr pingPath)
		{
			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(pingPath, sysPath);

			U8FilePath expandedPingPath;
			installation.ConvertSysPathToPingPath(sysPath, expandedPingPath);

			auto i = samples.find(expandedPingPath);
			if (i == samples.end())
			{
				auto* s = new AudioSample(i->first);
				i = samples.insert(expandedPingPath, s).first;

				sync->Lock();
				uncachedSamples.push_back(s);
				sync->Unlock();
			}

			return *i->second;
		}

		void StopAndClean()
		{
			if (managementThreadId != OS::GetCurrentThreadIdentifier())
			{
				Throw(0, "%s must be called on the same thread as the constructor", __FUNCTION__);
			}

			thread = nullptr;

			for (auto i : samples)
			{
				i.second->Free();
			}

			samples.clear();
			uncachedSamples.clear();
		}

		void Clear() override
		{
			StopAndClean();

			thread = OS::CreateRococoThread(this, 0);
			thread->Resume();
		}

		void Free() override
		{
			delete this;
		}

		uint32 RunThread(OS::IThreadControl& tc) override
		{
			AutoFree<IMP3LoaderSupervisor> loader(CreateMP3Loader(installation, nChannels));

			while (tc.IsRunning())
			{
				IAudioSampleSupervisor* sample = nullptr;

				do
				{
					sync->Lock();

					if (!uncachedSamples.empty())
					{
						sample = uncachedSamples.front();
						uncachedSamples.pop_front();
					}
					else
					{
						sample = nullptr;
					}
					sync->Unlock();

					sample->Cache(*loader);
				} while (tc.IsRunning() && sample != nullptr);

				tc.SleepUntilAysncEvent(1000);
			}

			return 0;
		}
	};
}

namespace Rococo::Audio
{
	ROCOCO_AUDIO_API IAudioSampleDatabaseSupervisor* CreateAudioSampleDatabase(IInstallation& installation, int nChannels)
	{
		return new AudioSampleDatabase(installation, nChannels);
	}
}