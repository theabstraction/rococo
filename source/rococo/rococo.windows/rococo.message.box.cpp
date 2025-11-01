#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <rococo.target.h>
#include <Windows.h>

#undef min
#undef max

#include <rococo.api.h>
#include <rococo.strings.h>
#include <rococo.window.h>
#include <stdio.h>

#include "resource.h"

using namespace Rococo::Strings;

HANDLE g_hThisDLL;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID /*lpReserved*/)
{
    try
    {
        switch (reasonForCall)
        {
        case DLL_PROCESS_ATTACH:
            g_hThisDLL = hModule;
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
        }
    }
    catch (...)
    {
        return FALSE;
    }

    return TRUE;
}

namespace Rococo::Windows
{
    ROCOCO_WINDOWS_API int ShowMessageBox(IWindow& window, cstr text, cstr caption, uint32 type)
    {
        return MessageBoxA(window, text, caption, type);
    }

    ROCOCO_API_EXPORT void ShowExceptionDialog(const ExceptionDialogSpec& spec, HWND parent, IException& ex);

    ROCOCO_API_EXPORT void ShowErrorBox(Rococo::Windows::IWindow& parent, IException& ex, cstr captionFormat, ...)
    {
        char caption[256];

        va_list args;
        va_start(args, captionFormat);
        vsnprintf_s(caption, _TRUNCATE, captionFormat, args);
        va_end(args);

        HMODULE hRichEditor = LoadLibraryA("Riched20.dll");
        if (hRichEditor == NULL)
        {
            ShowMessageBox(parent, ex.Message(), caption, MB_ICONEXCLAMATION);
            return;
        }

        ExceptionDialogSpec spec
        {
            (HINSTANCE) g_hThisDLL, // dll of this module - where the dialog template is defined
            { 48, 520, 520, 230, 200 }, // column widths for the stackview
            MAKEINTRESOURCEA(IDD_EXCEPTION_DIALOG), // Typically (int) IDD_EXCEPTION_DIALOG from <rococo.win32.resources.h>
            IDC_STACKVIEW, // Typically (int) IDC_STACKVIEW from <rococo.win32.resources.h>
            IDC_LOGVIEW, // Typically (int) IDC_LOGVIEW from <rococo.win32.resources.h>
            caption,
        };

        ShowExceptionDialog(spec, parent, ex);
    }

    ROCOCO_API_EXPORT void ShowErrorBox(Rococo::Windows::IWindow& parent, int errorCode, cstr message, cstr caption)
    {
        try
        {
            Throw(errorCode, "%s", message);
        }
        catch (IException& ex)
        {
            ShowErrorBox(parent, ex, caption);
        }
    }
}//Rococo::Windows
