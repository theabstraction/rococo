// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#include <rococo.types.h>
#include <rococo.os.win32.global-ns.h>

BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
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

