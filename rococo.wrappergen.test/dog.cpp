#include <rococo.sexy.api.h>
#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>

namespace Rococo::Script
{
	using namespace Rococo::Compiler;
	using namespace Rococo::VM;

	ROCOCOAPI ISexyScriptClasses
	{
		virtual IPublicScriptSystem* CreateScript(cstr filename) = 0;
		virtual EXECUTERESULT ExecuteFunction(IVirtualMachine& vm) = 0;
		virtual const IFunction* GetEntryFunction(IPublicScriptSystem& ss, cstr functionName) = 0;	
		virtual IVirtualMachine* SetProgramAndEntryPoint(IPublicScriptSystem& ss, const IFunction& function) = 0;
	};

	void validate(const IFunction* function, ISexyScriptClasses& ssc);
	void validate(InterfacePointer ptr, ISexyScriptClasses& ssc);

	void ExecuteMethod(IVirtualMachine& vm, ISexyScriptClasses& ssc, InterfacePointer pInstance, int methodIndex);
	void ReleaseSexyInterface(InterfacePointer ptr, ISexyScriptClasses& ssc);
}

#include "dog_sxy.inl"