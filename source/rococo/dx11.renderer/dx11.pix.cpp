#include <rococo.types.h>
#include <rococo.functional.h>
#include <shlobj.h>
#include <strsafe.h>
#include <string>

namespace Rococo::DX11
{
    // Code derived from the examples in the PIX docs
    void PreparePixDebugger()
    {
        LPWSTR programFilesPath = nullptr;
        SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

        std::wstring pixSearchPath = programFilesPath + std::wstring(L"\\Microsoft PIX\\*");

        WIN32_FIND_DATAW findData;
        bool foundPixInstallation = false;
        wchar_t newestVersionFound[MAX_PATH];

        HANDLE hFind = FindFirstFileW(pixSearchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
                    (findData.cFileName[0] != '.'))
                {
                    if (!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0)
                    {
                        foundPixInstallation = true;
                        StringCchCopyW(newestVersionFound, _countof(newestVersionFound), findData.cFileName);
                    }
                }
            } while (FindNextFileW(hFind, &findData) != 0);
        }

        FindClose(hFind);

        if (!foundPixInstallation)
        {
            // TODO: Error, no PIX installation found
            Throw(0, "No PIX installation found at %ws", pixSearchPath.c_str());
        }

        wchar_t capturerPath[MAX_PATH];
        StringCchCopyW(capturerPath, pixSearchPath.length(), pixSearchPath.data());
        StringCchCatW(capturerPath, MAX_PATH, &newestVersionFound[0]);
        StringCchCatW(capturerPath, MAX_PATH, L"\\WinPixGpuCapturer.dll");

        // Only load WinPixGpuCapturer.dll if it has not already been injected into the application.
        // This may happen if the application is launched through the PIX UI. 
        if (GetModuleHandleW(L"WinPixGpuCapturer.dll") == 0)
        {
            HMODULE hModule = LoadLibraryW(capturerPath);
            if (!hModule)
            {
                Throw(GetLastError(), "%s: LoadLibraryW('%ws') failed", __FUNCTION__, capturerPath);
            }
        }
    }
}