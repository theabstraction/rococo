#include "cat.h"

#include <rococo.domme.h>
#include <rococo.sexy.api.h>
#include <rococo.ide.h>
#include <sexy.compiler.public.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>
#include <rococo.strings.h>
#include <..\STC\stccore\Sexy.Compiler.h>

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

	ICatSupervisor* CreateCat(ScriptingResources& scripting)
	{
		struct Cat: ICatSupervisor
		{
			ScriptingResources& scripting;

			ISParserTree* sourceTree = nullptr;
			AutoFree<IPublicScriptSystem> ss;

			DommeLog logger;

			struct SexyScriptContext
			{
				InterfacePointer ip{ nullptr };
			} scriptContext;
			
			Cat(ScriptingResources& _scripting): scripting(_scripting), logger(scripting)
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

				scriptContext.ip = (InterfacePointer) pNullInterface;

				ExecuteFunctionUntilYield(section.Id, *ss, scripting.debuggerWindow, false);
			}

			~Cat()
			{
			}

			void MakeBiscuits() override
			{

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

		return new Cat(scripting);
	}
}