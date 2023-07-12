#include "hv.h"
#include <vector>
#include <rococo.strings.h>
#include <rococo.clock.h>

using namespace Rococo::Strings;

namespace
{
	using namespace HV;

	struct AIStore: ITriggersAndActions, ITagContainer
	{
		std::vector<ITriggerSupervisor*> triggers;
		IIActionFactoryCreateContext& afcc;

		AIStore(IIActionFactoryCreateContext& _afcc) : afcc(_afcc) {}
		~AIStore()
		{
			for (auto* t : triggers)
			{
				t->Free();
			}
		}

		struct Tags : public IStringVector
		{
			std::vector<HString> items;

			int32 Count() const override
			{
				return (int32)items.size();
			}

			void GetItem(int32 index, char* text, size_t capacity) const
			{
				SafeFormat(text, capacity, "%s", items[index].c_str());
			}
		} tags;

		bool AddTag(int32 pos, cstr text) override
		{
			for (auto& t : tags.items)
			{
				if (Eq(t.c_str(), text))
				{
					// Duplicate - ignore
					return false;
				}
			}

			if (pos >= (int32)tags.items.size())
			{
				tags.items.push_back(text);
				return true;
			}

			auto i = tags.items.begin();

			if (pos > 0)
			{
				std::advance(i, pos);
			}

			tags.items.insert(i, text);
			return true;
		}

		void RaiseTag(int32 pos) override
		{
			if (pos > 0 && pos < tags.items.size())
			{
				std::swap(tags.items[pos], tags.items[pos - 1]);
			}
		}

		void LowerTag(int32 pos) override
		{
			if (tags.items.size() >= 2 && pos >= 0 && pos < tags.items.size() - 1)
			{
				std::swap(tags.items[pos], tags.items[pos + 1]);
			}
		}

		int32 TagCount() const override
		{
			return (int32)tags.items.size();
		}

		void SetTag(int32 pos, cstr text) override
		{
			if (pos < 0 || pos >= tags.items.size())
			{
				AddTag(pos, text);
				return;
			}

			for (auto& t : tags.items)
			{
				if (Eq(t.c_str(), text))
				{
					return; // We are not alloed to duplicate
				}
			}

			tags.items[pos] = text;
		}

		void RemoveTag(int32 pos) override
		{
			if (pos < 0 || pos >= tags.items.size())
			{
				return;
			}

			auto i = tags.items.begin();
			std::advance(i, pos);
			tags.items.erase(i);
		}

		void AddAction(int32 triggerIndex)
		{
			if (triggerIndex < 0 || triggerIndex >(int32) triggers.size())
			{
				Throw(0, "Sector.AddAction(%d) -> index out of range", triggerIndex);
			}

			auto& t = *triggers[triggerIndex];
			t.Actions().AddAction(GetDefaultActionFactory(), afcc);
		}

		void RemoveAction(int32 triggerIndex, int32 actionIndex)
		{
			if (triggerIndex < 0 || triggerIndex >(int32) triggers.size())
			{
				Throw(0, "Sector.RemoveAction(%d) -> index out of range", triggerIndex);
			}

			auto& t = *triggers[triggerIndex];
			if (actionIndex >= 0 && actionIndex < t.Actions().Count())
			{
				t.Actions().RemoveAction(actionIndex);
			}
		}

		void AddTrigger(int32 pos) override
		{
			auto* t = CreateTrigger();

			if (pos >= 0 && pos < (int32)triggers.size())
			{
				auto i = triggers.begin();
				std::advance(i, pos);
				triggers.insert(i, t);
			}
			else
			{
				triggers.push_back(t);
			}
		}

		void RemoveTrigger(int32 pos)
		{
			if (pos >= 0 && pos < (int32)triggers.size())
			{
				auto i = triggers.begin();
				std::advance(i, pos);
				triggers.erase(i);
			}
			else
			{
				Throw(0, "Sector.TriggersAndActions.RemoveTrigger[%d] Bad trigger index", pos);
			}
		}

		int32 TriggerCount() const
		{
			return (int32)triggers.size();
		}

		ITrigger& operator[](int32 i)
		{
			return *triggers[i];
		}

		IStringVector& EnumTags()
		{
			return tags;
		}

		void Trigger(TriggerType type)
		{
			for (auto* t : triggers)
			{
				if (t->Type() == type)
				{
					t->QueueActionSequence();
				}
			}
		}
	};

	struct AIBuilder : ISectorAIBuilderSupervisor
	{
		IIActionFactoryCreateContext& afcc;
		AIStore aiStore;

		AIBuilder(IIActionFactoryCreateContext& _afcc):
			afcc(_afcc), aiStore(_afcc)
		{}

		void Free() override
		{
			delete this;
		}

		ITriggersAndActions& TriggersAndActions()
		{
			return aiStore;
		}

		void AddTag(const fstring& tag) override
		{
			if (tag.length == 0)
				Throw(0, "AIBuilder.AddTag(...): tag was 0 length");

			if (tag.length >= IGlobalVariables::MAX_VARIABLE_NAME_LENGTH)
				Throw(0, "AIBuilder.AddTag(...) : tag was > %u characters", IGlobalVariables::MAX_VARIABLE_NAME_LENGTH);

			afcc.GetGlobals().ValidateName(tag);

			aiStore.AddTag(-1, tag);
		}

		void ClearTriggers() override
		{
			aiStore.triggers.clear();
		}

		HString lastTrigger;

		void AddTrigger(const fstring& name) override
		{
			int32 index = (int32)aiStore.triggers.size();
			aiStore.AddTrigger(index);

			lastTrigger = name;

			TriggerType type;
			if (!HV::TryShortParse(name, type))
			{
				Throw(0, "ISectorAIBuilder.AddTrigger(...): Unrecognized trigger type: %s", (cstr)name);
			}

			aiStore.triggers[index]->SetType(type);
		}

		void Trigger(TriggerType type)
		{
			aiStore.Trigger(type);
		}

		IAction& GetLastAction()
		{
			if (aiStore.triggers.empty())
			{
				Throw(0, "GetLastAction(): no trigger to which to append action");
			}

			int32 triggerIndex = (int32)aiStore.triggers.size() - 1;

			auto& actions = aiStore.triggers[triggerIndex]->Actions();

			if (actions.Count() == 0)
			{
				Throw(0, "GetLastAction(): no actions for last trigger");
			}

			int32 actionIndex = actions.Count() - 1;

			return actions[actionIndex];
		}

		HString lastFactory;

		void AddAction(const fstring& factoryName) override
		{
			if (aiStore.triggers.empty())
			{
				Throw(0, "ISectorAIBuilder::AddAction(%s): no trigger to which to append action", (cstr)factoryName);
			}

			lastFactory = factoryName;

			int32 triggerIndex = (int32)aiStore.triggers.size() - 1;
			aiStore.AddAction(triggerIndex);

			auto& actions = aiStore.triggers[triggerIndex]->Actions();

			int32 actionIndex = actions.Count() - 1;

			IActionFactory& factoryRef = GetActionFactory(factoryName);
			actions.SetAction(actionIndex, factoryRef, afcc);
		}

		int32 FindParameterIndex(IAction& a, const fstring& argName)
		{
			for (int32 i = 0; i < a.ParameterCount(); ++i)
			{
				auto desc = a.GetParameterName(i);
				if (Eq(desc.name, argName))
				{
					return i;
				}
			}

			Throw(0, "Cannot find argument '%s' to %s of %s", (cstr)argName, lastFactory.c_str(), lastTrigger.c_str());
		}

		void AddActionArgumentI32(const fstring& argName, int32 value) override
		{
			IAction& a = GetLastAction();
			int32 paramIndex = FindParameterIndex(a, argName);

			char buf[32];
			SafeFormat(buf, sizeof buf, "%d", value);
			a.SetParameter(paramIndex, buf);
		}

		void AddActionArgumentF32(const fstring& argName, float value) override
		{
			IAction& a = GetLastAction();
			int32 paramIndex = FindParameterIndex(a, argName);

			char buf[32];
			SafeFormat(buf, sizeof buf, "%f", value);
			a.SetParameter(paramIndex, buf);
		}

		void AddActionArgumentString(const fstring& argName, const fstring& value) override
		{
			IAction& a = GetLastAction();
			int32 paramIndex = FindParameterIndex(a, argName);
			a.SetParameter(paramIndex, value);
		}

		void SaveAsScript(StringBuilder& sb) override
		{
			auto& ta = TriggersAndActions();
			if (ta.TriggerCount() > 0 || !aiStore.tags.items.empty())
			{
				sb << "\n\t(ISectorAIBuilder ai = (sectors.GetSectorAIBuilder id))";

				for (int i = 0; i < ta.TriggerCount(); ++i)
				{
					auto& trigger = ta[i];
					TriggerType type = trigger.Type();
					auto stype = ToShortString(type);
					sb << "\n\t(ai.AddTrigger \"" << (cstr)stype << "\")";

					auto& actions = trigger.Actions();
					for (int j = 0; j < actions.Count(); j++)
					{
						auto& action = actions[j];
						sb << "\n\t\t(ai.AddAction \"" << action.Factory().Name() << "\")";

						for (int k = 0; k < action.ParameterCount(); ++k)
						{
							auto desc = action.GetParameterName(k);

							ParameterBuffer buf;
							action.GetParameter(k, buf);

							switch (desc.type)
							{
							case PARAMETER_TYPE_EVENT_NAME:
							case PARAMETER_TYPE_GLOBALVAR_NAME:
							case PARAMETER_TYPE_SECTOR_STRING:
								sb << "\n\t\t\t(ai.AddActionArgumentString \"" << desc.name << "\" \"" << buf << "\")";
								break;
							case PARAMETER_TYPE_FLOAT:
								sb << "\n\t\t\t(ai.AddActionArgumentF32 \"" << desc.name << "\" " << buf << ")";
								break;
							case PARAMETER_TYPE_INT:
							case PARAMETER_TYPE_INT_UNBOUNDED:
							case PARAMETER_TYPE_INT_HEX:
								sb << "\n\t\t\t(ai.AddActionArgumentI32 \"" << desc.name << "\" " << buf << ")";
								{
									if (desc.type == PARAMETER_TYPE_INT_HEX)
									{
										int32 iValue = atoi(buf);
										sb.AppendFormat(" // 0x%8.8X\n", iValue);
									}
									else
									{
										sb << "\n";
									}
								}
								break;
							}
						}
					}
				}

				for (auto& t : aiStore.tags.items)
				{
					sb << "\n\t\t(ai.AddTag \"" << t.c_str() << "\")";
				}

				sb << "\n";
			}
		}

		void AdvanceInTime(IPublisher& publisher, const IUltraClock& clock) override
		{
			AdvanceInfo info{ publisher, clock.DT() };

			for (auto t : aiStore.triggers)
			{
				t->Advance(info);
			}
		}

		ITagContainer& Tags()
		{
			return aiStore;
		}
	};
}

namespace HV
{
	ISectorAIBuilderSupervisor* CreateSectorAI(IIActionFactoryCreateContext& afcc)
	{
		return new AIBuilder(afcc);
	}
}