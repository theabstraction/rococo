#include <rococo.api.h>
#include <rococo.sexy.api.h>
#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>

#include <rococo.hashtable.h>

namespace Rococo::Script
{
	using namespace Rococo::Compiler;
	using namespace Rococo::VM;

	ROCOCO_INTERFACE ISexyScriptClasses
	{
		virtual IPublicScriptSystem* CreateScript(cstr filename) = 0;
		virtual EXECUTERESULT ExecuteFunction(IVirtualMachine& vm) = 0;
		virtual const IFunction* GetEntryFunction(IPublicScriptSystem& ss, cstr functionName) = 0;	
		virtual IVirtualMachine* SetProgramAndEntryPoint(IPublicScriptSystem& ss, const IFunction& function) = 0;
	};

	void validate(const IFunction* function, ISexyScriptClasses& ssc)
	{

	}

	void validate(InterfacePointer ptr, ISexyScriptClasses& ssc)
	{

	}

	void ExecuteMethod(IVirtualMachine& vm, ISexyScriptClasses& ssc, InterfacePointer pInstance, int methodIndex)
	{

	}

	void ReleaseSexyInterface(InterfacePointer ptr, ISexyScriptClasses& ssc)
	{

	}

	typedef void* (*FN_CREATE_OBJECT)(ISexyScriptClasses& ssc);

	struct CreateObjectContext
	{
		FN_CREATE_OBJECT fnCreateObject;
		HString interfaceName;
	};

	static stringmap<CreateObjectContext> fnCreateFunctions;

	void AddGlobalCreateFunction(cstr key, FN_CREATE_OBJECT createObject, cstr interfaceName)
	{
		fnCreateFunctions.insert(key, CreateObjectContext { createObject, interfaceName });
		static int x = 0;
		x++;
	}

	template<class CLASSOBJECT>
	class RegisterWrapper
	{
	public:
		RegisterWrapper(cstr key)
		{
			AddGlobalCreateFunction(key, (FN_CREATE_OBJECT)CLASSOBJECT::CreateObject, CLASSOBJECT::InterfaceName());
		}
	};

	enum class EWrapperCode
	{
		NoError = 0,
		UrlNull = 1,
		UrlUnknown = 2,
		InterfaceNotSupported = 3,
		NoInstanceReturned = 4
	};

	template<class INTERFACE>
	INTERFACE* CreateWrapperObject(EWrapperCode& outErrorCode, cstr url, ISexyScriptClasses& ssc)
	{
		if (url == nullptr || *url == 0)
		{
			outErrorCode = EWrapperCode::UrlNull;
			return nullptr;
		}

		auto i = fnCreateFunctions.find(url);
		if (i == fnCreateFunctions.end())
		{
			outErrorCode = EWrapperCode::UrlUnknown;
			return nullptr;
		}

		auto& context = i->second;

		if (!Eq(context.interfaceName, INTERFACE::InterfaceName()))
		{
			outErrorCode = EWrapperCode::InterfaceNotSupported;
			return nullptr;
		}

		void* instance = context.fnCreateObject(ssc);
		outErrorCode = instance ? EWrapperCode::NoError : EWrapperCode::NoInstanceReturned;
		return reinterpret_cast<INTERFACE*>(instance);
	}

	struct SexyScriptClasses : ISexyScriptClasses
	{
		IPublicScriptSystem* CreateScript(cstr filename) override
		{
			return nullptr;
		}

		EXECUTERESULT ExecuteFunction(IVirtualMachine& vm) override
		{
			return EXECUTERESULT_RETURNED;
		}

		const IFunction* GetEntryFunction(IPublicScriptSystem& ss, cstr functionName) override
		{
			return nullptr;
		}

		IVirtualMachine* SetProgramAndEntryPoint(IPublicScriptSystem& ss, const IFunction& function)
		{
			return nullptr;
		}
	};
}

#include "dog_sxy.inl"

void test()
{
	using namespace Rococo::Script;

	SexyScriptClasses ssc;

	EWrapperCode err;
	auto* instance = CreateWrapperObject<Rococo::WrapperTest::IDogSupervisor>(err, R"(sxyobject\dog.sxy)", ssc);
	instance->Free();
}