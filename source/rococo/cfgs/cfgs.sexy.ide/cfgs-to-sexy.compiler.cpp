#include "cfgs.sexy.ide.h"
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.events.h>
#include <rococo.sexml.h>
#include <rococo.functional.h>
#include <stdio.h>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::SexyStudio;
using namespace Rococo::Strings;
using namespace Rococo::Sex::SEXML;

static void CompileToStringProtected(StringBuilder& sb, ISexyDatabase& db, ICFGSDatabase& cfgs)
{
	sb << "(interface Sys.CFGS.IMyObject\n";

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

	sb << "(class MyObject (implements Sys.CFGS.IMyObject)\n";
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

static bool CompileToString(StringBuilder& sb, ISexyDatabase& db, ICFGSDatabase& cfgs) noexcept
{
	try
	{
		CompileToStringProtected(sb, db, cfgs);
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
	void Compile(ISexyDatabase& db, ICFGSDatabase& cfgs, Strings::IStringPopulator& populator)
	{
		AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(32768);
		auto& sb = dsb->Builder();

		bool success = CompileToString(sb, db, cfgs);

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

struct CLI_Compiler : ICFGSSexyCLI, ICFGSLoader
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

	void Compile(cstr targetFile) override
	{
		if (!targetFile)
		{
			Throw(0, "No target file. Specify -cfgs:<target_file> on command line");
		}

		LoadDatabase(*cfgs, targetFile, *this);

		struct : Strings::IStringPopulator
		{
			void Populate(cstr text) override
			{
				puts(text);
			}
		} onCompile;

		Rococo::CFGS::Compile(DB(), *cfgs, onCompile);
	}

	void Free() override
	{
		delete this;
	}

	void Loader_OnLoadNavigation(const ISEXMLDirective& directive) override
	{
		UNUSED(directive);
	}
};

extern "C" __declspec(dllexport) ICFGSSexyCLI* Create_CFGS_Win32_CLI()
{
	return new CLI_Compiler();
}