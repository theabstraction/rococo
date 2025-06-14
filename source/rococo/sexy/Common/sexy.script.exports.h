#pragma once
#include <sexy.script.h>

#ifndef SCRIPTEXPORT_API
# error "define SCRIPTEXPORT_API in the compiler environment"
#endif

namespace Rococo::Script
{
#pragma pack(push, 1)
	struct CClassSysTypeStringBuilder
	{
		Compiler::ObjectStub header;
		int32 length;
		char* buffer;
		int32 capacity;

		SCRIPTEXPORT_API void AppendAndTruncate(const fstring& text);
	};
#pragma pack(pop)

	SCRIPTEXPORT_API bool IsIString(const Rococo::Compiler::IInterface& i);
	SCRIPTEXPORT_API bool IsIString(const Rococo::Compiler::IStructure& typeDesc);
	SCRIPTEXPORT_API void SetDefaultNativeSourcePath(crwstr pathname);
	SCRIPTEXPORT_API const Rococo::Sex::ISExpression* GetSexSymbol(VM::CPU& cpu, const uint8* pcAddress, Rococo::Script::IPublicScriptSystem& ss);
	SCRIPTEXPORT_API const Rococo::Compiler::IFunction* GetFunctionFromBytecode(const Rococo::Compiler::IModule& module, Rococo::ID_BYTECODE id);
	SCRIPTEXPORT_API const Rococo::Compiler::IFunction* GetFunctionFromBytecode(Rococo::Compiler::IPublicProgramObject& obj, Rococo::ID_BYTECODE id);
	SCRIPTEXPORT_API const Rococo::Compiler::IFunction* GetFunctionAtAddress(Rococo::Compiler::IPublicProgramObject& po, size_t pcOffset);
	SCRIPTEXPORT_API const uint8* GetCallerSF(Rococo::VM::CPU& cpu, const uint8* sf);
	SCRIPTEXPORT_API const uint8* GetReturnAddress(Rococo::VM::CPU& cpu, const uint8* sf);
	SCRIPTEXPORT_API const uint8* GetPCAddress(Rococo::VM::CPU& cpu, int32 callDepth);
	SCRIPTEXPORT_API const uint8* GetStackFrame(Rococo::VM::CPU& cpu, int32 callDepth);
	SCRIPTEXPORT_API bool GetVariableByIndex(cstr& name, Rococo::Compiler::MemberDef& def, const Rococo::Compiler::IStructure*& pseudoType, const uint8*& SF, IPublicScriptSystem& ss, size_t index, size_t callOffset);
	SCRIPTEXPORT_API bool GetCallDescription(const uint8*& sf, const uint8*& pc, const Rococo::Compiler::IFunction*& f, size_t& fnOffset, IPublicScriptSystem& ss, size_t callDepth, size_t& pcOffset);
	SCRIPTEXPORT_API size_t GetCurrentVariableCount(IPublicScriptSystem& ss, size_t callDepth);
	SCRIPTEXPORT_API void ForeachStackLevel(Rococo::Compiler::IPublicProgramObject& obj, Rococo::Debugger::ICallStackEnumerationCallback& cb);
	SCRIPTEXPORT_API void ForeachVariable(Rococo::Script::IPublicScriptSystem& ss, Rococo::Debugger::IVariableEnumeratorCallback& variableEnum, size_t callOffset);
	SCRIPTEXPORT_API void FormatValue(IPublicScriptSystem& ss, char* buffer, size_t bufferCapacity, SexyVarType type, const void* pVariableData);
	SCRIPTEXPORT_API void SkipJIT(Rococo::Compiler::IPublicProgramObject& po);
	SCRIPTEXPORT_API bool GetMembers(IPublicScriptSystem& ss, const Rococo::Compiler::IStructure& s, cstr parentName, const uint8* instance, ptrdiff_t offset, MemberEnumeratorCallback& enumCallback, int recurseDepth);
	SCRIPTEXPORT_API const Rococo::uint8* GetInstance(const Rococo::Compiler::MemberDef& def, const Rococo::Compiler::IStructure* pseudoType, const uint8* SF);
	SCRIPTEXPORT_API cstr GetShortName(const Rococo::Compiler::IStructure& s);
	SCRIPTEXPORT_API cstr GetInstanceTypeName(const Rococo::Compiler::MemberDef& def, const Rococo::Compiler::IStructure* pseudoType);
	SCRIPTEXPORT_API cstr GetInstanceVarName(cstr name, const Rococo::Compiler::IStructure* pseudoType);
	SCRIPTEXPORT_API bool FindVariableByName(Rococo::Compiler::MemberDef& def, const Rococo::Compiler::IStructure*& pseudoType, const Rococo::uint8*& SF, IPublicScriptSystem& ss, cstr searchName, size_t callOffset);
	SCRIPTEXPORT_API const Rococo::Compiler::IStructure* FindStructure(IPublicScriptSystem& ss, cstr fullyQualifiedName);
	SCRIPTEXPORT_API void AddNativeCallSecurity(IPublicScriptSystem& ss, cstr nativeNamespace, cstr permittedPingPath);
}

namespace Rococo::Script
{
	class CScriptSystemProxy
	{
	private:
		IScriptSystemFactory* factory;
		IPublicScriptSystem* ss;

	public:
		IPublicScriptSystem& operator()() { return *ss; }

		SCRIPTEXPORT_API CScriptSystemProxy(const Rococo::Compiler::ProgramInitParameters& pip, ILog& logger);
		SCRIPTEXPORT_API ~CScriptSystemProxy();
	};
}

#ifndef THIS_IS_THE_SEXY_CORE_LIBRARY
// Ensure the allocator used for CreateScriptV_1_4_0_0(...) is in scope when you call Sexy_CleanupGlobalSources to clean up global resources
extern "C" SCRIPTEXPORT_API Rococo::Script::IScriptSystemFactory* CreateScriptSystemFactory_1_5_0_0();

#endif