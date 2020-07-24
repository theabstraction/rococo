#include "hv.h"
#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <rococo.rings.inl>
#include <vector>

namespace
{
	using namespace HV;

	struct Trigger : public ITriggerSupervisor, public IActionArray
	{
		TRIGGER_TYPE type = TRIGGER_TYPE_NONE;
		std::vector<IAction*> actions;

		Trigger()
		{

		}

		~Trigger()
		{
			for (auto action : actions)
			{
				action->Free();
			}
		}

		void Free() override
		{
			delete this;
		}

		TRIGGER_TYPE Type() const override { return type; }
		void SetType(TRIGGER_TYPE type) override { this->type = type; }
		IActionArray& Actions() override { return *this; }

		IAction& operator[](int32 index) override
		{
			if (index < 0 || index >= actions.size()) Throw(0, "Trigger.Action[%d] - bad index", index);
			return *actions[index];
		}

		int32 Count() const override
		{
			return (int32)actions.size();
		}

		void AddAction(IActionFactory& factory) override
		{
			actions.push_back(factory.Create());
		}
	};

	struct FixedEvent : public IAction
	{
		HString eventString;
		EventIdRef ev{ 0,0 };

		FixedEvent(cstr _eventString): eventString(_eventString)
		{

		}

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			if (eventString.length() != 0)
			{
				if (ev.hashCode == 0)
				{
					ev = info.publisher.CreateEventIdFromVolatileString(eventString);
				}

				struct TEventArgs<bool> noArgs;
				noArgs.data = false;
				info.publisher.Publish(noArgs, ev);
			}

			return ADVANCE_STATE_COMPLETED;
		}

		int32 ParameterCount() const override
		{
			return 0;
		}

		void GetParameter(int32 index, ParameterBuffer& buffer) const override
		{
			Throw(0, "GenericEvent.GetParameter(%d). Bad index", index);
		}

		void SetParameter(int32 index, cstr value) override
		{
			Throw(0, "GenericEvent.SetParameter(%d). Bad index", index);
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			Throw(0, "GenericEvent.GetParameterName(%d). Bad index", index);
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "Publish \"%s\"_event", eventString.length() == 0 ? "<none>" : eventString.c_str());
		}

		void Free() override
		{
			delete this;
		}
	};

	struct GenericEvent : public IAction
	{
		HString eventString = "";
		EventIdRef ev;

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			if (eventString.length() != 0)
			{
				if (ev.hashCode == 0)
				{
					ev = info.publisher.CreateEventIdFromVolatileString(eventString);
				}

				struct TEventArgs<bool> noArgs;
				noArgs.data = false;
				info.publisher.Publish(noArgs, ev);
			}

			return ADVANCE_STATE_COMPLETED;
		}

		int32 ParameterCount() const override
		{
			return 1;
		}

		void GetParameter(int32 index, ParameterBuffer& buffer) const override
		{
			if (index != 0) Throw(0, "GenericEvent.GetParameter(%d). Bad index", index);
			CopyString(buffer.data, buffer.CAPACITY, eventString);
		}

		void SetParameter(int32 index, cstr value) override
		{
			if (index != 0) Throw(0, "GenericEvent.SetParameter(%d). Bad index", index);
			eventString = value;
			ev = { 0,0 };
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			if (index != 0) Throw(0, "GenericEvent.GetParameterName(%d). Bad index", index);
			return { "event-name", PARAMETER_TYPE_EVENT_NAME, 0, 0 };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "GenericEvent %s", eventString.length() == 0 ? "<none>" : eventString.c_str());
		}

		void Free() override
		{
			delete this;
		}
	};

	struct GenericEventFactory : public IActionFactory
	{
		IAction* Create() override
		{
			return new GenericEvent();
		}

		cstr Name() const override
		{
			return "GenericEvent";
		}
	};

	GenericEventFactory s_GenericEventFactory;

	struct CountDown : public IAction
	{
		int countInit = 2;
		int count = 2;

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			if (count > 0)
			{
				return ADVANCE_STATE_YIELD;
			}
			else
			{
				count = countInit; // reset the counter
				return ADVANCE_STATE_COMPLETED;
			}
		}

		int32 ParameterCount() const override
		{
			return 1;
		}

		void GetParameter(int32 index, ParameterBuffer& buffer) const override
		{
			if (index != 0) Throw(0, "CountDown.GetParameter(%d). Bad index", index);
			SafeFormat(buffer.data, buffer.CAPACITY, "%d", countInit);
		}

		void SetParameter(int32 index, cstr value) override
		{
			if (index != 0) Throw(0, "CountDown.SetParameter(%d). Bad index", index);
			if (1 != sscanf_s(value, "%d", &countInit))
			{
				Throw(0, "CountDown.SetParameter(%d, %s). Bad int string", index, value);
			}

			count = countInit;
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			if (index != 0) Throw(0, "CountDown.GetParameterName(%d). Bad index", index);
			return { "count", PARAMETER_TYPE_INT, 0.0f, 10000.0f };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "CountDown %d", countInit);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct CountDownFactory : public IActionFactory
	{
		IAction* Create() override
		{
			return new CountDown();
		}

		cstr Name() const override
		{
			return "CountDown";
		}
	};

	CountDownFactory s_CountDownFactory;

	struct PossiblyAbort : public IAction
	{
		float probabilityPercent = 50.0f; // probabily of abort happening

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			// Catch edge cases, in case rng does not handle 0 and 100% exactly
			if (probabilityPercent <= 0) return ADVANCE_STATE_COMPLETED;
			if (probabilityPercent >= 100.0f) return ADVANCE_STATE_TERMINATE;		
				
			if (Random::NextFloat(GetRandomizer(), 0, 100.0f) < probabilityPercent)
			{
				return ADVANCE_STATE_TERMINATE;
			}
			else
			{
				return ADVANCE_STATE_COMPLETED;
			}
		}

		int32 ParameterCount() const override
		{
			return 1;
		}

		void GetParameter(int32 index, ParameterBuffer& buffer) const override
		{
			if (index != 0) Throw(0, "PossiblyAbort.GetParameter(%d). Bad index", index);
			SafeFormat(buffer.data, buffer.CAPACITY, "%f", probabilityPercent);
		}

		void SetParameter(int32 index, cstr value) override
		{
			if (index != 0) Throw(0, "PossiblyAbort.SetParameter(%d). Bad index", index);
			if (1 != sscanf_s(value, "%f", &probabilityPercent))
			{
				Throw(0, "PossiblyAbort.SetParameter(%d, %s). Bad float string", index, value);
			}
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			if (index != 0) Throw(0, "PossiblyAbort.GetParameterName(%d). Bad index", index);
			return { "%", PARAMETER_TYPE_FLOAT, 0.0f, 100.0f };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "PossiblyAbort %%%f", probabilityPercent);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct PossiblyAbortFactory : public IActionFactory
	{
		IAction* Create() override
		{
			return new PossiblyAbort();
		}

		cstr Name() const override
		{
			return "PossiblyAbort";
		}
	};

	PossiblyAbortFactory s_PossiblyAbortFactory;

	struct Delay : public IAction
	{
		float delayPeriod = 1.0_seconds;
		float expiredTime = 0.0_seconds;

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			expiredTime += info.dt;
			if (expiredTime > delayPeriod)
			{
				expiredTime = 0;
				return ADVANCE_STATE_COMPLETED;
			}
			else
			{
				return ADVANCE_STATE_YIELD;
			}
		}

		int32 ParameterCount() const override
		{
			return 1;
		}

		void GetParameter(int32 index, ParameterBuffer& buffer) const override
		{
			if (index != 0) Throw(0, "Delay.GetParameter(%d). Bad index", index);	
			SafeFormat(buffer.data, buffer.CAPACITY, "%f", delayPeriod);
		}

		void SetParameter(int32 index, cstr value) override
		{
			if (index != 0) Throw(0, "Delay.SetParameter(%d). Bad index", index);
			if (1 != sscanf_s(value, "%f", &delayPeriod))
			{
				Throw(0, "Delay.SetParameter(%d, %s). Bad float string", index, value);
			}
			expiredTime = 0;
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			if (index != 0) Throw(0, "LowerScenery.GetParameterName(%d). Bad index", index);
			return { "period", PARAMETER_TYPE_FLOAT, 0.0f, 3600.0f };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "Delay %f second%s", delayPeriod, delayPeriod != 1.0f ? "s" : "");
		}

		void Free() override
		{
			delete this;
		}
	};

	struct DelayFactory : public IActionFactory
	{
		IAction* Create() override
		{
			return new Delay();
		}

		cstr Name() const override
		{
			return "Delay";
		}
	};

	DelayFactory s_DelayFactory;

	struct RandomDelay : public IAction
	{
		float upperBound = 4.0_seconds;
		float lowerBound = 0.1_seconds;
		float delayPeriod = -1.0_seconds;
		float expiredTime = 0.0_seconds;

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			if (delayPeriod < 0)
			{
				delayPeriod = Random::NextFloat(GetRandomizer(), lowerBound, upperBound);
			}

			expiredTime += info.dt;

			if (expiredTime > delayPeriod)
			{
				expiredTime = 0;

				delayPeriod = Random::NextFloat(GetRandomizer(), lowerBound, upperBound);

				return ADVANCE_STATE_COMPLETED;
			}
			else
			{
				return ADVANCE_STATE_YIELD;
			}
		}

		int32 ParameterCount() const override
		{
			return 2;
		}

		void GetParameter(int32 index, ParameterBuffer& buffer) const override
		{
			switch (index)
			{
			default: Throw(0, "RandomDelay.GetParameter(%d). Bad index", index);
			case 0: SafeFormat(buffer.data, buffer.CAPACITY, "%f", lowerBound);
			case 1: SafeFormat(buffer.data, buffer.CAPACITY, "%f", upperBound);
			}	
		}

		void SetParameter(int32 index, cstr value) override
		{
			switch (index)
			{
			default: Throw(0, "RandomDelay.SetParameter(%d). Bad index", index);
			case 0:
			{
				if (1 != sscanf_s(value, "%f", &lowerBound))
				{
					Throw(0, "RandomDelay.SetParameter(0, %s). Bad float string", value);
				}
				lowerBound = 0;
				break;
			}
			case 1:
			{
				if (1 != sscanf_s(value, "%f", &upperBound))
				{
					Throw(0, "RandomDelay.SetParameter(1, %s). Bad float string", value);
				}
				upperBound = 0;
				break;
			}
			}

			if (lowerBound > upperBound) std::swap(lowerBound, upperBound);
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			switch (index)
			{
			default: Throw(0, "RandomDelay.GetParameterName(%d). Bad index", index);
			case 0: return { "lower-bound", PARAMETER_TYPE_FLOAT, 0.0f, 3600.0f };
			case 1: return { "upper-bound", PARAMETER_TYPE_FLOAT, 0.0f, 3600.0f };
			}
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "RandomDelay %f second%s", delayPeriod, delayPeriod != 1.0f ? "s" : "");
		}

		void Free() override
		{
			delete this;
		}
	};

	struct RandomDelayFactory: public IActionFactory
	{
		IAction* Create() override
		{
			return new RandomDelay();
		}

		cstr Name() const override
		{
			return "RandomDelay";
		}
	};

	RandomDelayFactory s_RandomDelayFactory;

	static auto ev_LowerScenery = "hv.action.scenery.lower"_event;

	struct LowerScenery : public IAction
	{
		HString target;

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			TEventArgs<cstr> args;
			args.data = target.c_str();
			if (args.data)
			{
				info.publisher.Publish(args, ev_LowerScenery);
			}

			return ADVANCE_STATE_COMPLETED;
		}

		int32 ParameterCount() const override
		{
			return 1;
		}

		void GetParameter(int32 index, ParameterBuffer& buf) const override
		{
			if (index != 0) Throw(0, "LowerScenery.GetParameter(%d). Bad index", index);
			CopyString(buf.data, buf.CAPACITY, target);
		}

		void SetParameter(int32 index, cstr value) override
		{
			if (index != 0) Throw(0, "LowerScenery.SetParameter(%d). Bad index", index);
			target = value;
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			if (index != 0) Throw(0, "LowerScenery.GetParameterName(%d). Bad index", index);
			return { "target", PARAMETER_TYPE_SECTOR_STRING, 0, 0 };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "LowerScenery @%s", target.length() == 0 ? "<none>" : target.c_str());
		}

		void Free() override
		{
			delete this;
		}
	};

	struct LowerSceneryFactory : public IActionFactory
	{
		IAction* Create() override
		{
			return new LowerScenery();
		}

		cstr Name() const override
		{
			return "LowerScenery";
		}
	};

	LowerSceneryFactory s_LowerSceneryFactory;

	static auto ev_RaiseScenery = "hv.action.scenery.raise"_event;

	struct RaiseScenery : public IAction
	{
		HString target;

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			TEventArgs<cstr> args;
			args.data = target.c_str();
			if (args.data)
			{
				info.publisher.Publish(args, ev_RaiseScenery);
			}

			return ADVANCE_STATE_COMPLETED;
		}

		int32 ParameterCount() const override
		{
			return 1;
		}

		void GetParameter(int32 index, ParameterBuffer& buf) const override
		{
			if (index != 0) Throw(0, "RaiseScenery.GetParameter(%d). Bad index", index);
			CopyString(buf.data, buf.CAPACITY, target);
		}

		void SetParameter(int32 index, cstr value) override
		{
			if (index != 0) Throw(0, "RaiseScenery.SetParameter(%d). Bad index", index);
			target = value;
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			if (index != 0) Throw(0, "RaiseScenery.GetParameterName(%d). Bad index", index);
			return { "target", PARAMETER_TYPE_SECTOR_STRING, 0, 0 };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "RaiseScenery @%s", target.length() == 0 ? "<none>" : target.c_str());
		}

		void Free() override
		{
			delete this;
		}
	};

	struct RaiseSceneryFactory : public IActionFactory
	{
		IAction* Create() override
		{
			return new RaiseScenery();
		}

		cstr Name() const override
		{
			return "RaiseScenery";
		}
	};

	RaiseSceneryFactory s_RaiseSceneryFactory;

	static auto ev_ToggleElevation = "hv.action.elevation.toggle"_event;

	struct ToggleElevation : public IAction
	{
		HString target;

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			TEventArgs<cstr> args;
			args.data = target.c_str();
			if (args.data)
			{
				info.publisher.Publish(args, ev_ToggleElevation);
			}

			return ADVANCE_STATE_COMPLETED;
		}

		int32 ParameterCount() const override
		{
			return 1;
		}

		void GetParameter(int32 index, ParameterBuffer& buf) const override
		{
			if (index != 0) Throw(0, "RaiseScenery.GetParameter(%d). Bad index", index);
			CopyString(buf.data, buf.CAPACITY, target);
		}

		void SetParameter(int32 index, cstr value) override
		{
			if (index != 0) Throw(0, "ToggleElevation.SetParameter(%d). Bad index", index);
			target = value;
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			if (index != 0) Throw(0, "ToggleDoor.GetParameterName(%d). Bad index", index);
			return { "target", PARAMETER_TYPE_SECTOR_STRING, 0, 0 };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "ToggleElevation @%s", target.length() == 0 ? "<none>" : target.c_str());
		}

		void Free() override
		{
			delete this;
		}
	};

	struct ToggleElevationFactory: public IActionFactory
	{
		IAction* Create() override
		{
			return new ToggleElevation();
		}

		cstr Name() const override
		{
			return "ToggleElevation";
		}
	};

	ToggleElevationFactory s_ToggleElevationFactory;

	std::vector<IActionFactory*> s_factories =
	{
		&s_RaiseSceneryFactory,
		&s_LowerSceneryFactory,
		&s_ToggleElevationFactory,
		&s_RandomDelayFactory,
		&s_DelayFactory,
		&s_PossiblyAbortFactory,
		&s_CountDownFactory,
		&s_GenericEventFactory,
	};
}

namespace HV
{
	ITriggerSupervisor* CreateTrigger()
	{
		return new Trigger();
	}

	IActionFactory& GetDefaultActionFactory()
	{
		return s_ToggleElevationFactory;
	}

	size_t ActionFactoryCount()
	{
		return s_factories.size();
	}

	IActionFactory& GetActionFactory(size_t index)
	{
		return *s_factories[index];
	}

	IActionFactory& GetActionFactory(cstr name)
	{
		for (auto* f : s_factories)
		{
			if (Eq(f->Name(), name))
			{
				return *f;
			}
		}

		Throw(0, "GetActionFactory(%s): No such factory", name);
	}
}