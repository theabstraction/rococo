#include <rococo.audio.h>
#include <rococo.strings.h>
#include <rococo.maths.h>
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
	};

	struct InstrumentDescriptor
	{
		IdInstrumentIndexType index = 0;
		IdInstrumentSaltType salt = 0;
		IAudioSample* sample = nullptr;
		int32 flags = 0; // Combination of Rococo::Audio::InstrumentFlag enumeration values
		int priority = 0; // The higher the priority the more urgent to preserve the instrument
		float volume = 1.0f; // Amplitude
		Vec3 dopplerVelocity = { 0,0,0 };
		Vec3 worldPosition = { 0.0f, 0.0f, 0.0f };
		
		void Assign(IAudioSample* sample, const AudioSource3D& source)
		{
			this->sample = sample;
			this->priority = priority;
			this->flags = 0;
			this->volume = source.volume;
			this->dopplerVelocity = source.dopplerVelocity;
			this->worldPosition = source.position;
		}
	};

	struct Concert3D : IConcert3DSupervisor, IConcertGoer
	{
		IAudioSampleDatabase& sampleDatabase;
		Matrix4x4 worldToGoer;
		uint32 maxVoices;
		std::vector<IdInstrument> freeIds;
		std::vector<InstrumentDescriptor> instruments;
		IdInstrumentSaltType nextSalt = 1;

		Concert3D(IAudioSampleDatabase& refSampleDatabase): 
			sampleDatabase(refSampleDatabase),
			worldToGoer(Matrix4x4::Identity()), maxVoices(255)
		{
			instruments.resize(maxVoices);
			freeIds.reserve(maxVoices);
			InitVectors();		
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
			}

			for (auto i = instruments.rbegin(); i != instruments.rend(); i++)
			{
				freeIds.push_back(IdInstrument(i->index, i->salt));
			}
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

			i.Assign(sample, source);

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

				i.Assign(sample, source);

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

			i.Assign(sample, source);

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
	};
}

namespace Rococo::Audio
{
	ROCOCO_AUDIO_API IConcert3DSupervisor* CreateConcert(IAudioSampleDatabase& database)
	{
		if (database.NumberOfChannels() != 1)
		{
			Throw(0, "%s: database channel count must be 1, i.e a mono sample database", __FUNCTION__);
		}
		return new Concert3D(database);
	}
}