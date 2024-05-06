#include "cfgs.sexy.ide.h"
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.events.h>
#include <rococo.sexml.h>
#include <rococo.functional.h>
#include <..\sexystudio\sexystudio.api.h>
#include <stdio.h>
#include <rococo.hashtable.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::SexyStudio;
using namespace Rococo::Strings;
using namespace Rococo::Sex::SEXML;

static Rococo::stringmap<int> knownPrimitives;


cstr GetCorrectedDefaultValue(const ISXYType* type, cstr defaultValue)
{
	if (!type)
	{
		return nullptr;
	}

	if (knownPrimitives.empty())
	{
		knownPrimitives["Float32"] = 0;
		knownPrimitives["Float64"] = 0;
		knownPrimitives["Int32"] = 0;
		knownPrimitives["Int64"] = 0;
	}

	auto* localType = type->LocalType();

	if (localType && localType->FieldCount() == 1)
	{
		if (knownPrimitives.find(localType->GetField(0).type) != knownPrimitives.end())
		{
			return defaultValue;
		}
		else if (Eq(localType->GetField(0).type, "Bool"))
		{
			if (Eq(defaultValue, "0"))
			{
				return "false";
			}

			if (Eq(defaultValue, "1"))
			{
				return "true";
			}

			if (Eq(defaultValue, "true") || Eq(defaultValue, "false"))
			{
				return defaultValue;
			}
		}
	}

	return nullptr;
}

static void CompileToStringProtected(StringBuilder& sb, ISexyDatabase& db, ICFGSDatabase& cfgs, ICompileExportsSpec& exportSpec, ICFGSVariableEnumerator& variables)
{
	sb.AppendFormat("(interface %s\n", exportSpec.InterfaceName());

	cfgs.ForEachFunction(
		[&sb](ICFGSFunction& f)
		{
			cstr name = f.Name();
			if (name[0] != '_')
			{
				// Public method
				sb.AppendFormat("  (%s -> )\n", name);
			}
		}
	);

	sb << ")\n\n";

	sb.AppendFormat("(class %s (implements %s)\n", exportSpec.InterfaceName(), exportSpec.ClassName());

	variables.ForEachVariable(
		[&sb, &db](cstr name, cstr type, cstr defaultValue)
		{
			const ISXYType* sxyType = db.FindPrimitiveOrFQType(type);
			cstr correctedDefaultValue = GetCorrectedDefaultValue(sxyType, defaultValue);
			if (correctedDefaultValue)
			{
				sb.AppendFormat("  (%s %s = %s)\n", type, name, correctedDefaultValue);
			}
			else
			{
				sb.AppendFormat("  (%s %s)\n", type, name);
			}
		}
	);


	sb << ")\n";


	cfgs.ForEachFunction(
		[&sb](ICFGSFunction& f)
		{
			cstr name = f.Name();
			if (name[0] == '_') name++;

			// Public method
			sb.AppendFormat("\n(method MyObject.%s -> : \n", name);			
			sb.AppendFormat(")\n");
		}
	);

	UNUSED(db);
}

static bool CompileToString(StringBuilder& sb, ISexyDatabase& db, ICFGSDatabase& cfgs, ICompileExportsSpec& exportSpec, ICFGSVariableEnumerator& variables) noexcept
{
	try
	{
		CompileToStringProtected(sb, db, cfgs, exportSpec, variables);
		return true;
	}
	catch (IException& ex)
	{
		char buffer[2048];
		OS::BuildExceptionString(buffer, sizeof buffer, ex, true);
		sb << "Exception thrown compiling CFGS graphs:" << buffer;
	}
	catch (...)
	{
		sb << "Unspecified exception thrown compiling CFGS graphs:";
	}

	return false;
}

namespace Rococo::CFGS
{
	void Compile(ISexyDatabase& db, ICFGSDatabase& cfgs, Strings::IStringPopulator& populator, ICompileExportsSpec& exportSpec, ICFGSVariableEnumerator& variables)
	{
		AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(32768);
		auto& sb = dsb->Builder();

		bool success = CompileToString(sb, db, cfgs, exportSpec, variables);

		populator.Populate(*sb);

		if (!success)
		{
			Throw(0, "Error compiling cfgs into sexy. Check file for more information");
		}
	}
}

#include <rococo.os.win32.h>
#include <rococo.sexystudio.api.h>

struct SexyIDEWindow : ISexyStudioEventHandler
{
	AutoFree<Rococo::SexyStudio::ISexyStudioInstance1> ideInstance;

	SexyIDEWindow()
	{

	}

	void Create(ISexyStudioFactory1& factory)
	{
		ideInstance = factory.CreateSexyIDE(Windows::NoParent(), *this);
	}

	bool TryOpenEditor(cstr filePath, int lineNumber) override
	{
		Rococo::OS::ShellOpenDocument(ideInstance->GetIDEFrame(), "CFGS SexyStudio IDE. Open file... ", filePath, lineNumber);
		return true;
	}

	EIDECloseResponse OnIDEClose(IWindow&) override
	{
		return EIDECloseResponse::Shutdown;
	}
};

namespace Rococo::CFGS
{
	ROCOCO_API_IMPORT void LoadDatabase(ICFGSDatabase& db, cstr filename, ICFGSLoader& loader);
}

struct CLI_Compiler : ICFGSSexyCLI, ICFGSLoader, ICFGSVariableEnumerator
{
	AutoFree<Rococo::Events::IPublisherSupervisor> publisher;
	AutoFree<ICFGSDatabaseSupervisor> cfgs;
	AutoFree<ISexyStudioFactory1> ssFactory;
	SexyIDEWindow ideWindow;

	CLI_Compiler()
	{
		publisher = Events::CreatePublisher();

		HMODULE hSexyStudio = LoadLibraryA("sexystudio.dll");
		if (!hSexyStudio)
		{
			Throw(GetLastError(), "%s: failed to load sexystudio.dll", __FUNCTION__);
		}

		auto CreateSexyStudioFactory = (FN_CreateSexyStudioFactory)GetProcAddress(hSexyStudio, "CreateSexyStudioFactory");
		if (!CreateSexyStudioFactory)
		{
			Throw(GetLastError(), "%s: failed to find proc CreateSexyStudioFactory in sexystudio.dll", __FUNCTION__);
		}

		cstr interfaceURL = "Rococo.SexyStudio.ISexyStudioFactory1";

		int nErr = CreateSexyStudioFactory((void**)&ssFactory, interfaceURL);
		if FAILED(nErr)
		{
			Throw(nErr, "CreateSexyStudioFactory with URL %s failed", interfaceURL);
		}

		ideWindow.Create(*ssFactory);

		cfgs = CreateCFGSDatabase(*publisher);
	}

	ISexyDatabase& DB()
	{
		return ideWindow.ideInstance->GetDatabase();
	}

	void ForEachVariable(Rococo::Function<void(Rococo::cstr name, Rococo::cstr type, Rococo::cstr defaultValue)> callback)
	{
		for (auto& v : variables)
		{
			callback.Invoke(v.first, v.second.type, v.second.defaultValue);
		}
	}

	void Compile(cstr targetFile) override
	{
		if (!targetFile)
		{
			Throw(0, "No target file. Specify -cfgs:<target_file> on command line");
		}

		struct : Strings::IStringPopulator
		{
			void Populate(cstr text) override
			{
				puts(text);
			}
		} onCompile;

		struct spec : ICompileExportsSpec
		{
			cstr InterfaceName() const override
			{
				return "Sys.CFGS.ITest";
			}

			cstr FactoryName() const override
			{
				return "Sys.CFGS.NewTest";
			}

			cstr ClassName() const override
			{
				return "Test";
			}
		} exportSpec;

		try
		{
			LoadDatabase(*cfgs, targetFile, *this);
			Rococo::CFGS::Compile(DB(), *cfgs, onCompile, exportSpec, *this);
		}
		catch (ParseException& ex)
		{
			Throw(ex.ErrorCode(), "%s: parse error at line %d pos %d.\n\t%s", targetFile, ex.Start().y, ex.Start().x, ex.Message());
		}
	}

	void Free() override
	{
		delete this;
	}

	struct VariableDef
	{
		HString type;
		HString defaultValue;
	};

	stringmap<VariableDef> variables;

	void Loader_OnLoadNavigation(const ISEXMLDirective& navDirective) override
	{
		size_t startIndex = 0;
		const auto* varDirective = navDirective.FindFirstChild(REF startIndex, "Variables");
		if (varDirective)
		{
			auto& variableList = varDirective->Children();

			for (size_t i = 0; i < variableList.NumberOfDirectives(); i++)
			{
				auto& variable = variableList[i];

				cstr name = SEXML::AsAtomic(variable["Name"]).c_str();
				cstr type = SEXML::AsAtomic(variable["Type"]).c_str();
				cstr defaultValue = SEXML::AsAtomic(variable["Default"]).c_str();

				variables.insert(name, VariableDef{  type, defaultValue });
			}
		}
	}
};

extern "C" __declspec(dllexport) ICFGSSexyCLI* Create_CFGS_Win32_CLI()
{
	return new CLI_Compiler();
}