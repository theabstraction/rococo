#include "sexy.windows.internal.h"

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