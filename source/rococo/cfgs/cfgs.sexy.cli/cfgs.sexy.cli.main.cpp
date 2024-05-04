#include <rococo.os.win32.h>
#include "..\cfgs.sexy.ide\cfgs.sexy.ide.h"

using namespace Rococo;

ROCOCO_INTERFACE ICFGSSexyCLI
{
	virtual void Compile() = 0;
	virtual void Free() = 0;
};

typedef ICFGSSexyCLI* (*FN_Create_CFGS_Win32_CLI)(int argc, char* argv[]);

int MainProtected(int argc, char* argv[])
{
	cstr dllname = "cfgs.sexy.ide.dll";

	HMODULE hSexyModule = LoadLibraryA(dllname);
	if (!hSexyModule)
	{
		Throw(GetLastError(), "%s: Could not load library: %s", __FUNCTION__, dllname);
	}

	cstr procName = "Create_CFGS_Win32_CLI";

	FARPROC factoryProc = GetProcAddress(hSexyModule, procName);
	if (!factoryProc)
	{
		Throw(GetLastError(), "%s: Could not load find '%s' in %s", __FUNCTION__, procName, dllname);
	}

	FN_Create_CFGS_Win32_CLI createCLI = (FN_Create_CFGS_Win32_CLI)factoryProc;

	AutoFree<ICFGSSexyCLI> cli = createCLI(argc, argv);
	cli->Compile();

	FreeLibrary(hSexyModule);

	return 0;
}

#include <stdio.h>

int main(int argc, char* argv[])
{
	try
	{
		return MainProtected(argc, argv);
	}
	catch (IException& ex)
	{
		printf("%s", ex.Message());
	}
}