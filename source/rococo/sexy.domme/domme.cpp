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

	DOMME_API void TerminateScript(Rococo::Compiler::IPublicProgramObject& object, cstr srcName, Rococo::Script::InterfacePointer scriptInterface)
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
		if (arg.MemberCount() != 1 || arg.VarType() != VARTYPE_Derivative)
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

	DommeObject::DommeObject(ScriptingResources& _scripting, cstr sourceName, const char* const _scriptInterfaceName) : 
		logger(_scripting), scripting(_scripting), scriptInterfaceName(_scriptInterfaceName)
	{
		sourceTree = scripting.sourceCache.GetSource(sourceName);

		Compiler::ProgramInitParameters pip;
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
			Throw(0, "Could not find (namespace Rococo) in %s", sourceTree->Source().Name());
		}

		auto* icat = rococoNS->FindInterface("ICat");
		if (!icat)
		{
			Throw(0, "Could not find interface Rococo.ICat in %s", sourceTree->Source().Name());
		}

		auto* nullIcat = icat->UniversalNullInstance();
		if (!nullIcat)
		{
			Throw(0, "Could not find Rococo.ICat universal null object");
		}

		auto* pNullInterface = GetInterfacePtr(*nullIcat);

		scriptContext.ip = (Rococo::Compiler::InterfacePointer) pNullInterface;

		ExecuteFunctionUntilYield(section.Id, *ss, scripting.debuggerWindow, false);

		objectStub = InterfaceToInstance(scriptContext.ip);
		concreteType = objectStub->Desc->TypeInfo;

		if (concreteType->InterfaceCount() == 0)
		{
			Throw(0, "Class %s does not implement interfaces", Rococo::Compiler::GetFriendlyName(*concreteType));
		}

		interface0 = &concreteType->GetInterface(0);
	}

	void DommeObject::CallVirtualMethod(int methodIndex)
	{
		auto& vtable0 = objectStub->pVTables[0];
		ID_BYTECODE* vtableMethods = (ID_BYTECODE*) vtable0;
		ID_BYTECODE methodAddress = vtableMethods[methodIndex + 1];

		auto& cpu = vm->Cpu();
		cpu.SetSF(cpu.SP());

		vm->Push(scriptContext.ip);
		vm->ExecuteFunctionUntilReturn(methodAddress);
		vm->PopPointer();
	}

	DommeObject::~DommeObject()
	{
		auto& object = ss->PublicProgramObject();
		auto& vm = object.VirtualMachine();

		VM::ExecutionFlags executionFlags;
		EXECUTERESULT terminationResult = vm.ContinueExecution(executionFlags, nullptr);
		if (terminationResult != EXECUTERESULT_TERMINATED)
		{
			char msg[256];
			Strings::SafeFormat(msg, "Expected EXECUTERESULT_TERMINATED for %s", sourceTree->Source().Name());
			object.Log().Write(msg);
		}

		object.DecrementRefCount(scriptContext.ip);
	}

	int DommeObject::GetMethodIndex(cstr methodName, int expectedInput, int expectedOutputs)
	{
		for (int i = 0; i < interface0->MethodCount(); i++)
		{
			auto& method = interface0->GetMethod(i);
			if (Rococo::Strings::Eq(method.Name(), methodName))
			{
				if (method.NumberOfInputs() != expectedInput)
				{
					Throw(0, "method %s.%s of %s had input count %d. C++ equivalent had input count %d", scriptInterfaceName, methodName, sourceTree->Source().Name(), method.NumberOfInputs(), expectedInput);
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
}