#define ROCOCO_COMPONENTS_CONFIG_API __declspec(dllexport)

#include <components/rococo.components.configuration.h>
#include <components/rococo.ecs.builder.inl>
#include <rococo.hashtable.h>

using namespace Rococo::Strings;

namespace Rococo::Components
{
	struct ConfigurationComponent : IConfigurationComponent
	{
		stringmap<int> mapInts;
		stringmap<float> mapFloats;
		stringmap<double> mapDoubles;
		stringmap<HString> mapStrings;

		ConfigurationComponent(InstanceInfo&)
		{

		}

		ComponentTypeInfo TypeInfo() const override
		{
			return ComponentTypeInfo{ "ConfigurationComponent" };
		};

		void Reflect(ComponentReflectionInfo& info) override
		{
			UNUSED(info);
		}

		void Add(const fstring& key, int value) override
		{
			mapInts.insert(key, value);
		}

		void Add(const fstring& key, float value) override
		{
			mapFloats.insert(key, value);
		}

		void Add(const fstring& key, double value) override
		{
			mapDoubles.insert(key, value);
		}

		void Add(const fstring& key, const fstring& value) override
		{
			mapStrings.insert(key, (cstr) value);
		}

		SearchResult<int> Get(const fstring& key, int defaultValue) override
		{
			auto i = mapInts.find(key);
			if (i != mapInts.end())
			{
				return { i->second, true };
			}
			else
			{
				return { defaultValue, false };
			}
		}

		SearchResult<float> Get(const fstring& key, float defaultValue) override
		{
			auto i = mapFloats.find(key);
			if (i != mapFloats.end())
			{
				return { i->second, true };
			}
			else
			{
				return { defaultValue, false };
			}
		}

		SearchResult<double> Get(const fstring& key, double defaultValue) override
		{
			auto i = mapDoubles.find(key);
			if (i != mapDoubles.end())
			{
				return { i->second, true };
			}
			else
			{
				return { defaultValue, false };
			}
		}

		bool Get(const fstring& key, Strings::IStringPopulator& populator) override
		{
			auto i = mapStrings.find(key);
			if (i != mapStrings.end())
			{
				populator.Populate(i->second.c_str());
				return true;
			}
			else
			{
				return false;
			}
		}
	};
}

DEFINE_AND_EXPORT_SINGLETON_METHODS_WITH_DEFAULT_FACTORY(ROCOCO_COMPONENTS_CONFIG_API, IConfigurationComponent, ConfigurationComponent)