#pragma once
#include <rococo.ecs.h>

#ifndef ROCOCO_COMPONENTS_CONFIG_API
# define ROCOCO_COMPONENTS_CONFIG_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Components
{
	ROCOCO_INTERFACE IConfigurationComponents
	{
		virtual IComponentTable& Table() = 0;
	};

	// Designed for scripting, this component provides a designer the ability to store arbitrary data on a component without having to modify the C++
	ROCOCO_INTERFACE IConfigurationComponent
	{
		virtual void Add(const fstring& key, int value) = 0;
		virtual void Add(const fstring& key, float value) = 0;
		virtual void Add(const fstring& key, double value) = 0;
		virtual void Add(const fstring& key, const fstring& value) = 0;
		virtual SearchResult<int> Get(const fstring& key, int defaultValue) = 0;
		virtual SearchResult<float> Get(const fstring& key, float defaultValue) = 0;
		virtual SearchResult<double> Get(const fstring& key, double defaultValue) = 0;
		virtual bool Get(const fstring& key, Strings::IStringPopulator& populator) = 0;
	};

	ROCOCO_COMPONENTS_CONFIG_API Ref<IConfigurationComponent> AddConfigurationComponent(ROID id);
	ROCOCO_COMPONENTS_CONFIG_API void ConfigurationComponent_LinkToECS(IECS* ecs);
}