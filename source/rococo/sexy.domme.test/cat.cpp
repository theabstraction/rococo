#include "cat.h"

#include <rococo.domme.h>
#include <rococo.sexy.api.h>
#include <rococo.ide.h>
#include <sexy.compiler.public.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>
#include <rococo.strings.h>
#include <..\STC\stccore\Sexy.Compiler.h>

#include <stdio.h>

using namespace Rococo::Script;
using namespace Rococo::Sex;
using namespace Rococo::Sexy;

namespace Rococo::Animals::Implementation
{
	// Retrieves a function ref from the module (function CreateDommeObject (ScriptContext context) -> : ... ), 
	// Ensuring the name and arguments are correct
	// The argument is asserted to be a a struct with one member that is an interface field
	const Compiler::IFunction& GetValidDommeObjectEntryPoint(Compiler::IModule& module)
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

	const IArchetype* FindMethod(const IInterface& refInterface, cstr name, int& methodIndex)
	{
		for (int i = 0; i < refInterface.MethodCount(); i++)
		{
			auto& method = refInterface.GetMethod(i);
			if (Eq(method.Name(), name))
			{
				return &method;
			}
		}

		methodIndex = 1;
		return nullptr;
	}

	struct Cat : ICatSupervisor
	{
		ScriptingResources& scripting;

		ISParserTree* sourceTree = nullptr;
		AutoFree<IPublicScriptSystem> ss;

		DommeLog logger;

		struct SexyScriptContext
		{
			InterfacePointer ip{ nullptr };
		} scriptContext;

		const IStructure* concreteType = nullptr;

		const IInterface* interface0 = nullptr;

		const IArchetype* makeBiscuitsMethod = nullptr;

		ID_BYTECODE makeBiscuitsCode;

		Cat(ScriptingResources& _scripting) : scripting(_scripting), logger(scripting)
		{
			sourceTree = scripting.sourceCache.GetSource("!scripts/domme/cat.sxy");

			Compiler::ProgramInitParameters pip;
			ss = scripting.ssFactory.CreateScriptSystem(pip, logger);
			auto* module = ss->AddTree(*sourceTree);

			AddNativeCallSecurity_ToSysNatives(*ss);

			ss->Compile();

			auto& vm = ss->PublicProgramObject().VirtualMachine();

			auto& scriptMain = GetValidDommeObjectEntryPoint(*module);

			Compiler::CodeSection section;
			scriptMain.Code().GetCodeSection(section);

			ss->PublicProgramObject().SetProgramAndEntryPoint(scriptMain);

			vm.Push(&scriptContext);

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

			scriptContext.ip = (InterfacePointer)pNullInterface;

			ExecuteFunctionUntilYield(section.Id, *ss, scripting.debuggerWindow, false);

			ObjectStub& object = *InterfaceToInstance(scriptContext.ip);
			concreteType = object.Desc->TypeInfo;

			if (concreteType->InterfaceCount() == 0)
			{
				Throw(0, "Class %s does not implement interfaces", GetFriendlyName(*concreteType));
			}

			interface0 = &concreteType->GetInterface(0);

			int index = 0;
			makeBiscuitsMethod = FindMethod(*interface0, "MakeBiscuits", OUT index);
			if (!makeBiscuitsMethod)
			{
				Throw(0, "%s: C++ contract requires a method MakeBiscuits");
			}

			if (makeBiscuitsMethod->NumberOfOutputs() != 0)
			{
				Throw(0, "%s: C++ contract requires zero outputs", makeBiscuitsMethod->Name());
			}

			if (makeBiscuitsMethod->NumberOfInputs() != 1)
			{
				Throw(0, "%s: C++ contract requires no inputs", makeBiscuitsMethod->Name());
			}

			auto& vtable0 = *object.pVTables[0];
			ID_BYTECODE* vtableMethods = (ID_BYTECODE*)&vtable0;
			makeBiscuitsCode = vtableMethods[index + 1];
		}

		~Cat()
		{
		}

		void MakeBiscuits() override
		{
			try
			{
				// Todo - required fix - push stack frame and pop frame afterwards
				auto& vm = ss->PublicProgramObject().VirtualMachine();

				vm.Push(scriptContext.ip);
				vm.ExecuteFunctionUntilReturn(makeBiscuitsCode);
				vm.PopPointer();
			}
			catch (...)
			{
				logger.Write("Error invoking " __FUNCTION__);
				throw;
			}
		}

		void Free() override
		{
			auto& vm = ss->PublicProgramObject().VirtualMachine();

			VM::ExecutionFlags executionFlags;
			EXECUTERESULT terminationResult = vm.ContinueExecution(executionFlags, nullptr);
			if (terminationResult != EXECUTERESULT_TERMINATED)
			{
				char msg[256];
				Strings::SafeFormat(msg, "Expected EXECUTERESULT_TERMINATED for %s", sourceTree->Source().Name());
				logger.Write(msg);
			}

			ss->PublicProgramObject().DecrementRefCount(scriptContext.ip);

			delete this;
		}
	};

	ICatSupervisor* CreateCat(ScriptingResources& scripting)
	{
		return new Cat(scripting);
	}
}