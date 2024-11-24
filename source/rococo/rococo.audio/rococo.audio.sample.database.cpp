#include <rococo.audio.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>
#include <vector>
#include <list>

using namespace Rococo;
using namespace Rococo::Audio;
using namespace Rococo::Strings;

namespace AudioAnon
{
	struct AudioSample: IAudioSampleSupervisor, IPCMAudioBufferManager
	{
		HString name;
		AudioBufferDescriptor descriptor = { 0,0 };
		OS::IdThread managementThreadId = 0;

		IdSample id;

		AutoFree<IExpandingBuffer> sampleData;
		uint32 sampleLength = 0; // in bytes

		AudioSample(IdSample argId, cstr refName): name(refName), id(argId), sampleData(CreateExpandingBuffer(0))
		{
		}

		virtual ~AudioSample()
		{
		}

		IdSample Id() const override
		{
			return id;
		}

		bool IsLoaded() const override
		{
			return sampleLength > 0;
		}

		cstr Name() const override
		{
			return name;
		}

		uint32 ReadSamples(int16* outputBuffer, uint32 outputBufferSampleCapacity, uint32 startAtSampleIndex) override
		{
			uint32 numberOfMonoSamples = sampleLength >> 1;
			if (numberOfMonoSamples <= startAtSampleIndex)
			{
				return 0;
			}

			const auto* byteData = sampleData->GetData();
			const int16* monoReprentation = reinterpret_cast<const int16*>(byteData);

			uint32 numberOfElementsInSection;

			if (outputBufferSampleCapacity + startAtSampleIndex <= numberOfMonoSamples)
			{
				numberOfElementsInSection = outputBufferSampleCapacity;
			}
			else
			{
				numberOfElementsInSection = numberOfMonoSamples - startAtSampleIndex;
			}

			// This crashed at program exit, with monoRepresentation junked.
			if (outputBuffer) memcpy(outputBuffer, monoReprentation + startAtSampleIndex, numberOfElementsInSection * sizeof int16);
			return numberOfElementsInSection;
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

		void Cache(ILoaderSupervisor& loader, IAudioSampleEvents& eventHandler) override
		{
			sampleLength = loader.DecodeAudio(name, *this, eventHandler, *this);
			if (sampleLength > 0)
			{
				eventHandler.OnSampleLoaded(*this);
			}
		}

		void Finalize(const PCMAudioLoadingMetrics&) override
		{

		}

		void Free() override
		{
			delete this;
		}
	};

	struct AudioSampleDatabase: IAudioSampleDatabaseSupervisor, OS::IThreadJob
	{
		IAudioInstallationSupervisor& installation;
		IAudioSampleEvents& eventHandler;

		stringmap<IAudioSampleSupervisor*> samples;
		std::list<IAudioSampleSupervisor*> uncachedSamples;
		std::vector<IAudioSampleSupervisor*> idToSample;

		// Here we only use one thread for  loading samples, but we could add more and have them use the same synchronization section and run the same threadjob
		// in which case the threads would each poll the uncachedSamples list and pop the front as needed
		AutoFree<OS::IThreadSupervisor> thread;

		AutoFree<OS::ICriticalSection> sync;

		OS::IdThread managementThreadId;

		int32 nChannels;

		AudioSampleDatabase(IAudioInstallationSupervisor& _installation, int nChannels, IAudioSampleEvents& _eventHandler): installation(_installation), eventHandler(_eventHandler)
		{
			this->nChannels = nChannels;

			managementThreadId = OS::GetCurrentThreadIdentifier();

			thread = OS::CreateRococoThread(this, 0);
			sync = OS::CreateCriticalSection();
			thread->Resume();
		}

		~AudioSampleDatabase()
		{
			StopAndClean();
		}

		cstr GetLastError(int& errorCode) const override
		{
			return thread->GetErrorMessage(errorCode);
		}

		IAudioSample& Bind(cstr pingPath)
		{
			U8FilePath expandedPingPath;
			installation.NormalizePath(pingPath, expandedPingPath);

			auto i = samples.find(expandedPingPath);
			if (i == samples.end())
			{
				i = samples.insert(expandedPingPath, nullptr).first;
				IdSample id(idToSample.size() + 1);
				auto* s = new AudioSample(id, i->first);
				i->second = s;
				idToSample.push_back(s);

				sync->Lock();
				uncachedSamples.push_back(s);
				sync->Unlock();
			}

			return *i->second;
		}

		IAudioSample* Find(IdSample id) const
		{
			auto index = id.value - 1;
			return index < idToSample.size() ? idToSample[index] : nullptr;
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

		uint32 NumberOfChannels() const override
		{
			return nChannels;
		}

		uint32 RunThread(OS::IThreadControl& tc) override
		{
			AutoFree<ILoaderSupervisor> mp3Loader(CreateSingleThreadedMP3Loader(installation, nChannels));
			AutoFree<ILoaderSupervisor> oggLoader(CreateSingleThreadedOggVorbisLoader(installation, nChannels));

			tc.SetRealTimePriority();

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

					if (sample)
					{
						Substring filename = Strings::Substring::ToSubstring(sample->Name());
						cstr extension = Strings::ReverseFind('.', filename);
						if (!extension)
						{
							eventHandler.MarkBadSample(*sample, "no extension");
						}
						else
						{
							if (_strcmpi(extension, ".mp3") == 0)
							{
								sample->Cache(*mp3Loader, eventHandler);
							}
							else if (_strcmpi(extension, ".ogg") == 0)
							{
								sample->Cache(*oggLoader, eventHandler);
							}
							else
							{
								eventHandler.MarkBadSample(*sample, "Unknown extension");
							}
						}
					}
				} while (tc.IsRunning() && sample != nullptr);

				tc.SleepUntilAysncEvent(200);
			}

			return 0;
		}
	};
}

namespace Rococo::Audio
{
	ROCOCO_AUDIO_API IAudioSampleDatabaseSupervisor* CreateAudioSampleDatabase(IAudioInstallationSupervisor& installation, int nChannels, IAudioSampleEvents& eventHandler)
	{
		return new AudioAnon::AudioSampleDatabase(installation, nChannels, eventHandler);
	}
}