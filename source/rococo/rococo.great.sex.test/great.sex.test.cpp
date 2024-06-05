#include <rococo.os.win32.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.strings.h>
#include <stdio.h>

using namespace Rococo;

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNUSED(hInstance);
	UNUSED(hPrevInstance);
	UNUSED(lpCmdLine);
	UNUSED(nCmdShow);

	U8FilePath binPath;
	GetModuleFileNameA(hInstance, binPath.buf, sizeof binPath);
	IO::MakeContainerDirectory(binPath.buf);

	U8FilePath mhostCmdLine;
	Strings::Format(mhostCmdLine, "%s\\rococo.mhost.exe !scripts/mhost/great.sex.test.sxy -debug", binPath.buf);

	STARTUPINFOA startupInfo = { 0 };
	PROCESS_INFORMATION processInfo = { 0 };
	if (!CreateProcessA(NULL, mhostCmdLine.buf, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
	{
		int err = GetLastError();
		MessageBoxA(NULL, "Failed to create process", "Great Sex Test Error", MB_ICONEXCLAMATION);
		return err;		
	}

	return 0;
}