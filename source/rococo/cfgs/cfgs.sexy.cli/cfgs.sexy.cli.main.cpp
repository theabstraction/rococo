#include <rococo.os.win32.h>
#include "..\cfgs.sexy.ide\cfgs.sexy.ide.h"
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace MSWindows;

ROCOCO_INTERFACE ICFGSSexyCLI
{
	virtual void Compile(cstr cfgsSexmlFilename) = 0;
	virtual void Free() = 0;
};

typedef ICFGSSexyCLI* (*FN_Create_CFGS_Win32_CLI)();

struct AutoModule
{
	HMODULE hModule;

	~AutoModule()
	{
		FreeLibrary(hModule);
	}
};

#include <stdio.h>

void MainProtected(int argc, char* argv[])
{
	cstr dllname = "cfgs.sexy.ide.dll";

	if (argc == 1)
	{
		printf("Usage:\n\t-compile:<sexml-source> compiles the sexml-source file\n");
		return;
	}

	AutoModule mod;
	mod.hModule = LoadLibraryA(dllname);
	if (!mod.hModule)
	{
		Throw(GetLastError(), "%s: Could not load library: %s", __ROCOCO_FUNCTION__, dllname);
	}

	cstr procName = "Create_CFGS_Win32_CLI";

	FARPROC factoryProc = GetProcAddress(mod.hModule, procName);
	if (!factoryProc)
	{
		Throw(GetLastError(), "%s: Could not load find '%s' in %s", __ROCOCO_FUNCTION__, procName, dllname);
	}

	FN_Create_CFGS_Win32_CLI createCLI = (FN_Create_CFGS_Win32_CLI)factoryProc;

	AutoFree<ICFGSSexyCLI> cli = createCLI();

	for (int i = 0; i < argc; i++)
	{
		cstr arg = argv[i];
		fstring prefix = "-compile:"_fstring;
		if (StartsWith(arg, prefix))
		{
			cli->Compile(arg + prefix.length);
		}
	}
}

int main(int argc, char* argv[])
{
	try
	{
		MainProtected(argc, argv);
		return 0;
	}
	catch (IException& ex)
	{
		printf("%s", ex.Message());
		return ex.ErrorCode();
	}
}