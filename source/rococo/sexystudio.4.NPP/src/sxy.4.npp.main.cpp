//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

// Modifications to the original Copyright(c)2021-2023 by Mark Anthony Taylor. This DLL code, sexystudio.4.notepad++, is open source and free.

#include <rococo.types.h>

#include "PluginDefinition.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;

using namespace Rococo;
using namespace Rococo::Strings;

HANDLE g_hModule = NULL;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID /*lpReserved*/)
{
	try
	{
		switch (reasonForCall)
		{
			case DLL_PROCESS_ATTACH:
                g_hModule = hModule;
				pluginInit(hModule);
				break;

			case DLL_PROCESS_DETACH:
				pluginCleanUp();
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

#include <string.h>

void GetDllPath(WideFilePath& pathToDLL)
{
    DWORD len = GetModuleFileNameW((HMODULE) g_hModule, pathToDLL.buf, WideFilePath::CAPACITY);
    for (wchar_t* p = pathToDLL.buf + len; p >= pathToDLL.buf; p--)
    {
        if (*p == '\\')
        {
            *p = 0;
            break;
        }
    }

    char configPath[MAX_PATH];
    _snprintf_s(configPath, MAX_PATH, "%ws\\npp.config.txt", pathToDLL.buf);

    HANDLE hFile = CreateFileA(configPath, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    char contents[WideFilePath::CAPACITY];
    DWORD bytesRead = 0;
    BOOL status = ReadFile(hFile, contents, sizeof contents - 1, &bytesRead, NULL);
    if (status == 0)
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        UNUSED(hr);
        return;
    }

    CloseHandle(hFile);

    if (!status || bytesRead < 4)
    {
        return;
    }

    contents[sizeof(contents) - 1] = 0;
    contents[bytesRead] = 0;

    cstr key = nullptr, value = nullptr;

    auto parseKeyValue = [&pathToDLL](cstr key, cstr value)
        {
            if (!value) return;

            if (strcmp(key, "DebugBinPath") == 0)
            {
                // Typically: C:\work\rococo\SexyStudioBin\sexystudio.dll
#ifdef _DEBUG
                _snwprintf_s(pathToDLL.buf, WideFilePath::CAPACITY, L"%hs", value);
#endif
            }
        };

    enum EState { BLANKSPACE_BEFORE_TOKENS, READING_KEY, READING_VALUE } state = BLANKSPACE_BEFORE_TOKENS;

    for (char* p = contents; p < contents + bytesRead; p++)
    {
        if (state == BLANKSPACE_BEFORE_TOKENS)
        {
            if (*p <= 32)
            {
                // More whitespace
                continue;
            }

            state = READING_KEY;
            key = p;
            continue;
        }

        if (state == READING_KEY)
        {
            if (*p == '=')
            {
                *p = 0;
                state = READING_VALUE;
                value = p + 1;
            }

            continue;
        }

        // reading value

        if (*p <= 32)
        {
            // Blankspace or 0
            *p = 0;

            // we now have a key and value pair
            parseKeyValue(key, value);

            key = p + 1;
            value = nullptr;
            state = BLANKSPACE_BEFORE_TOKENS;
        }
    }

    parseKeyValue(key, value);
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
	commandMenuInit();
}

bool IsPluginValid();

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return L"SexyStudio";
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = IsPluginValid() ? nbFunc : 1;
	return funcItem;
}

void onModified(SCNotification& notifyCode);
void onCharAdded(HWND hScintilla, char c);
void onUserItemSelected(HWND hScintilla, int idList, cstr item);
void onCalltipClicked(HWND hScintilla, int argValue);

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	switch (notifyCode->nmhdr.code) 
	{
		case NPPN_SHUTDOWN:
			commandMenuCleanUp();
			break;
		case SCN_MODIFIED:
			onModified(*notifyCode);
			break;
		case SCN_CHARADDED:
			onCharAdded((HWND) notifyCode->nmhdr.hwndFrom, static_cast<char>(notifyCode->ch));
			break;
		case SCN_USERLISTSELECTION:
			onUserItemSelected((HWND)notifyCode->nmhdr.hwndFrom, (int) notifyCode->wParam, notifyCode->text);
			break;
		case SCN_CALLTIPCLICK:
			onCalltipClicked((HWND)notifyCode->nmhdr.hwndFrom, (int)notifyCode->wParam);
	}

	return;
}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	UNUSED(msg);
	UNUSED(wParam);
	UNUSED(lParam);
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
