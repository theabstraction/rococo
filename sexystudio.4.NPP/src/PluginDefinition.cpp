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

using namespace Rococo;
using namespace Rococo::SexyStudio;

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

#ifdef _DEBUG
static auto DLL_NAME = L"sexystudio.debug.dll";
#else
static auto DLL_NAME = L"sexystudio.dll";
#endif

cstr ErrorCaption = "SexyStudio 4 Notepad++ - error!";

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
    try
    {
        WideFilePath directory;

        if (!GetModuleFileNameW((HMODULE)hModule, directory.buf, directory.CAPACITY))
        {
            Throw(GetLastError(), "GetModuleFileNameW failed");
        }

        Rococo::OS::StripLastSubpath(directory.buf);

        // path now contains the directory
        WideFilePath pathToDLL;
        Format(pathToDLL, L"%ls%ls", directory.buf, DLL_NAME);

        hFactoryModule = LoadLibraryW(pathToDLL);
        if (hFactoryModule == nullptr)
        {
            Throw(GetLastError(), "Could not load library: %ls", pathToDLL.buf);
        }

        FARPROC proc = GetProcAddress(hFactoryModule, "CreateSexyStudioFactory");
        if (proc == nullptr)
        {
            Throw(GetLastError(), "Could not find CreateSexyStudioFactory in %ls", DLL_NAME);
        }

        auto CreateSexyStudioFactory = (Rococo::SexyStudio::FN_CreateSexyStudioFactory)proc;

        cstr interfaceURL = "Rococo.SexyStudio.ISexyStudioFactory1";

        int nErr = CreateSexyStudioFactory((void**)&factory, interfaceURL);
        if FAILED(nErr)
        {
            Throw(nErr, "CreateSexyStudioFactory with URL %s failed", interfaceURL);
        }

        struct CLOSURE : Rococo::Windows::IWindow
        {
            HWND hWnd;
            operator HWND() const override
            {
                return hWnd;
            }
        } topLevelWindow;

        // N.B making the IDE a child of the Notepad++ window is a lot more obstructive than having no parent
        topLevelWindow.hWnd = nppData._nppHandle;

        if (sexyIDE == nullptr)
        {
            if (factory)
            {
                sexyIDE = factory->CreateSexyIDE(topLevelWindow);
            }
        }

        if (sexyIDE)
        {
            sexyIDE->SetTitle("SexyStudio For Notepad++");
        }
    }
    catch (IException& ex)
    {
        Rococo::OS::ShowErrorBox(Rococo::Windows::NoParent(), ex, ErrorCaption);
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
        setCommand(0, TEXT("Show Sexy IDE"), showSexyIDE, NULL, false);
        setCommand(1, TEXT("About Sexy IDE..."), helloDlg, NULL, false);
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

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void showSexyIDE()
{
    if (sexyIDE)
    {
        sexyIDE->Activate();
    }
    /*
    // Open a new document
    ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

    // Get the current scintilla
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
        return;
    HWND curScintilla = (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;

    // Say hello now :
    // Scintilla control has no Unicode mode, so we use (char *) here
    ::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)"Hello, Notepad++!");
    */
}

bool HasFlags(int fields, int flags)
{
    return (fields & flags) == flags;
}

void ParseToken(HWND hScintilla, cstr token, cstr endOfToken)
{
    using namespace Rococo;

    char prefix[1024];
    errno_t err = strncpy_s(prefix, token, endOfToken - token);

    static AutoFree<IStringBuilder> dsb = CreateDynamicStringBuilder(1024);
    auto& sb = dsb->Builder();
    sb.Clear();

    WideFilePath path;
    ::SendMessage(nppData._nppHandle, NPPM_GETNPPDIRECTORY, path.CAPACITY, (LPARAM)path.buf);

    WideFilePath autocompletionFile;
    Format(autocompletionFile, L"%ls\\autoCompletion\\sexy.xml", path.buf);

    struct ANON : IEnumerator<cstr>
    {
        StringBuilder& sb;

        bool first = true;

        void operator()(cstr item) override
        {
            if (first)
            {
                first = false;
            }
            else
            {
                 sb << " ";
            }

            sb << item;       
        }

        ANON(StringBuilder& _sb) : sb(_sb) {}
    } appendToString(sb);
    sexyIDE->ForEachAutoCompleteCandidate(prefix, appendToString);

    const fstring& sbString = *dsb->Builder();

    SendMessageA(hScintilla, SCI_AUTOCSETCANCELATSTART, false, 0);
    SendMessageA(hScintilla, SCI_USERLISTSHOW, 1, (LPARAM) sbString.buffer);

    static errno_t x = err;
}

void onCharAdded(HWND hScintilla, char c)
{
    size_t bufferLength = SendMessageA(hScintilla, SCI_GETCURLINE, 0, 0);

    if (bufferLength == 0 || bufferLength > 1024)
    {
        return;
    }
    
    char line[1024];
    ptrdiff_t caretPos = SendMessageA(hScintilla, SCI_GETCURLINE, bufferLength, (LPARAM)line);

    size_t lineLength = bufferLength - 1;

    size_t lineNumber = SendMessageA(hScintilla, SCI_LINEFROMPOSITION, 0, 0);
    ptrdiff_t lineStartPosition = SendMessageA(hScintilla, SCI_POSITIONFROMLINE, lineNumber, 0);
       
    line[lineLength] = 0;

    auto delta = caretPos - lineStartPosition;

    if (caretPos <= lineStartPosition)
    {
        return;
    }

    if (delta > lineLength || delta == 0)
    {
        return;
    }

    cstr pos = line + delta;
    while (pos > line)
    {
        pos--;

        switch (*pos)
        {
        case '(':
            pos++;
            goto foundFunction;
        case ')':
        case ' ':
        case '\t':
            return;
        }
    }

    return;

 foundFunction:

    cstr endOfLine = line + lineLength;

    cstr endOfToken = pos;
    while (endOfToken < endOfLine)
    {
        switch (*endOfToken)
        {
        case ')':
            return;
        case ' ':
        case '\t':
            ParseToken(hScintilla, pos, endOfToken);
            break;
        }

        endOfToken++;
    }

    ParseToken(hScintilla, pos, endOfToken);
}

void onModified(SCNotification& notifyCode)
{

}

#include <rococo.strings.h>

void helloDlg()
{
    AutoFree<IStringBuilder> dsb = CreateDynamicStringBuilder(1024);
    auto& sb = dsb->Builder();
    if (factory)
    {
        auto* author = factory->GetMetaDataString(EMetaDataType::Author);
        auto* email = factory->GetMetaDataString(EMetaDataType::Email);
        auto* copyright = factory->GetMetaDataString(EMetaDataType::Copyright);
        auto* buildDate = factory->GetMetaDataString(EMetaDataType::BuildDate);

        sb.AppendFormat("Written by %s (Email %s)\r\n", author, email);
        sb.AppendFormat("%s\r\n", copyright);
        sb.AppendFormat("Build: %s", buildDate);

        wchar_t fullDesc[1024];
        SafeFormat(fullDesc, L"%hs", (cstr) *sb);

        ::MessageBoxW(nppData._nppHandle, fullDesc, L"SexyIDE 4 Notepad++", MB_OK);
    }
}
