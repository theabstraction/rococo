#include <rococo.types.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <rococo.window.h>
#include "resource.h"

static HINSTANCE dllInstance = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    UNUSED(hModule);
    UNUSED(lpReserved);

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        dllInstance = hModule;
        {
            HICON hLargeIcon = LoadIconA(dllInstance, MAKEINTRESOURCEA(IDI_ICON_LARGE));
            HICON hSmallIcon = LoadIconA(dllInstance, MAKEINTRESOURCEA(IDI_ICON_LARGE));
            Rococo::Windows::InitRococoWindows(hModule, hLargeIcon, hSmallIcon, nullptr, nullptr);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

HINSTANCE GetDllInstance()
{
    return dllInstance;
}

#include <rococo.mvc.h>

using namespace Rococo;
using namespace Rococo::MVC;

namespace Rococo::Abedit
{
    IMVC_ViewSupervisor* CreateAbstractEditor(IMVC_Host& host, HWND hHostWindow, HINSTANCE hInstance, cstr commandLine);

    HINSTANCE GetAbEditorInstance()
    {
        return GetDllInstance();
    }
}

MVC_EXPORT_C_API IMVC_ViewSupervisor* CreateMVCView(IMVC_Host& host, HWND hHostWindow, HINSTANCE hInstance, cstr commandLine)
{
    return Abedit::CreateAbstractEditor(host, hHostWindow, hInstance, commandLine);
}