// mhost.os.win32.cpp : Defines the entry point for the application in Windows.
#include <rococo.mplat.dynamic.inl>
#include <rococo.strings.h>
#include <rococo.allocators.h>

#include "resource.h"

using namespace Rococo;

namespace MHost
{
	IDirectApp* CreateApp(Platform& platform, IDirectAppControl& control, cstr commandLine);

	namespace UI
	{
		void CaptureMouse(Rococo::Windows::IWindow& window)
		{
			::SetCapture(window);
		}

		void ReleaseMouse()
		{
			::ReleaseCapture();
		}
	}
}

namespace MHost
{
	void AddMHostNativeCallSecurity(ScriptCompileArgs& args);
}

int WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine,_In_ int nShowCmd)
{
	UNUSED(hPrevInstance);
	UNUSED(nShowCmd);

	Rococo::OS::SetBreakPoints(Rococo::OS::Flags::BreakFlag_All);

	char titleBuffer[128];
	Rococo::Strings::CLI::GetCommandLineArgument("-title:"_fstring, lpCmdLine, titleBuffer, sizeof titleBuffer, "Rococo MHost");

	using namespace Rococo::Memory;
	
	AllocatorLogFlags flags;
	flags.LogDetailedMetrics = true;
	flags.LogLeaks = true;
	flags.LogOnModuleExit = true;
	Rococo::Memory::SetAllocatorLogFlags(flags);

	struct : IDirectAppFactory, IScriptCompilationEventHandler
	{
		IDirectApp* CreateApp(Platform& e, IDirectAppControl& control) override
		{
			return MHost::CreateApp(e, control, GetCommandLineA());
		}

		void OnCompile(ScriptCompileArgs& args) override
		{
			MHost::AddMHostNativeCallSecurity(args);
		}

		IScriptEnumerator* ImplicitIncludes() override
		{
			return nullptr;
		}
	} factory;

	return LoadPlatformDll_AndRun(hInstance, factory, titleBuffer, MPLAT_DLL, nullptr, nullptr);
}