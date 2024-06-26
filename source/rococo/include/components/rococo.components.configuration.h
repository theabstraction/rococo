#pragma once
#include <components/rococo.ecs.ex.h>
#include "../../component.modules/configuration/code-gen/config.sxh.h"

namespace Rococo::Components
{
	template<class TYPE>
	struct SearchResult
	{
		TYPE value;
		boolean32 wasFound;
	};

	// Designed for scripting, this component provides a designer the ability to store arbitrary data on a component without having to modify the C++
	ROCOCO_INTERFACE IConfigurationComponent: Rococo::Components::Generated::IConfigBase
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
}

#ifndef ROCOCO_COMPONENTS_CONFIG_API
# define ROCOCO_COMPONENTS_CONFIG_API ROCOCO_API_IMPORT
#endif

DECLARE_SINGLETON_METHODS(ROCOCO_COMPONENTS_CONFIG_API, IConfigurationComponent)