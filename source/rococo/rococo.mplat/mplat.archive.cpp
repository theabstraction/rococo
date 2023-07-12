#include "rococo.mplat.h"
#include "rococo.hashtable.h"

using namespace Rococo;

namespace
{
	class Archive : public IArchiveSupervisor
	{
		stringmap<Vec3> vec3s;
		stringmap<float> floats;
	public:
		void LoadVec3(const fstring& key, Vec3& targetVariable, float defaultX, float defaultY, float defaultZ) override
		{
			auto i = vec3s.find(key);
			if (i == vec3s.end())
			{
				targetVariable = { defaultX, defaultY, defaultZ };
				vec3s.insert(key, targetVariable);
			}
			else
			{
				targetVariable = i->second;
			}
		}

		void SaveVec3(const fstring& key, const Vec3& value)
		{
			auto i = vec3s.insert(key, value);
			if (!i.second)
			{
				i.first->second = value;
			}
		}

		float/* value */ LoadF32(const fstring& key, float defaultValue) override
		{
			auto i = floats.find(key);
			if (i == floats.end())
			{
				float targetVariable = defaultValue;
				floats.insert(key, targetVariable);
				return targetVariable;
			}
			else
			{
				return i->second;
			}
		}

		void SaveF32(const fstring& key, float value) override
		{
			auto i = floats.insert(key, value);
			if (!i.second)
			{
				i.first->second = value;
			}
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	IArchiveSupervisor* CreateArchive()
	{
		return new Archive();
	}
}