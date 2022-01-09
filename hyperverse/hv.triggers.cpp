#include "hv.h"
#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <rococo.rings.inl>
#include <rococo.random.h>
#include <vector>
#include <string>

namespace
{
	using namespace HV;

	static auto ev_LowerScenery = "hv.action.scenery.lower"_event;
	static auto ev_RaiseScenery = "hv.action.scenery.raise"_event;
	static auto ev_ToggleElevation = "hv.action.elevation.toggle"_event;

	struct AIBrain : IAIBrain, IObserver
	{
		IPublisher& publisher;
		ITags& tags;

		AIBrain(IPublisher& _publisher, ISectors& _sectors):
			publisher(_publisher), tags(_sectors.Tags())
		{
			publisher.Subscribe(this, ev_LowerScenery);
			publisher.Subscribe(this, ev_RaiseScenery);
			publisher.Subscribe(this, ev_ToggleElevation);
		}

		void Free() override
		{
			delete this;
		}

		void OnEvent(Event& ev) override
		{
			if (ev == ev_LowerScenery)
			{
				auto& tag = As<TEventArgs<cstr>>(ev);

				struct : ITagCallback
				{
					void OnTag(TagContext& tc) override
					{
						tc.sector.Contents().LowerScenery();
					}
				} lowerScenery;
				tags.ForEachSectorWithTag(tag, lowerScenery);
			}
			else if (ev == ev_RaiseScenery)
			{
				auto& tag = As<TEventArgs<cstr>>(ev);
				struct : ITagCallback
				{
					void OnTag(TagContext& tc) override
					{
						tc.sector.Contents().RaiseScenery();
					}
				} raiseScenery;
				tags.ForEachSectorWithTag(tag, raiseScenery);
			}
			else if (ev == ev_ToggleElevation)
			{
				auto& tag = As<TEventArgs<cstr>>(ev);
				struct : ITagCallback
				{
					void OnTag(TagContext& tc) override
					{
						tc.sector.Contents().ToggleElevation();
					}
				} toggleElevation;
				tags.ForEachSectorWithTag(tag, toggleElevation);
			}
		}
	};

	struct Trigger : ITriggerSupervisor, IActionArray, IStringVector
	{
		TriggerType type = TriggerType_None;
		std::vector<IAction*> actions;

		enum { SLEEPING = -1};

		bool isInterruptible = true;
		size_t nextAction = (size_t) SLEEPING;

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

		void QueueActionSequence() override
		{
			if (isInterruptible && !actions.empty())
			{
				nextAction = 0;
			}
		}

		void Advance(AdvanceInfo& info) override
		{
			// Assumes dt > 0
			while (nextAction < actions.size())
			{
				ADVANCE_STATE state = actions[nextAction]->Advance(info);

				switch (state)
				{
				case ADVANCE_STATE_YIELD:
					isInterruptible = true;
					return;

				case ADVANCE_STATE_YIELD_UNINTERRUPTIBLE:
					isInterruptible = false;
					return;

				case ADVANCE_STATE_COMPLETED:
					nextAction++;
					break;

				case ADVANCE_STATE_TERMINATE:
					break;
				}
			}

			// All actions in a sequence have been executed, and so the sequence terminates
			isInterruptible = true;
			nextAction = (size_t)SLEEPING;
		}

		void Free() override
		{
			delete this;
		}

		TriggerType Type() const override { return type; }
		void SetType(TriggerType type) override { this->type = type; }
		IActionArray& Actions() override { return *this; }
		IStringVector& GetStringVector() override { return *this; }

		IAction& operator[](int32 index) override
		{
			if (index < 0 || index >= actions.size()) Throw(0, "Trigger.Action[%d] - bad index", index);
			return *actions[index];
		}

		void GetItem(int32 index, char* buffer, size_t capacity) const
		{
			actions[index]->Format(buffer, capacity);
		}

		int32 Count() const override
		{
			return (int32)actions.size();
		}

		void AddAction(IActionFactory& factory, IIActionFactoryCreateContext& context) override
		{
			isInterruptible = true;
			nextAction = (size_t)SLEEPING;
			actions.push_back(factory.Create(context));
		}

		void RemoveAction(int32 index)
		{
			isInterruptible = true;
			nextAction = (size_t)SLEEPING;
			actions[index]->Free();
			auto i = actions.begin();
			std::advance(i, index);
			actions.erase(i);
		}

		void SetAction(int32 index, IActionFactory& factory, IIActionFactoryCreateContext& context) override
		{
			isInterruptible = true;
			nextAction = (size_t)SLEEPING;
			actions[index]->Free();
			auto i = actions.begin();
			std::advance(i, index);
			actions[index] = factory.Create(context);
		}

		void Swap(int32 i, int32 j)
		{
			isInterruptible = true;
			nextAction = (size_t)SLEEPING;
			std::swap(actions[i], actions[j]);
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
				noArgs.value = false;
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

		IActionFactory& factory;
		IActionFactory& Factory() override { return factory; }

		GenericEvent(IActionFactory& _factory):
			factory(_factory)
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
				noArgs.value = false;
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
			SafeFormat(buffer.data, buffer.CAPACITY, "%s", eventString.c_str());
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
			return { "event", PARAMETER_TYPE_EVENT_NAME, 0, 0 };
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
		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			return new GenericEvent(*this);
		}

		cstr Name() const override
		{
			return "GenericEvent";
		}
	};

	GenericEventFactory s_GenericEventFactory;

	void StringToNumber(cstr s, int32& value)
	{
		value = atoi(s);
	}

	void StringToNumber(cstr s, int64& value)
	{
		value = _atoi64(s);
	}

	void StringToNumber(cstr s, float& value)
	{
		_CRT_FLOAT f;
		value = 0 == _atoflt(&f, s) ? f.f : 0;
	}

	void StringToNumber(cstr s, double& value)
	{
		value = atof(s);
	}

	template<typename NUMBER_TYPE, typename Transform>
	struct GlobalNumericVariableManipulator : public IAction
	{
		enum { DEFAULT_NUMBER_VALUE = 0 };

		NUMBER_TYPE arg = DEFAULT_NUMBER_VALUE;
		IGlobalVariables& globals;

		char variableName[IGlobalVariables::MAX_VARIABLE_NAME_LENGTH] = "";

		IActionFactory& factory;
		IActionFactory& Factory() override { return factory; }

		GlobalNumericVariableManipulator(IGlobalVariables& _globals, IActionFactory& _factory) :
			globals(_globals), factory(_factory)
		{
		}

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			NUMBER_TYPE value = (NUMBER_TYPE) DEFAULT_NUMBER_VALUE;
			globals.GetValue(variableName, value);

			NUMBER_TYPE transformedValue = Transform::Operator(value, arg);
			globals.SetValue(variableName, transformedValue);

			return ADVANCE_STATE_COMPLETED;
		}

		int32 ParameterCount() const override
		{
			return 2;
		}

		void GetParameter(int32 index, ParameterBuffer& buffer) const override
		{
			switch (index)
			{
			default: Throw(0, "GlobalNumericVariableManipulator.GetParameter(%d). Bad index", index);
			case 0: SafeFormat(buffer.data, buffer.CAPACITY, "%s", (cstr)variableName); break;
			case 1:
			{
				Rococo::StackStringBuilder sb(buffer.data, buffer.CAPACITY);
				sb << arg;
				break;
			}
			}
		}

		void SetParameter(int32 index, cstr value) override
		{
			switch (index)
			{
			default: Throw(0, "GlobalNumericVariableManipulator.SetParameter(%d). Bad index", index); break;
			case 0: SafeFormat(variableName, sizeof variableName, "%s", value); break;
			case 1: StringToNumber(value, arg); break;
			}
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			switch (index)
			{
			default: Throw(0, "GlobalNumericVariableManipulator.GetParameterName(%d). Bad index", index); break;
			case 0: return { "int32", PARAMETER_TYPE_GLOBALVAR_NAME, 0.0f,  0.0f }; break;
			case 1: return { "constant", PARAMETER_TYPE_INT_HEX, 0.0f,  0.0f }; break;
			}
		}

		void Format(char* buffer, size_t capacity) override
		{
			cstr proxyName = *variableName ? variableName : "<undefined>";
			StackStringBuilder sb(buffer, capacity);
			sb.AppendFormat("Int32 %s -> %s ", proxyName, Transform::Name()) << arg;
			int32 bigEnd = 0x0000FFFF & (arg >> 16);
			int32 littleEnd = 0x0000FFFF & arg;
			sb.AppendFormat(" { 0x%4.4X:%4.4X }", bigEnd, littleEnd);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct GlobalInt32LogicalAndFactory: public IActionFactory
	{
		GlobalInt32LogicalAndFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct AndInt32
			{
				static int32 Operator(int32 value, int32 arg)
				{
					return value & arg;
				}

				static cstr Name()
				{
					return "Bitwise-AND";
				}
			};
			return new GlobalNumericVariableManipulator<int32, AndInt32>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Bitwise_And";
		}
	};

	GlobalInt32LogicalAndFactory s_GlobalInt32LogicalAndFactory;

	struct GlobalInt32LogicalOrFactory : public IActionFactory
	{
		GlobalInt32LogicalOrFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct OrInt32
			{
				static int32 Operator(int32 value, int32 arg)
				{
					return value | arg;
				}

				static cstr Name()
				{
					return "Bitwise-OR";
				}
			};
			return new GlobalNumericVariableManipulator<int32, OrInt32>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Bitwise_Or";
		}
	};

	GlobalInt32LogicalOrFactory s_GlobalInt32LogicalOrFactory;

	// N.B: Y xor 1 yields 0 when Y = 1, and 1 when Y = 0, so Y xor 0xFFFFFFFF is bitwise NOT
	// Hence we do not need a bitwise not factory
	struct GlobalInt32LogicalXorFactory : public IActionFactory
	{
		GlobalInt32LogicalXorFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct XorInt32
			{
				static int32 Operator(int32 value, int32 arg)
				{
					return value ^ arg;
				}

				static cstr Name()
				{
					return "Bitwise-XOR";
				}
			};
			return new GlobalNumericVariableManipulator<int32, XorInt32>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Bitwise_Xor";
		}
	};

	GlobalInt32LogicalXorFactory s_GlobalInt32LogicalXorFactory;

	struct GlobalInt32SetFactory : public IActionFactory
	{
		GlobalInt32SetFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct Set
			{
				static int32 Operator(int32 value, int32 arg)
				{
					return arg;
				}

				static cstr Name()
				{
					return "Set to";
				}
			};
			return new GlobalNumericVariableManipulator<int32, Set>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Set";
		}
	};

	GlobalInt32SetFactory s_GlobalInt32SetFactory;

	struct GlobalInt32AddFactory : public IActionFactory
	{
		GlobalInt32AddFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct Add
			{
				static int32 Operator(int32 value, int32 arg)
				{
					return value + arg;
				}

				static cstr Name()
				{
					return "Add";
				}
			};
			return new GlobalNumericVariableManipulator<int32, Add>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Add";
		}
	};

	GlobalInt32AddFactory s_GlobalInt32AddFactory;

	// Add also covers subtract when the argument is negative
	struct GlobalInt32SubtractFactory : public IActionFactory
	{
		GlobalInt32SubtractFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct Subtract
			{
				static int32 Operator(int32 value, int32 arg)
				{
					return value - arg;
				}

				static cstr Name()
				{
					return "Subtract";
				}
			};
			return new GlobalNumericVariableManipulator<int32, Subtract>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Subtract";
		}
	};

	GlobalInt32SubtractFactory s_GlobalInt32SubtractFactory;

	// Shift left if arg +ve and right if arg -ve
	struct GlobalInt32ShiftFactory : public IActionFactory
	{
		GlobalInt32ShiftFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct Add
			{
				static int32 Operator(int32 value, int32 arg)
				{
					if (arg > 0)
					{
						return value << arg;
					}
					else if (arg < 0)
					{
						return value >> (-arg);
					}
					else
					{
						return value;
					}
				}

				static cstr Name()
				{
					return "BitShift";
				}
			};
			return new GlobalNumericVariableManipulator<int32, Add>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_BitShift";
		}
	};

	GlobalInt32ShiftFactory s_GlobalInt32ShiftFactory;

	template<typename NUMBER_TYPE, typename Test>
	struct GlobalNumericVariableTest : public IAction
	{
		enum { DEFAULT_NUMBER_VALUE = 0 };

		NUMBER_TYPE arg = DEFAULT_NUMBER_VALUE;
		IGlobalVariables& globals;

		char variableName[IGlobalVariables::MAX_VARIABLE_NAME_LENGTH] = "";

		IActionFactory& factory;
		IActionFactory& Factory() { return factory;  }

		GlobalNumericVariableTest(IGlobalVariables& _globals, IActionFactory& _factory) :
			globals(_globals), factory(_factory)
		{
		}

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			NUMBER_TYPE value = (NUMBER_TYPE)DEFAULT_NUMBER_VALUE;
			globals.GetValue(variableName, value);

			bool hasPassed = Test::IsTrue(value, arg);
			
			return hasPassed ? ADVANCE_STATE_COMPLETED : ADVANCE_STATE_TERMINATE;
		}

		int32 ParameterCount() const override
		{
			return 2;
		}

		void GetParameter(int32 index, ParameterBuffer& buffer) const override
		{
			switch (index)
			{
			default: Throw(0, "GlobalNumericVariableTest.GetParameter(%d). Bad index", index);
			case 0: SafeFormat(buffer.data, buffer.CAPACITY, "%s", (cstr)variableName); break;
			case 1:
			{
				Rococo::StackStringBuilder sb(buffer.data, buffer.CAPACITY);
				sb << arg;
				break;
			}
			}
		}

		void SetParameter(int32 index, cstr value) override
		{
			switch (index)
			{
			default: Throw(0, "GlobalNumericVariableTest.SetParameter(%d). Bad index", index); break;
			case 0: SafeFormat(variableName, sizeof variableName, "%s", value); break;
			case 1: StringToNumber(value, arg); break;
			}
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			switch (index)
			{
			default: Throw(0, "GlobalNumericVariableTest.GetParameterName(%d). Bad index", index); break;
			case 0: return { "int32", PARAMETER_TYPE_GLOBALVAR_NAME, 0.0f,  0.0f }; break;
			case 1: return { "constant", PARAMETER_TYPE_INT_HEX, 0.0f,  0.0f }; break;
			}
		}

		void Format(char* buffer, size_t capacity) override
		{
			StackStringBuilder sb(buffer, capacity);
			cstr proxyName = *variableName ? variableName : "<undefined>";
			sb << "Continue if " << proxyName << " " << Test::Name() << " " << arg;

			int32 bigEnd = 0x0000FFFF & (arg >> 16);
			int32 littleEnd = 0x0000FFFF & arg;
			sb.AppendFormat(" { 0x%4.4X:%4.4X }", bigEnd, littleEnd);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct GlobalInt32LTEFactory : public IActionFactory
	{
		GlobalInt32LTEFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct LTE
			{
				static int32 IsTrue(int32 value, int32 arg)
				{
					return value <= arg;
				}

				static cstr Name()
				{
					return "<=";
				}
			};
			return new GlobalNumericVariableTest<int32, LTE>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Continue_LTE";
		}
	};

	GlobalInt32LTEFactory s_GlobalInt32TestLTEFactory;

	struct GlobalInt32LTFactory : public IActionFactory
	{
		GlobalInt32LTFactory() 
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct LT
			{
				static int32 IsTrue(int32 value, int32 arg)
				{
					return value < arg;
				}

				static cstr Name()
				{
					return "<";
				}
			};
			return new GlobalNumericVariableTest<int32, LT>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Continue_LT";
		}
	};

	GlobalInt32LTFactory s_GlobalInt32TestLTFactory;

	struct GlobalInt32EQFactory : public IActionFactory
	{
		GlobalInt32EQFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct EQ
			{
				static int32 IsTrue(int32 value, int32 arg)
				{
					return value == arg;
				}

				static cstr Name()
				{
					return "==";
				}
			};
			return new GlobalNumericVariableTest<int32, EQ>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Continue_EQ";
		}
	};

	GlobalInt32EQFactory s_GlobalInt32TestEQFactory;

	struct GlobalInt32TestAndFactory : public IActionFactory
	{
		GlobalInt32TestAndFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct And
			{
				static int32 IsTrue(int32 value, int32 arg)
				{
					return (value & arg) != 0;
				}

				static cstr Name()
				{
					return "Bitwise-AND";
				}
			};
			return new GlobalNumericVariableTest<int32, And>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Continue_And";
		}
	};

	GlobalInt32TestAndFactory s_GlobalInt32TestAndFactory;

	struct GlobalInt32TestNorFactory : public IActionFactory
	{
		GlobalInt32TestNorFactory() 
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct Nor
			{
				static int32 IsTrue(int32 value, int32 arg)
				{
					return (value & arg) == 0;
				}

				static cstr Name()
				{
					return "Bitwise-Nor";
				}
			};
			return new GlobalNumericVariableTest<int32, Nor>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Continue_Nor";
		}
	};

	GlobalInt32TestNorFactory s_GlobalInt32TestNorFactory;

	struct GlobalInt32GTFactory : public IActionFactory
	{
		GlobalInt32GTFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct GT
			{
				static int32 IsTrue(int32 value, int32 arg)
				{
					return value > arg;
				}

				static cstr Name()
				{
					return ">";
				}
			};
			return new GlobalNumericVariableTest<int32, GT>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Continue_GT";
		}
	};

	GlobalInt32GTFactory s_GlobalInt32TestGTFactory;

	struct GlobalInt32GTEFactory : public IActionFactory
	{
		GlobalInt32GTEFactory()
		{

		}

		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			struct GTE
			{
				static int32 IsTrue(int32 value, int32 arg)
				{
					return value >= arg;
				}

				static cstr Name()
				{
					return ">=";
				}
			};
			return new GlobalNumericVariableTest<int32, GTE>(context.GetGlobals(), *this);
		}

		cstr Name() const override
		{
			return "int32_Continue_GTE";
		}
	};

	GlobalInt32GTEFactory s_GlobalInt32TestGTEFactory;

	struct CountDown : public IAction
	{
		int countInit = 2;
		int count = 2;

		IActionFactory& factory;
		IActionFactory& Factory() override { return factory; }

		CountDown(IActionFactory& _factory) : factory(_factory) {}

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			if (count > 0)
			{
				return ADVANCE_STATE_COMPLETED;
			}
			else
			{
				count = countInit; // reset the counter
				return ADVANCE_STATE_TERMINATE;
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
			return { "from", PARAMETER_TYPE_INT, 0.0f, 10000000.0f };
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
		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			return new CountDown(*this);
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

		IActionFactory& factory;
		IActionFactory& Factory() override { return factory; }

		PossiblyAbort(IActionFactory& _factory) : factory(_factory) {}

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			// Catch edge cases, in case rng does not handle 0 and 100% exactly
			if (probabilityPercent <= 0) return ADVANCE_STATE_COMPLETED;
			if (probabilityPercent >= 100.0f) return ADVANCE_STATE_TERMINATE;		
				
			if (Rococo::Random::NextFloat(GetRandomizer(), 0, 100.0f) < probabilityPercent)
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

			if (probabilityPercent > 20.0f)
			{
				SafeFormat(buffer.data, buffer.CAPACITY, "%.0f", probabilityPercent);
			}
			else if (probabilityPercent > 5.0f)
			{
				SafeFormat(buffer.data, buffer.CAPACITY, "%.1f", probabilityPercent);
			}
			else
			{
				SafeFormat(buffer.data, buffer.CAPACITY, "%f", probabilityPercent);
			}
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
			if (probabilityPercent <= 0)
			{
				SafeFormat(buffer, capacity, "Abort %%0.0 of the time (i.e never)");
			}
			else if (probabilityPercent >= 100.0f)
			{
				SafeFormat(buffer, capacity, "Abort %%100 of the time (i.e always)");
			}
			else
			{
				if (probabilityPercent < 1.0f)
				{
					SafeFormat(buffer, capacity, "Abort %%%.4f of the time", probabilityPercent);
				}
				else if (probabilityPercent < 10.0f)
				{
					SafeFormat(buffer, capacity, "Abort %%%.1f of the time", probabilityPercent);
				}
				else
				{
					SafeFormat(buffer, capacity, "Abort %%%.0f of the time", probabilityPercent);
				}
			}
		}

		void Free() override
		{
			delete this;
		}
	};

	struct PossiblyAbortFactory : public IActionFactory
	{
		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			return new PossiblyAbort(*this);
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

		IActionFactory& factory;
		IActionFactory& Factory() override { return factory; }

		Delay(IActionFactory& _factory) : factory(_factory) {}

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
			return { "duration", PARAMETER_TYPE_FLOAT, 0.0f, 3600.0f };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "Delay %.2f secs", delayPeriod);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct DelayFactory : public IActionFactory
	{
		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			return new Delay(*this);
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

		IActionFactory& factory;
		IActionFactory& Factory() override { return factory; }

		RandomDelay(IActionFactory& _factory) : factory(_factory) {}

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
			case 0: SafeFormat(buffer.data, buffer.CAPACITY, "%f", lowerBound); break;
			case 1: SafeFormat(buffer.data, buffer.CAPACITY, "%f", upperBound); break;
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
				break;
			}
			case 1:
			{
				if (1 != sscanf_s(value, "%f", &upperBound))
				{
					Throw(0, "RandomDelay.SetParameter(1, %s). Bad float string", value);
				}
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
			case 0: return { "min", PARAMETER_TYPE_FLOAT, 0.0f, 3600.0f };
			case 1: return { "max", PARAMETER_TYPE_FLOAT, 0.0f, 3600.0f };
			}
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "RandomDelay %.2f-%.2f secs", lowerBound, upperBound);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct RandomDelayFactory: public IActionFactory
	{
		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			return new RandomDelay(*this);
		}

		cstr Name() const override
		{
			return "RandomDelay";
		}
	};

	RandomDelayFactory s_RandomDelayFactory;

	struct LowerScenery : public IAction
	{
		HString target;

		IActionFactory& factory;
		IActionFactory& Factory() override { return factory; }

		LowerScenery(IActionFactory& _factory) : factory(_factory) {}

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			TEventArgs<cstr> args;
			args.value = target.c_str();
			if (args.value)
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
			SafeFormat(buf.data, buf.CAPACITY, "%s", target.c_str());
		}

		void SetParameter(int32 index, cstr value) override
		{
			if (index != 0) Throw(0, "LowerScenery.SetParameter(%d). Bad index", index);
			target = value;
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			if (index != 0) Throw(0, "LowerScenery.GetParameterName(%d). Bad index", index);
			return { "sector", PARAMETER_TYPE_SECTOR_STRING, 0, 0 };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "LowerScenery %s", target.length() == 0 ? "<none>" : target.c_str());
		}

		void Free() override
		{
			delete this;
		}
	};

	struct LowerSceneryFactory : public IActionFactory
	{
		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			return new LowerScenery(*this);
		}

		cstr Name() const override
		{
			return "LowerScenery";
		}
	};

	LowerSceneryFactory s_LowerSceneryFactory;

	struct RaiseScenery : public IAction
	{
		HString target;

		IActionFactory& factory;
		IActionFactory& Factory() override { return factory; }

		RaiseScenery(IActionFactory& _factory) : factory(_factory) {}

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			TEventArgs<cstr> args;
			args.value = target.c_str();
			if (args.value)
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
			SafeFormat(buf.data, buf.CAPACITY, "%s", target.c_str());
		}

		void SetParameter(int32 index, cstr value) override
		{
			if (index != 0) Throw(0, "RaiseScenery.SetParameter(%d). Bad index", index);
			target = value;
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			if (index != 0) Throw(0, "RaiseScenery.GetParameterName(%d). Bad index", index);
			return { "sector", PARAMETER_TYPE_SECTOR_STRING, 0, 0 };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "RaiseScenery %s", target.length() == 0 ? "<none>" : target.c_str());
		}

		void Free() override
		{
			delete this;
		}
	};

	struct RaiseSceneryFactory : public IActionFactory
	{
		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			return new RaiseScenery(*this);
		}

		cstr Name() const override
		{
			return "RaiseScenery";
		}
	};

	RaiseSceneryFactory s_RaiseSceneryFactory;

	struct ToggleElevation : public IAction
	{
		HString target;

		IActionFactory& factory;
		IActionFactory& Factory() override { return factory; }

		ToggleElevation(IActionFactory& _factory) : factory(_factory) {}

		ADVANCE_STATE Advance(AdvanceInfo& info)
		{
			TEventArgs<cstr> sector;
			sector.value = target.c_str();
			if (sector.value)
			{
				info.publisher.Publish(sector, ev_ToggleElevation);
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
			SafeFormat(buf.data, buf.CAPACITY, "%s", target.c_str());
		}

		void SetParameter(int32 index, cstr value) override
		{
			if (index != 0) Throw(0, "ToggleElevation.SetParameter(%d). Bad index", index);
			target = value;
		}

		ParamDesc GetParameterName(int32 index) const override
		{
			if (index != 0) Throw(0, "ToggleDoor.GetParameterName(%d). Bad index", index);
			return { "sector", PARAMETER_TYPE_SECTOR_STRING, 0, 0 };
		}

		void Format(char* buffer, size_t capacity) override
		{
			SafeFormat(buffer, capacity, "ToggleElevation %s", target.length() == 0 ? "<none>" : target.c_str());
		}

		void Free() override
		{
			delete this;
		}
	};

	struct ToggleElevationFactory: public IActionFactory
	{
		IAction* Create(IIActionFactoryCreateContext& context) override
		{
			return new ToggleElevation(*this);
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
		// &s_GenericEventFactory,
		&s_GlobalInt32LogicalAndFactory,
		&s_GlobalInt32LogicalOrFactory,
		&s_GlobalInt32LogicalXorFactory,
		&s_GlobalInt32SetFactory,
		&s_GlobalInt32AddFactory,
		&s_GlobalInt32SubtractFactory,
		&s_GlobalInt32ShiftFactory,
		&s_GlobalInt32TestLTEFactory,
		&s_GlobalInt32TestLTFactory,
		&s_GlobalInt32TestEQFactory,
		&s_GlobalInt32TestGTFactory,
		&s_GlobalInt32TestGTEFactory,
		&s_GlobalInt32TestAndFactory,
		&s_GlobalInt32TestNorFactory,
	};
}

namespace HV
{
	ITriggerSupervisor* CreateTrigger()
	{
		return new Trigger();
	}

	IAIBrain* CreateAIBrain(IPublisher& publisher, ISectors& sectors)
	{
		return new AIBrain(publisher, sectors);
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