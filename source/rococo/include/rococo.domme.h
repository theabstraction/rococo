// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
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
		const Rococo::Compiler::IObjectInterface* interface0 = nullptr;

		Rococo::Compiler::ObjectStub* objectStub = nullptr;

		Rococo::VM::IVirtualMachine* vm;

		cstr scriptInterfaceName;

	public:
		DOMME_API DommeObject(ScriptingResources& _scripting, cstr sourceName, const char* const _namespace, const char* const scriptInterfaceName);
		DOMME_API ~DommeObject();
		DOMME_API void Terminate();
		DOMME_API int GetMethodIndex(cstr methodName, int expectedInputs, int expectedOutputs);
		Rococo::Script::IPublicScriptSystem& SS() { return *ss; }
		Rococo::VM::IVirtualMachine& VM() { return *vm; }
		DOMME_API void CallVirtualMethod(int methodIndex);
		DOMME_API void Push(int32 value);
		DOMME_API void Push(int64 value);
		DOMME_API void PushPtr(void* ptr);
		DOMME_API void PopBytes(size_t nBytes);
		DOMME_API void PrepVM();

		template<class T>
		void PushRef(T& t)
		{
			PushPtr((void*) &t);
		}

		void Push(float tValue)
		{
			int* pF = (int*)&tValue;
			Push(*pF);
		}

		DOMME_API boolean32 Pop_boolean32();

		DOMME_API Rococo::Compiler::InterfacePointer MarshalString(fstring s);
	};

	class CallContext
	{
	private:
		DommeObject& obj;

		const uint8* sp = nullptr;
		const uint8* sf = nullptr;
	public:
		DOMME_API CallContext(DommeObject& _obj);
		DOMME_API ~CallContext();
		DOMME_API void ValidateRegisters(cstr caller, cstr file, int lineNumber);
	};

#ifdef _DEBUG
# define REGISTER_DOMME_CALL(x) CallContext register_CC(x)
# define VALIDATE_REGISTERS register_CC.ValidateRegisters(__ROCOCO_FUNCTION__, __FILE__, __LINE__);
#else
# define REGISTER_DOMME_CALL(x) 
# define VALIDATE_REGISTERS
#endif

	template<class T, class U>
	inline T To(const U& u)
	{
		BadConversion();
	}

	template<>
	inline bool To(const boolean32& value)
	{
		return value == 0 ? false : true;
	}
}

#define DECLARE_DOMME_INTERFACE(FQ_INTERFACE_NAME)	\
namespace Rococo									\
{													\
	template<class T> struct Deallocator;			\
													\
	template<>										\
	struct Deallocator<FQ_INTERFACE_NAME>			\
	{												\
		void operator()(FQ_INTERFACE_NAME* t)		\
		{											\
			if (t) t->SV_Free();					\
		}											\
	};												\
}

