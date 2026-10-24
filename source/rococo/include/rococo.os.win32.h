// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#ifdef _WIN32

# ifdef _WINDOWS_
#  error _WINDOWS_ already defined Windows.h already included apparently
# endif

// UE5 namespaced Windows API conflicts with our own, so trick our code into thinking it is using our own
# ifdef MINIMAL_WINDOWS_API
namespace MSWindows = ::Windows;

namespace Windows
{
	MINIMAL_WINDOWS_API DWORD GetLastError();
	inline bool IsNull(HANDLE handle) { return handle == nullptr; }
    MINIMAL_WINDOWS_API HWND GetActiveWindow();

    enum EAncestor
    {
        GA_PARENT = 1,
        GA_ROOT,
        GA_ROOTOWNER
    };

    MINIMAL_WINDOWS_API HWND GetAncestor(HWND hwnd, UINT gaFlags);
}

#else
# include "rococo.os.win32.mswindow.h"

namespace MSWindows
{
	inline bool IsNull(HANDLE handle) { return handle.internal == 0; }
}

#endif

#ifndef ROCOCO_WINDOWS_API
# define ROCOCO_WINDOWS_API __declspec(dllimport)
#endif

namespace Rococo::Windows
{
    // WindowsAPI free version of InitRococoWindows. Note, you need to provide HINSTANCE for the app to pInstance
    ROCOCO_WINDOWS_API void InitRococoWindows(void* pInstance, const char* titleFontFaceName, int titleHeight, const char* controlFontFaceName, int controlFontHeight);
}

#endif
