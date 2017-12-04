#include "hv.h"
#include <unordered_map>

using namespace HV;

namespace ANON
{
	enum VariableType: int32
	{
		VariableType_Float
	};

	cstr TypeName(VariableType t)
	{
		switch (t)
		{
		case VariableType_Float: return "float";
		default: return "unknown";
		}
	}

	template<typename T> struct VariableValue
	{
		T defaultValue;
		T minValue;
		T maxValue;
		T value;
	};

	struct Variable
	{
		VariableType Type;
		VariableValue<float> floatValue;
	};

	struct ScriptConfig : public IScriptConfigSupervisor
	{
		std::unordered_map<std::string, Variable*> variables;

		~ScriptConfig()
		{
			for (auto& v : variables)
			{
				delete v.second;
			}
		}

		void BindProperties(IBloodyPropertySetEditor& editor)
		{
			for (auto& v : variables)
			{
				switch (v.second->Type)
				{
				case VariableType_Float:
				{
					auto& f = v.second->floatValue;
					editor.AddFloat(v.first.c_str(), &f.value, f.minValue, f.maxValue);
				}
				break;
				}
			}
		}

		void Free() override
		{
			delete this;
		}

		float GetFloat(const fstring& variableName, float defaultValue, float minValue, float maxValue) override
		{
			auto i = variables.find((cstr)variableName);
			if (i == variables.end())
			{
				i = variables.insert(std::make_pair(std::string(variableName), new Variable())).first;
				i->second->floatValue = { defaultValue, minValue, maxValue, defaultValue };
				i->second->Type = VariableType_Float;
			}
			else
			{
				if (i->second->Type != VariableType_Float)
				{
					Throw(0, "ScriptConfig::GetFloat(\"%s\", ...): variable type first seen as %s", TypeName(i->second->Type));
				}
			}

			return i->second->floatValue.value;
		}
	};

	struct ScriptSet: IScriptConfigSet
	{
		IScriptConfigSupervisor* current = nullptr;
		std::unordered_map<std::string, IScriptConfigSupervisor*> scripts;

		~ScriptSet()
		{
			for (auto& i : scripts)
			{
				i.second->Free();
			}
		}

		IScriptConfigSupervisor& Current() override
		{
			if (!current) Throw(0, "ScriptSet: No script set");
			return *current;
		}

		void Free() override
		{
			delete this;
		}

		void SetCurrentScript(cstr scriptName) override
		{
			auto i = scripts.find(scriptName);
			if (i == scripts.end())
			{
				scripts[scriptName] = current = HV::CreateScriptConfig();
			}
			else
			{
				current = i->second;
			}
		}
	};
}

namespace HV
{
	IScriptConfigSupervisor* CreateScriptConfig()
	{
		return new ANON::ScriptConfig();
	}

	IScriptConfigSet* CreateScriptConfigSet()
	{
		return new ANON::ScriptSet();
	}
}