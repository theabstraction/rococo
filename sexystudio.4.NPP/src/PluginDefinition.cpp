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

#include <rococo.types.h>
#include <rococo.api.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.sexystudio.api.h>

#include <rococo.os.win32.h>
#include <rococo.window.h>

#include <rococo.strings.h>

#include "PluginDefinition.h"
#include "menuCmdID.h"

#include <vector>

#include <rococo.auto-complete.h>

using namespace Rococo;
using namespace Rococo::SexyStudio;
using namespace Rococo::AutoComplete;

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

HMODULE hFactoryModule = nullptr;
ISexyStudioFactory1* factory = nullptr;
ISexyStudioInstance1* sexyIDE = nullptr;

static cstr ErrorCaption = "SexyStudio 4 Notepad++ - error!";

void GetDllPath(WideFilePath& pathToDLL)
{
    *pathToDLL.buf = 0;

    HKEY hSexy4Npp;
    LSTATUS status = RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Rococo.Sexy\\SexyStudio", &hSexy4Npp);
    if (status == ERROR_SUCCESS)
    {
        enum { MAX_ROOT_LEN = 128 };
        static_assert(MAX_ROOT_LEN < WideFilePath::CAPACITY);

        DWORD type = REG_SZ;     
        DWORD len = MAX_ROOT_LEN * sizeof(wchar_t);
        status = RegQueryValueExW(hSexy4Npp, L"BinPath", NULL, &type, (LPBYTE)pathToDLL.buf, &len);
        if (status != ERROR_SUCCESS)
        {
            *pathToDLL.buf = 0;
        }

        RegCloseKey(hSexy4Npp);
    }

    if (*pathToDLL.buf == 0)
    {
#ifdef _DEBUG
        Format(pathToDLL, L"C:\\work\\rococo\\bin\\sexystudio.debug.dll");
#else
        Format(pathToDLL, L"C:\\work\\rococo\\bin\\sexystudio.dll");
#endif
    }
}

struct SexyStudioEventHandler: ISexyStudioEventHandler
{
    EIDECloseResponse OnIDEClose(IWindow& topLevelParent) override
    {
        ShowWindow(topLevelParent, SW_HIDE);
        return EIDECloseResponse::Continue;
    }
};

static SexyStudioEventHandler static_SexyStudioEventHandler;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
    struct CLOSURE : Rococo::Windows::IWindow
    {
        HWND hWnd;
        operator HWND() const override
        {
            return hWnd;
        }
    } topLevelWindow;

    topLevelWindow.hWnd = nppData._nppHandle;

    try
    {
        WideFilePath pathToDLL;
        GetDllPath(pathToDLL);

        hFactoryModule = LoadLibraryW(pathToDLL);
        if (hFactoryModule == nullptr)
        {
            Throw(GetLastError(), "Could not load library: %ls", pathToDLL.buf);
        }

        FARPROC proc = GetProcAddress(hFactoryModule, "CreateSexyStudioFactory");
        if (proc == nullptr)
        {
            Throw(GetLastError(), "Could not find CreateSexyStudioFactory in %ls", pathToDLL);
        }

        auto CreateSexyStudioFactory = (Rococo::SexyStudio::FN_CreateSexyStudioFactory)proc;

        cstr interfaceURL = "Rococo.SexyStudio.ISexyStudioFactory1";

        int nErr = CreateSexyStudioFactory((void**)&factory, interfaceURL);
        if FAILED(nErr)
        {
            Throw(nErr, "CreateSexyStudioFactory with URL %s failed", interfaceURL);
        }

        if (sexyIDE == nullptr)
        {
            if (factory)
            {
                sexyIDE = factory->CreateSexyIDE(topLevelWindow, static_SexyStudioEventHandler);
            }
        }

        if (sexyIDE)
        {
            sexyIDE->SetTitle("SexyStudio For Notepad++");
        }
    }
    catch (IException& ex)
    {
        Rococo::OS::ShowErrorBox(topLevelWindow, ex, ErrorCaption);
    }
}

void pluginCleanUp()
{
    if (sexyIDE)
    {
        sexyIDE->Free();
    }

    if (factory)
    {
        factory->Free();
    }

    if (hFactoryModule != nullptr)
    {
        FreeLibrary(hFactoryModule);
    }
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{
    setCommand(0, TEXT("About Sexy IDE..."), helloDlg, NULL, false);

    if (factory != NULL)
    {
        //--------------------------------------------//
        //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
        //--------------------------------------------//
        // with function :
        // setCommand(int index,                      // zero based number to indicate the order of command
        //            TCHAR *commandName,             // the command name that you want to see in plugin menu
        //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
        //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
        //            bool check0nInit                // optional. Make this menu item be checked visually
        //            );
        setCommand(1, TEXT("Show Sexy IDE"), showSexyIDE, NULL, false);
    //  setCommand(2, TEXT("Generate Autocomplete data..."), generateAutocompleteData, NULL, false);
    }
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

void generateAutocompleteData()
{
    if (sexyIDE)
    {

    }
}

bool IsPluginValid()
{
    return sexyIDE != nullptr;
}

void showSexyIDE()
{
    if (sexyIDE)
    {
        sexyIDE->Activate();
    }
}

enum { USERLIST_SEXY_AUTOCOMPLETE = 0x07112021 };

thread_local AutoFree<IDynamicStringBuilder> autocompleteStringBuilder = CreateDynamicStringBuilder(1024);

class SexyEditor_Scintilla : public ISexyEditor, IAutoCompleteBuilder
{
    HWND hScintilla;

public:
    SexyEditor_Scintilla(HWND _hScintilla) : hScintilla(_hScintilla) {}

    int64 GetDocLength() const override
    {
        return SendMessageA(hScintilla, SCI_GETTEXT, 0x7FFFFFFFFFFFFFFF, (LPARAM) nullptr);
    }

    int64 GetText(int64 len, char* buffer) override
    {
        return SendMessageA(hScintilla, SCI_GETTEXT, len, (LPARAM) buffer);
    }

    int64 GetCaretPos() const override
    {
        return SendMessageA(hScintilla, SCI_GETCURRENTPOS, 0, 0);
    }

    void ShowCallTipAtCaretPos(cstr tip) const
    {
        int64 caretPos = GetCaretPos();
        SendMessageA(hScintilla, SCI_CALLTIPCANCEL, 0, 0);
        SendMessageA(hScintilla, SCI_CALLTIPSHOW, caretPos, (LPARAM) tip);
    }

    void SetAutoCompleteCancelWhenCaretMoved() override
    {
        SendMessageA(hScintilla, SCI_AUTOCSETCANCELATSTART, false, 0);
    }

    void ShowAutoCompleteList(cstr spaceSeparatedItems) override
    {       
        SendMessageA(hScintilla, SCI_USERLISTSHOW, USERLIST_SEXY_AUTOCOMPLETE, (LPARAM)spaceSeparatedItems);
    }

    bool TryGetCurrentLine(EditorLine& line) const override
    {
        size_t bufferLength = SendMessageA(hScintilla, SCI_GETCURLINE, 0, 0);

        if (bufferLength == 0 || bufferLength > EditorLine::MAX_LINE_LENGTH)
        {
            return false;
        }

        SendMessageA(hScintilla, SCI_GETCURLINE, bufferLength, (LPARAM)line.Data());
        line.SetLength(bufferLength - 1);

        return true;
    }

    void GetCursor(EditorCursor& cursor) const override
    {
        cursor.caretPos = SendMessageA(hScintilla, SCI_GETCURRENTPOS, 0, 0);
        cursor.lineNumber = SendMessageA(hScintilla, SCI_LINEFROMPOSITION, cursor.caretPos, 0);
        cursor.lineStartPosition = SendMessageA(hScintilla, SCI_POSITIONFROMLINE, cursor.lineNumber, 0);
        cursor.column = cursor.caretPos - cursor.lineStartPosition;
    }

    void ReplaceText(int64 startPos, int64 endPos, cstr item) const override
    {
        SendMessageA(hScintilla, SCI_SETSELECTIONSTART, startPos, 0);
        SendMessageA(hScintilla, SCI_SETSELECTIONEND, endPos, 0);
        SendMessageA(hScintilla, SCI_REPLACESEL, 0, (LPARAM)item);
    }

    IAutoCompleteBuilder& AutoCompleteBuilder() override
    {
        return *this;
    }

    void AddItem(cstr item) override
    {
        auto& sb = autocompleteStringBuilder->Builder();
        if (sb.Length() > 0)
        {
            sb.AppendChar(' ');
        }

        sb.AppendFormat("%s", item);
    }

    void ShowAndClearItems() override
    {
        auto& sb = autocompleteStringBuilder->Builder();
        SetAutoCompleteCancelWhenCaretMoved();
        ShowAutoCompleteList(*sb);
        sb.Clear();
    }
};

void onCalltipClicked(HWND hScintilla, int argValue)
{
    if (sexyIDE)
    {
        SexyEditor_Scintilla editor(hScintilla);
        sexyIDE->ReplaceCurrentSelectionWithCallTip(editor);
        SendMessageA(hScintilla, SCI_CALLTIPCANCEL, 0, 0);
    }
}

void onUserItemSelected(HWND hScintilla, int idList, cstr item)
{
    if (sexyIDE && idList == USERLIST_SEXY_AUTOCOMPLETE)
    {
        SexyEditor_Scintilla editor(hScintilla);
        sexyIDE->ReplaceSelectedText(editor, item);
    }
}

void onCharAdded(HWND hScintilla, char c)
{
    if (sexyIDE)
    {
        SexyEditor_Scintilla editor(hScintilla);
        sexyIDE->UpdateAutoComplete(editor);
    }
}

void onModified(SCNotification& notifyCode)
{

}

void helloDlg()
{
    if (factory)
    {
        AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(1024);
        auto& sb = dsb->Builder();

        auto* author = factory->GetMetaDataString(EMetaDataType::Author);
        auto* email = factory->GetMetaDataString(EMetaDataType::Email);
        auto* copyright = factory->GetMetaDataString(EMetaDataType::Copyright);
        auto* buildDate = factory->GetMetaDataString(EMetaDataType::BuildDate);

        sb.AppendFormat("Written by %s (Email %s)\r\n", author, email);
        sb.AppendFormat("%s\r\n", copyright);
        sb.AppendFormat("Build: %s", buildDate);

        wchar_t fullDesc[1024];
        SafeFormat(fullDesc, L"%hs", (cstr) *sb);

        ::MessageBoxW(nppData._nppHandle, fullDesc, L"SexyStudio 4 Notepad++", MB_OK);
    }
    else
    {
        WideFilePath pathToDLL;
        GetDllPath(pathToDLL);

        ::MessageBoxW(nppData._nppHandle, pathToDLL, L"SexyStudio 4 Notepad++. Could not load DLL:", MB_OK);
    }
}
