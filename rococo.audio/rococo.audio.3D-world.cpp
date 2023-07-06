#include <rococo.audio.h>
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.os.h>
#include <array>
#include <atomic>
#include <vector>

using namespace Rococo;
using namespace Rococo::Audio;
using namespace Rococo::Strings;

namespace Rococo::Audio
{
	enum class InstrumentFlag: int32
	{
		None = 0,
		FreeAtEnd = 0x1,
		FreeIfSilent = 0x2,
		Loop = 0x4,
	};

	struct InstrumentDescriptor: IAudioVoiceContext
	{
		enum { SAMPLES_PER_BLOCK = 176 }; // This gives a block size of just under 4ms
		enum { BEST_SAMPLE_RATE = 44100, PCM_BLOCK_COUNT = 4 };

		IdInstrumentIndexType index = 0;
		IdInstrumentSaltType salt = 0;
		IAudioSample* sample = nullptr;
		IOSAudioVoiceSupervisor* monoVoice = nullptr;
		int32 flags = 0; // Combination of Rococo::Audio::InstrumentFlag enumeration values
		int priority = 0; // The higher the priority the more urgent to preserve the instrument
		float volume = 1.0f; // Amplitude
		Vec3 dopplerVelocity = { 0,0,0 };
		Vec3 worldPosition = { 0.0f, 0.0f, 0.0f };
		uint32 cursor = 0;
		std::atomic<uint32> playCount;
		std::atomic<uint32> stopCount;
		std::array<int16*, PCM_BLOCK_COUNT> pcm_blocks;
		size_t currentIndex = 0;
		bool isSampleQueued = false;
		std::atomic<bool> waitingForLoad = false;
		Seconds delayRemaining = 0.0_seconds;
		OS::ticks assignmentStart = 0;

		InstrumentDescriptor() : playCount(0), stopCount(0)
		{
			size_t nBytesPerBlock = SAMPLES_PER_BLOCK * sizeof(int16);
			for (auto& block : pcm_blocks)
			{
				block = (int16*) AudioAlignedAlloc(nBytesPerBlock, 64);
				memset(block, 0, nBytesPerBlock);
			}
		}

		virtual ~InstrumentDescriptor()
		{
			for (auto* block: pcm_blocks)
			{
				AudioAlignedFree(block);
			}
		}
		
		void Assign(IAudioSample* sample, const AudioSource3D& source, OS::IThreadControl& thread)
		{
			this->sample = sample;
			this->priority = priority;
			this->flags = 0;
			this->volume = source.volume;
			this->dopplerVelocity = source.dopplerVelocity;
			this->worldPosition = source.position;
			this->isSampleQueued = false;
			this->playCount = 0;
			this->stopCount = 0;
			this->waitingForLoad = !sample->IsLoaded();
			this->assignmentStart = OS::CpuTicks();

			if (!this->waitingForLoad)
			{
				SendPlayMessageOnThread(thread);
			}
		}

		// this method is only invoked from the concert thread via APCs
		void Play()
		{
			if (isSampleQueued)
			{
				// Already queued
				return;
			}

			monoVoice->StartPulling();

			StreamCurrentBlock();
		}

		void SendPlayMessageOnThread(OS::IThreadControl& thread)
		{
			struct Anon
			{
				static void OnPlayMessage(InstrumentDescriptor* instrument)
				{
					instrument->stopCount = 0;
					instrument->Play();
					instrument->playCount = 0;
				}
			};
			thread.QueueAPC((OS::IThreadControl::FN_APC)Anon::OnPlayMessage, this);
		}

		void Stop()
		{
			this->stopCount++;
			this->playCount = 0;
		}

		void StreamCurrentBlock()
		{
			isSampleQueued = false;

			if (stopCount > 0)
			{
				monoVoice->Stop();
				stopCount = 0;
				return;
			}

			uint32 nSamples = SAMPLES_PER_BLOCK;

			int16* sampleBuffer = pcm_blocks[currentIndex];

			currentIndex = (currentIndex + 1) % PCM_BLOCK_COUNT;

			auto* sampleThisTick = this->sample;
			if (!sampleThisTick || !sampleThisTick->IsLoaded())
			{
				return;
			}
			else
			{
				waitingForLoad = false;

				nSamples = sampleThisTick->ReadSamples(sampleBuffer, SAMPLES_PER_BLOCK, cursor);
				cursor += nSamples;

				if (nSamples == 0)
				{
					if (HasFlag(InstrumentFlag::Loop, flags))
					{
						nSamples = sampleThisTick->ReadSamples(sampleBuffer, SAMPLES_PER_BLOCK, cursor);
						if (nSamples == 0)
						{
							return;
						}
					}
					else
					{
						return;
					}
				}
			}
			
			monoVoice->QueueSample((uint8*)sampleBuffer, SAMPLES_PER_BLOCK * sizeof(int16), 0, nSamples);
			isSampleQueued = true;
		}
	};

	// Private methods in this class indicate that they are run inside the concert thread
	class Concert3D : public IConcert3DSupervisor, public IConcertGoer, private IOSAudioVoiceCompletionHandler, private OS::IThreadJob
	{
		enum { MAX_VOICES = 255 };
		IAudioSampleDatabase& sampleDatabase;
		IOSAudioAPI& audio;
		Matrix4x4 worldToGoer;

		std::vector<IdInstrument> freeIds;
		std::array<InstrumentDescriptor, MAX_VOICES> instruments;
		IdInstrumentSaltType nextSalt = 1;
		AutoFree<OS::IThreadSupervisor> thread;
		uint32 maxVoices;
	public:
		Concert3D(IAudioSampleDatabase& refSampleDatabase, IOSAudioAPI& refAudio): 
			audio(refAudio),
			sampleDatabase(refSampleDatabase),
			worldToGoer(Matrix4x4::Identity()),
			maxVoices(MAX_VOICES)
		{
			freeIds.reserve(MAX_VOICES);
			InitVectors();		

			thread = OS::CreateRococoThread(this, 0);
			thread->Resume();
		}

		virtual ~Concert3D()
		{
			for (auto& i : instruments)
			{
				Rococo::Free(i.monoVoice);
				i.monoVoice = nullptr;
			}
		}

		void ThrowOnThreadError() override
		{
			int32 dbErrorCode;
			cstr dbError = sampleDatabase.GetLastError(dbErrorCode);
			if (dbError) Throw(dbErrorCode, "The audio sample database thread crashed with exception:\n%s", dbError);

			int32 concertErrorCode;
			cstr concertError = thread->GetErrorMessage(concertErrorCode);
			if (concertError)
			{
				Throw(concertErrorCode, "The Concert3D thread crashed with exception:\n%s", concertError);
			}
		}

		void InitVectors()
		{
			freeIds.clear();	

			IdInstrumentIndexType counter = 1;
			for (auto& i : instruments)
			{
				i.index = counter++;
				i.salt = nextSalt;
				i.priority = 0x00000000;
				i.dopplerVelocity = Vec3{ 0,0,0 };
				if (!i.monoVoice) i.monoVoice = audio.Create16bitMono44100kHzVoice(*this, i);
			}

			for (auto i = instruments.begin(); i != instruments.end() && i->index <= maxVoices; i++)
			{
				freeIds.push_back(IdInstrument(i->index, i->salt));
			}

			std::reverse(freeIds.begin(), freeIds.end());
		}

		void SetMaxVoices(uint32 nVoices)
		{
			enum { HARD_LIMIT = 4095 };
			if (nVoices > HARD_LIMIT)
			{
				Throw(0, "%s. There is a hard maximum of 4095 voices", __FUNCTION__);
			}

			size_t maximumRepresentableIds = 1 << sizeof(IdInstrument) << 3;

			if (nVoices > maximumRepresentableIds - 1)
			{
				Throw(0, "%s. There were more voices specified than can be represented with an IdInstrument", __FUNCTION__);
			}

			this->maxVoices = (int32) nVoices;

			InitVectors();
		}

		InstrumentDescriptor& GetRef(IdInstrument goodId)
		{
			IdInstrumentIndexType index = goodId.Index() - 1;
			if (index >= instruments.size())
			{
				Throw(0, "%s: Bad index. Not a good id!", __FUNCTION__);
			}
			
			auto& i = instruments[index];
			if (i.salt != goodId.Salt())
			{
				Throw(0, "%s: salt mismatch. Not a good id!", __FUNCTION__);
			}

			return i;
		}

		InstrumentDescriptor* TryGetRef(IdInstrument id)
		{
			if (!id) return nullptr;

			IdInstrumentIndexType index = id.Index() - 1;
			if (index >= instruments.size())
			{
				return nullptr;
			}

			auto& i = instruments[index];
			return i.salt == id.Salt() ? &i : nullptr;
		}

		IdInstrument AssignFreeInstrument(IdSample sampleId, const Rococo::Audio::AudioSource3D& source) override
		{
			IAudioSample* sample = sampleDatabase.Find(sampleId);

			if (!sample)
			{
				Throw(0, "AssignFreeInstrument([sampleId=%lld], %d]). bad sampleId", sampleId.value, source.priority);
			}

			if (source.priority <= 0)
			{
				Throw(0, "AssignFreeInstrument([id=%lld], [priority=%d]). priority must be > 0", sampleId.value, source.priority);
			}

			if (freeIds.empty())
			{
				return IdInstrument::None();
			}

			auto id = freeIds.back();
			freeIds.pop_back();

			auto& i = GetRef(id);
			i.salt = id.Salt();

			i.Assign(sample, source, *thread);

			return id;
		}

		IdInstrument GetInstrumentWithLeastPriority(int& leastPriority) const
		{
			auto& i0 = instruments[0];

			IdInstrument id = IdInstrument(i0.index, i0.salt);
			leastPriority = i0.priority;

			for (auto& i : instruments)
			{
				if (i.priority < leastPriority)
				{
					leastPriority = i.priority;
					id = IdInstrument(i.index, i.salt);
				}
			}

			return id;
		}

		IdInstrument AssignInstrumentByPriority(IdSample sampleId, const Rococo::Audio::AudioSource3D& source) override
		{
			IAudioSample* sample = sampleDatabase.Find(sampleId);

			if (!sample)
			{
				Throw(0, "AssignInstrumentByPriority([sampleId=%lld], %d]). bad sampleId", sampleId.value, source.priority);
			}

			if (source.priority <= 0)
			{
				Throw(0, "AssignInstrumentByPriority([id=%lld], [priority=%d]). priority must be > 0", sampleId.value, source.priority);
			}

			auto id = AssignFreeInstrument(sampleId, source);
			if (id) return id;

			int leastPriority;
			IdInstrument idLeastPriority = GetInstrumentWithLeastPriority(OUT leastPriority);
			if (source.priority >= leastPriority)
			{
				auto& i = GetRef(idLeastPriority);
				i.salt++;

				if (i.salt > nextSalt)
				{
					nextSalt = i.salt + 1;
				}

				i.Assign(sample, source, *thread);

				return IdInstrument(i.index, i.salt);
			}

			return IdInstrument::None();
		}

		IdInstrument AssignInstrumentAlways(IdSample sampleId, const Rococo::Audio::AudioSource3D& source)
		{
			IAudioSample* sample = sampleDatabase.Find(sampleId);

			if (!sample)
			{
				Throw(0, "AssignInstrumentAlways([sampleId=%lld], %d]). bad sampleId", sampleId.value, source.priority);
			}

			if (source.priority <= 0)
			{
				Throw(0, "AssignInstrumentAlways([id=%lld], [priority=%d]). priority must be > 0", sampleId.value, source.priority);
			}

			IdInstrument id = AssignInstrumentByPriority(sampleId, source);
			if (id) return id;

			int leastPriority;
			IdInstrument idLeastPriority = GetInstrumentWithLeastPriority(OUT leastPriority);

			auto& i = GetRef(idLeastPriority);
			i.salt++;

			if (i.salt > nextSalt)
			{
				nextSalt = i.salt + 1;
			}

			i.Assign(sample, source, *thread);

			return IdInstrument(i.index, i.salt);
		}

		// Frees the instrument slot when the current sample has completed
		bool FreeAtEnd(IdInstrument id) override
		{
			auto* i = TryGetRef(id);
			if (!i)
			{
				return false;
			}
			else
			{
				i->flags |= (int32) InstrumentFlag::FreeAtEnd;
				return true;
			}
		}

		// Frees the instrument if the current output is silent
		bool FreeIfSilent(IdInstrument id) override
		{
			auto* i = TryGetRef(id);
			if (!i)
			{
				return false;
			}
			else
			{
				i->flags |= (int32)InstrumentFlag::FreeIfSilent;
				return true;
			}
		}

		// Retrieves the friendly name/path supplied at assignment
		bool GetName(IdInstrument id, U8FilePath& name) override
		{
			auto* i = TryGetRef(id);
			if (!i || !i->sample)
			{
				name.buf[0] = 0;
				return false;
			}
			else
			{
				CopyString(name.buf, U8FilePath::CAPACITY, i->sample->Name());
				return true;
			}
		}

		bool GetPriority(IdInstrument id, OUT int& priority) override
		{
			auto* i = TryGetRef(id);
			if (!i)
			{
				priority = 0;
				return false;
			}
			else
			{
				priority = i->priority;
				return true;
			}
		}

		bool Play(IdInstrument id) override
		{
			auto* i = TryGetRef(id);
			if (!i)
			{
				return false;
			}
			else
			{
				i->stopCount = 0;
				i->SendPlayMessageOnThread(*thread);
				return true;
			}
		}

		bool Pause(IdInstrument id) override
		{
			auto* i = TryGetRef(id);
			if (!i)
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		bool Stop(IdInstrument id, bool andFree) override
		{
			auto* i = TryGetRef(id);
			if (!i)
			{
				return false;
			}
			else
			{
				i->Stop();
				return true;
			}
		}

		bool SetSample(IdInstrument id, IdSample sampleId) override
		{
			auto* sample = sampleDatabase.Find(sampleId);

			auto* i = TryGetRef(id);
			if (!i)
			{
				return false;
			}
			else
			{
				i->sample = sample;
				return true;
			}
		}

		// Sets the volume of the source material
		bool SetVolume(IdInstrument id, float volume) override
		{
			auto* i = TryGetRef(id);
			if (!i)
			{
				return false;
			}
			else
			{
				i->volume = volume;
				return true;
			}
		}

		// Sets the doppler velocity of the source instrument's sound. Note that the position of the instrument itself does not update over time 
		bool SetDopplerVelocity(IdInstrument id, cr_vec3 velocity) override
		{
			auto* i = TryGetRef(id);
			if (!i)
			{
				return false;
			}
			else
			{
				i->dopplerVelocity = velocity;
				return true;
			}
		}

		// Sets the location of the source instrument in world space
		bool SetPosition(IdInstrument id, cr_vec3 position)
		{
			auto* i = TryGetRef(id);
			if (!i)
			{
				
				return false;
			}
			else
			{
				i->worldPosition = position;
				return true;
			}
		}

		IConcertGoer& Goer()
		{
			return *this;
		}

		void SetOrientation(cr_m4x4 worldToGoer) override
		{
			this->worldToGoer = worldToGoer;
		}

		void Free() override
		{
			delete this;
		}

	private:
		void OnSampleComplete(IOSAudioVoice& voice, IAudioVoiceContext& context) override
		{
			auto& i = static_cast<InstrumentDescriptor&>(context);
			i.StreamCurrentBlock();
		}

		void OnGrossChange()
		{
			for (auto& i : instruments)
			{
				if (i.playCount > 0)
				{
					i.playCount = 0;
					i.Play();
				}
			}
		}

		void SendGrossChangeMessageToThread()
		{
			struct ANON
			{
				static void OnGrossChangeMessage(Concert3D* This)
				{
					This->OnGrossChange();
				}
			};
			thread->QueueAPC((OS::IThreadControl::FN_APC)ANON::OnGrossChangeMessage, this);
		}

		void OnSampleLoaded(IAudioSample& sample) override
		{
			int nChanges = 0;
			// Generally samples will not be reloaded too frequently, so the cost of enumerating all voices here should not be significant
			for (auto& i : instruments)
			{
				if (i.sample == &sample && i.waitingForLoad)
				{
					nChanges++;
					// We increment playCount to flag that we want to play the sample, but we do not do significant play logic here as we are in a thread foreign to our knowledge of the internals of this class
					i.playCount++;
				}
			}

			if (nChanges > 0) SendGrossChangeMessageToThread();
		}

		uint32 RunThread(OS::IThreadControl& tc) override
		{
			tc.SetRealTimePriority();

			while (tc.IsRunning())
			{
				tc.SleepUntilAysncEvent(1000);
			}

			return 0;
		}
	};
}

namespace Rococo::Audio
{
	ROCOCO_AUDIO_API IConcert3DSupervisor* CreateConcert(IAudioSampleDatabase& database, IOSAudioAPI& audio)
	{
		if (database.NumberOfChannels() != 1)
		{
			Throw(0, "%s: database channel count must be 1, i.e a mono sample database", __FUNCTION__);
		}
		return new Concert3D(database, audio);
	}
}