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

// Modifications to the original by Mark Anthony Taylor

#include <rococo.types.h>
#include <rococo.strings.h>

#include <rococo.libs.inl>


#include "PluginDefinition.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;

using namespace Rococo;
using namespace Rococo::Strings;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID /*lpReserved*/)
{
	try
	{
		switch (reasonForCall)
		{
			case DLL_PROCESS_ATTACH:
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


extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
	commandMenuInit();
}

bool IsPluginValid();

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	static wchar_t name[128] = { 0 };

	cstr errMsg = IsPluginValid() ? "": " (Could not load the Autocomplete DLL)";

	if (*name == 0)
	{
#ifdef _DEBUG
		SafeFormat(name, sizeof name, L"SexyStudio for Notepad++ (Debug Build %hs)%hs", __TIMESTAMP__, errMsg);
#else
		SafeFormat(name, sizeof name, L"SexyStudio for Notepad++ (Build %hs)%hs", __TIMESTAMP__, errMsg);
#endif
	}
	return name;
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
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
