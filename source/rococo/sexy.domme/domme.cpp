#include <rococo.types.h>
#define DOMME_API ROCOCO_API_EXPORT
#include <rococo.domme.h>
#include <rococo.ide.h>

#include <sexy.compiler.public.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>
#include <rococo.strings.h>
#include <..\STC\stccore\Sexy.Compiler.h>
#include <sexy.compiler.public.h>
#include <rococo.sexy.api.h>

namespace Rococo::Domme
{
	DOMME_API void DommeLog::Write(cstr text)
	{
		scripting.debuggerWindow.Log("%s", text);
	}

	DOMME_API void DommeLog::OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance)
	{
		UNUSED(exceptionInstance);

		if (errorCode)
		{
			scripting.debuggerWindow.Log("Unhandled exception: %s: %s\n", exceptionType, message);
		}
		else
		{
			scripting.debuggerWindow.Log("Unhandled exception: (Code %d) %s: %s\n", errorCode, exceptionType, message);
		}
	}

	DOMME_API void DommeLog::OnJITCompileException(Sex::ParseException& ex)
	{
		cstr sourceName = ex.Source()->Tree().Source().Name();
		scripting.debuggerWindow.Log("JIT exception: (Code %d) %s: %s. Line %d pos %d\n", ex.ErrorCode(), sourceName, ex.Message(), ex.Start().y, ex.Start().x);
	}

	DOMME_API void TerminateScript(Rococo::Compiler::IPublicProgramObject& object, cstr srcName, Compiler::InterfacePointer scriptInterface)
	{
		auto& vm = object.VirtualMachine();

		VM::ExecutionFlags executionFlags;
		EXECUTERESULT terminationResult = vm.ContinueExecution(executionFlags, nullptr);
		if (terminationResult != EXECUTERESULT_TERMINATED)
		{
			char msg[256];
			Strings::SafeFormat(msg, "Expected EXECUTERESULT_TERMINATED for %s", srcName);
			object.Log().Write(msg);
		}

		object.DecrementRefCount(scriptInterface);
	}

	DOMME_API const Compiler::IFunction& GetValidDommeObjectEntryPoint(Rococo::Compiler::IModule& module)
	{
		auto* entryPoint = module.FindFunction("CreateDommeObject");
		if (!entryPoint)
		{
			Throw(0, "no entry point (function CreateDommeObject ...)");
		}

		auto& s = *reinterpret_cast<const Sex::ISExpression*>(entryPoint->Definition());

		if (entryPoint->NumberOfInputs() != 1)
		{
			Throw(s, "require one and only one input argument to CreateDommeObject");
		}

		if (entryPoint->NumberOfOutputs() != 0)
		{

			Throw(s, "require zero output arguments to CreateDommeObject");
		}

		auto& arg = entryPoint->GetArgument(0);
		if (arg.MemberCount() != 1 || arg.VarType() != SexyVarType_Derivative)
		{
			Throw(s, "expecting CreateDommeObject to take an argument that is a struct with one element");
		}

		auto& argDef = *reinterpret_cast<const Sex::ISExpression*>(arg.Definition());

		auto& m = arg.GetMember(0);

		if (!m.IsInterfaceVariable())
		{
			Throw(argDef, "expecting interface variable for member %s of %s", m.Name(), arg.Name());
		}

		return *entryPoint;
	}

	DommeObject::DommeObject(ScriptingResources& _scripting, cstr sourceName, const char* const _namespace, const char* const _scriptInterfaceName) :
		logger(_scripting), scripting(_scripting), scriptInterfaceName(_scriptInterfaceName)
	{
		sourceTree = scripting.sourceCache.GetSource(sourceName);

		Compiler::ProgramInitParameters pip;
		pip.addIO = true;
		ss = scripting.ssFactory.CreateScriptSystem(pip, logger);
		auto* module = ss->AddTree(*sourceTree);

		Rococo::Script::AddNativeCallSecurity_ToSysNatives(*ss);

		ss->Compile();

		vm = &ss->PublicProgramObject().VirtualMachine();

		auto& scriptMain = GetValidDommeObjectEntryPoint(*module);

		Compiler::CodeSection section;
		scriptMain.Code().GetCodeSection(section);

		ss->PublicProgramObject().SetProgramAndEntryPoint(scriptMain);

		vm->Push(&scriptContext);

		auto& rootNS = ss->PublicProgramObject().GetRootNamespace();
		auto* rococoNS = rootNS.FindSubspace("Rococo");
		if (!rococoNS)
		{
			Throw(0, "%s: Could not find (namespace %s) in %s", __ROCOCO_FUNCTION__, _namespace, sourceTree->Source().Name());
		}

		auto* iRef = rococoNS->FindInterface(_scriptInterfaceName);
		if (!iRef)
		{
			Throw(0, "%s: Could not find interface %s.%s in %s", __ROCOCO_FUNCTION__, _namespace, _scriptInterfaceName, sourceTree->Source().Name());
		}

		auto* nullIRef= iRef->UniversalNullInstance();
		if (!nullIRef)
		{
			Throw(0, "%s: Could not find %s.%s universal null object", __ROCOCO_FUNCTION__, _namespace, _scriptInterfaceName);
		}

		auto* pNullInterface = GetInterfacePtr(*nullIRef);

		scriptContext.ip = (Rococo::Compiler::InterfacePointer) pNullInterface;

		ExecuteFunctionUntilYield(section.Id, *ss, scripting.debuggerWindow, false);

		objectStub = InterfaceToInstance(scriptContext.ip);
		concreteType = objectStub->Desc->TypeInfo;

		if (concreteType->InterfaceCount() == 0)
		{
			Throw(0, "%s: Class %s does not implement interfaces", __ROCOCO_FUNCTION__, Rococo::Compiler::GetFriendlyName(*concreteType));
		}

		interface0 = &concreteType->GetInterface(0);
	}

	void DommeObject::CallVirtualMethod(int methodIndex)
	{
		auto& vtable0 = objectStub->pVTables[0];
		ID_BYTECODE* vtableMethods = (ID_BYTECODE*) vtable0;
		ID_BYTECODE methodAddress = vtableMethods[methodIndex + 1];

#ifdef _DEBUG
		if (!vm) Throw(0, "%s: No virtual machine. Script was Terminated", __ROCOCO_FUNCTION__);
#endif

		vm->Push(scriptContext.ip);
		vm->ExecuteFunctionUntilReturn(methodAddress);
		vm->PopPointer();
	}

	void DommeObject::Push(int32 value)
	{
#ifdef _DEBUG
		if (!vm) Throw(0, "%s: No virtual machine. Script was Terminated", __ROCOCO_FUNCTION__);
#endif
		vm->Push(value);
	}

	void DommeObject::Push(int64 value)
	{
#ifdef _DEBUG
		if (!vm) Throw(0, "%s: No virtual machine. Script was previously Terminated", __ROCOCO_FUNCTION__);
#endif
		vm->Push(value);
	}

	void DommeObject::PushPtr(void* ptr)
	{
#ifdef _DEBUG
		if (!vm) Throw(0, "%s: No virtual machine. Script was previously Terminated", __ROCOCO_FUNCTION__);
#endif
		vm->Push(ptr);
	}

	void DommeObject::PopBytes(size_t nBytes)
	{
#ifdef _DEBUG
		if (!vm) Throw(0, "%s: No virtual machine. Script was previously Terminated", __ROCOCO_FUNCTION__);
#endif
		auto& cpu = vm->Cpu();
		cpu.D[VM::REGISTER_SP].uint8PtrValue -= nBytes;
	}

	DommeObject::~DommeObject()
	{
	}

	void DommeObject::Terminate()
	{
#ifdef _DEBUG
		if (!vm) Throw(0, "%s: No virtual machine. Script was previously Terminated", __ROCOCO_FUNCTION__);
#endif
		auto& object = ss->PublicProgramObject();

		VM::ExecutionFlags executionFlags;
		EXECUTERESULT terminationResult = vm->ContinueExecution(executionFlags, nullptr);
		if (terminationResult != EXECUTERESULT_TERMINATED)
		{
			char msg[256];
			Strings::SafeFormat(msg, "Expected EXECUTERESULT_TERMINATED for %s", sourceTree->Source().Name());
			object.Log().Write(msg);
		}

		object.DecrementRefCount(scriptContext.ip);

		vm = nullptr;
	}

	int DommeObject::GetMethodIndex(cstr methodName, int expectedInputs, int expectedOutputs)
	{
		for (int i = 0; i < interface0->MethodCount(); i++)
		{
			auto& method = interface0->GetMethod(i);
			if (Rococo::Strings::Eq(method.Name(), methodName))
			{
				if (method.NumberOfInputs() != expectedInputs + 1)
				{
					Throw(0, "method %s.%s of %s had input count %d. C++ equivalent had input count %d", scriptInterfaceName, methodName, sourceTree->Source().Name(), method.NumberOfInputs(), expectedInputs);
				}

				if (method.NumberOfOutputs() != expectedOutputs)
				{
					Throw(0, "method %s.%s of %s had input count %d. C++ equivalent had input count %d", scriptInterfaceName, methodName, sourceTree->Source().Name(), method.NumberOfOutputs(), expectedOutputs);
				}

				return i;
			}
		}

		Throw(0, "No such method %s.%s of %s", scriptInterfaceName, methodName, sourceTree->Source().Name());
	}

	DOMME_API boolean32 DommeObject::Pop_boolean32()
	{
		return (boolean32) vm->PopInt32();
	}

	DOMME_API Rococo::Compiler::InterfacePointer DommeObject::MarshalString(fstring s)
	{
		auto* sc = ss->ReflectImmutableStringPointer(s.buffer, s.length);
		return sc->header.AddressOfVTable0();
	}

	CallContext::CallContext(DommeObject& _obj): obj(_obj)
	{
		auto& cpu = _obj.VM().Cpu();
		sp = cpu.SP();
		sf = cpu.SF();
	}

	CallContext::~CallContext()
	{
	}

	void CallContext::ValidateRegisters(cstr caller, cstr file, int lineNumber)
	{
		auto& cpu = obj.VM().Cpu();

		if (sp != cpu.SP())
		{
			Throw(0, "CallContext::ValidateRegisters failed: sp: %p, cpu.sp: %p, delta %lld [%s: %s line %d]", sp, cpu.SP(), cpu.SP() - sp, caller, file, lineNumber);
		}

		if (sf != cpu.SF())
		{
			Throw(0, "CallContext::ValidateRegisters failed: sf: %p, cpu.sf: %p, delta %lld [%s: %s line %d]", sf, cpu.SF(), cpu.SF() - sf, caller, file, lineNumber);
		}
	}
}