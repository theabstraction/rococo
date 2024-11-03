#pragma once
#include <sexy.types.h>
#include <sexy.compiler.public.h>

#ifndef DOMME_API 
# define DOMME_API ROCOCO_API_EXPORT
#endif

namespace Rococo
{
	DECLARE_ROCOCO_INTERFACE IDebuggerWindow;
	DECLARE_ROCOCO_INTERFACE ISourceCache;
}

namespace Rococo::Script
{
	DECLARE_ROCOCO_INTERFACE IScriptSystemFactory;
}

namespace Rococo::Domme
{
	struct ScriptingResources
	{
		// Script source cache
		ISourceCache& sourceCache;

		// Script debugger window
		IDebuggerWindow& debuggerWindow;

		Rococo::Script::IScriptSystemFactory& ssFactory;
	};

	struct DommeLog : Rococo::ILog
	{
		ScriptingResources& scripting;

		DommeLog(ScriptingResources& _scripting) : scripting(_scripting)
		{

		}

		DOMME_API void Write(cstr text) override;
		DOMME_API void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance) override;
		DOMME_API void OnJITCompileException(Sex::ParseException& ex) override;
	};

	DOMME_API void TerminateScript(Rococo::Compiler::IPublicProgramObject& object, cstr srcName, Rococo::Compiler::InterfacePointer scriptInterface);

	DOMME_API const Compiler::IFunction& GetValidDommeObjectEntryPoint(Rococo::Compiler::IModule& module);

	class DommeObject
	{
		DommeLog logger;
		ScriptingResources& scripting;

		Rococo::Sex::ISParserTree* sourceTree = nullptr;
		AutoFree<Rococo::Script::IPublicScriptSystem> ss;

		struct SexyScriptContext
		{
			Rococo::Compiler::InterfacePointer ip{ nullptr };
		} scriptContext;

		const Rococo::Compiler::IStructure* concreteType = nullptr;
		const Rococo::Compiler::IInterface* interface0 = nullptr;

		Rococo::Compiler::ObjectStub* objectStub = nullptr;

		Rococo::VM::IVirtualMachine* vm;

		cstr scriptInterfaceName;

	public:
		DOMME_API DommeObject(ScriptingResources& _scripting, cstr sourceName, const char* const _namespace, const char* const scriptInterfaceName);
		DOMME_API ~DommeObject();
		DOMME_API int GetMethodIndex(cstr methodName, int expectedInput, int expectedOutputs);
		Rococo::Script::IPublicScriptSystem& SS() { return *ss; }
		Rococo::VM::IVirtualMachine& VM() { return *vm; }
		DOMME_API void CallVirtualMethod(int methodIndex);
		DOMME_API void Push(int32 value);
		DOMME_API void Push(int64 value);
		DOMME_API void PopBytes(size_t nBytes);
		DOMME_API void PrepVM();

		template<class T> void Push(T tValue)
		{
			if (constexpr sizeof T == sizeof int32)
			{
				Push((int32)tValue);
			}

			if (constexpr sizeof T == sizeof int64)
			{
				Push((int64)tValue);
			}
		}
	};
}