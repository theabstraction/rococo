#include "sexy.windows.internal.h"

#ifdef _DEBUG
#pragma comment(lib, "sexy.script.Debug.lib")
#pragma comment(lib, "rococo.sexy.ide.debug.lib")
#pragma comment(lib, "rococo.windows.debug.lib")
#else
#pragma comment(lib, "sexy.script.Release.lib")
#pragma comment(lib, "rococo.sexy.ide.lib")
#pragma comment(lib, "rococo.windows.lib")
#endif

namespace ANON
{
	using namespace Rococo::SexyWindows;

	HINSTANCE g_DLLInstance = nullptr;

	class SexyWindows: public Rococo::SexyWindows::ISexyWindowsSupervisor
	{
		HINSTANCE hResourceInstance;
		HMODULE hRichEditor;
	public:
		SexyWindows(HINSTANCE hResourceInstance)
		{
			hRichEditor = LoadLibraryA(TEXT("Riched20.dll"));
		}

		~SexyWindows()
		{
			FreeLibrary(hRichEditor);
		}

		DialogResult ShowErrorDialog(const ErrorDialogSpec& e)  override
		{
			return Rococo::SexyWindows::ShowErrorDialog(e, g_DLLInstance);
		}

		DialogResult ShowScriptedDialog(const ScriptedDialogSpec& spec) override
		{
			return Rococo::SexyWindows::ShowScriptedDialog(spec, g_DLLInstance);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	namespace SexyWindows
	{
		__declspec(dllexport) ISexyWindowsSupervisor* CreateSexyWindows(void* hResourceInstance)
		{
			return new ANON::SexyWindows((HINSTANCE) hResourceInstance);
		}
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		ANON::g_DLLInstance = hinstDLL;
	}
	return TRUE;
}