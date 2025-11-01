#include <rococo.types.h>

#include <rococo.os.win32.global-ns.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    UNUSED(hModule);
    UNUSED(lpReserved);

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#include <rococo.mvc.h>

using namespace Rococo;
using namespace Rococo::MVC;

namespace Rococo::CFGS
{
    IMVC_ControllerSupervisor* CreateMVCControllerInternal(IMVC_Host& host, IMVC_View& view, cstr commandLine);
}

MVC_EXPORT_C_API IMVC_ControllerSupervisor * CreateMVCController(IMVC_Host & host, IMVC_View& view, cstr commandLine)
{
    return CFGS::CreateMVCControllerInternal(host, view, commandLine);
}