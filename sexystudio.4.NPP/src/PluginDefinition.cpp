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

enum { USERLIST_SEXY_AUTOCOMPLETE = 12345678 };

static ptrdiff_t autoCompleteCandidatePosition = 0;
static char callTipArgs[1024] = { 0 };

void onCalltipClicked(HWND hScintilla, int argValue)
{
    if (callTipArgs[0] != 0)
    {
        ptrdiff_t caretPos = SendMessageA(hScintilla, SCI_GETCURRENTPOS, 0, 0);
        if (autoCompleteCandidatePosition > 0 && autoCompleteCandidatePosition < caretPos)
        {
            SendMessageA(hScintilla, SCI_SETSELECTIONSTART, caretPos, 0);
            SendMessageA(hScintilla, SCI_SETSELECTIONEND, caretPos, 0);
            SendMessageA(hScintilla, SCI_REPLACESEL, 0, (LPARAM)callTipArgs);
            SendMessageA(hScintilla, SCI_CALLTIPCANCEL, 0, 0);
            callTipArgs[0] = 0;
        }
    }
}

#include <rococo.sxytype-inference.h>

thread_local std::vector<char> src_buffer;

cstr GetFirstNonAlphaPointer(substring_ref s)
{
    for (cstr p = s.start; p < s.end; ++p)
    {
        if (!IsAlphaNumeric(*p))
        {
            return p;
        }
    }

    return s.end;
}

cstr GetFirstNonTypeCharPointer(substring_ref s)
{
    bool inDot = false;

    for (cstr p = s.start; p < s.end; ++p)
    {
        if (!inDot)
        {
            if (*p == '.')
            {
                inDot = true;
                continue;
            }
        }

        if (IsAlphaNumeric(*p))
        {
            if (inDot)
            {
                inDot = false;
            }

            continue;
        }

        return p;
    }

    return s.end;
}

bool TryGetType(HWND hScintilla, substring_ref candidate, char type[256], ptrdiff_t caretPos)
{
    if (caretPos <= 0 || candidate.start == nullptr || !islower(*candidate.start))
    {
        return false;
    }

    src_buffer.resize(caretPos+1);

    Substring token = { candidate.start, GetFirstNonAlphaPointer(candidate) };

    SendMessageA(hScintilla, SCI_GETTEXT, caretPos+1, (LPARAM)src_buffer.data());

    cstr end = src_buffer.data() + caretPos;

    using namespace Rococo::Sexy;
    BadlyFormattedTypeInferenceEngine engine(src_buffer.data());

    cstr start = end - Length(token);

    auto inference = engine.InferVariableType({ start, end });
    if (inference.declarationType.start != inference.declarationType.end)
    {
        TypeInferenceType tit;
        engine.GetType(tit, inference);
        SafeFormat(type, 256, "%s", tit.buf);
        return true;
    }
    else
    {
        *type = 0;
        return false;
    }
}

void ShowAutocompleteDataForVariable(HWND hScintilla, substring_ref candidate)
{
    ptrdiff_t caretPos = SendMessageA(hScintilla, SCI_GETCURRENTPOS, 0, 0);

    char type[256];
    if (TryGetType(hScintilla, candidate, type, caretPos))
    {
        SendMessageA(hScintilla, SCI_CALLTIPSHOW, caretPos, (LPARAM)type);
    }
}

thread_local AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(1024);

void ShowAutocompleteDataForType(HWND hScintilla, substring_ref candidate)
{
    Substring token = { candidate.start, GetFirstNonTypeCharPointer(candidate) };

    auto& sb = dsb->Builder();
    sb.Clear();

    struct ANON : IEnumerator<cstr>
    {
        StringBuilder& sb;

        int count = 0;

        void operator()(cstr item) override
        {
            if (count > 0)
            {
                sb << " ";
            }

            count++;

            sb << item;
        }

        ANON(StringBuilder& _sb) : sb(_sb) {}
    } appendToString(sb);
    sexyIDE->ForEachAutoCompleteCandidate(token, appendToString);

    const fstring& sbString = *dsb->Builder();

    callTipArgs[0] = 0;

    if (appendToString.count == 1)
    {
        sexyIDE->GetHintForCandidate(token, callTipArgs);
        if (callTipArgs[0] != 0)
        {
            ptrdiff_t caretPos = SendMessageA(hScintilla, SCI_GETCURRENTPOS, 0, 0);
            SendMessageA(hScintilla, SCI_CALLTIPSHOW, caretPos, (LPARAM)callTipArgs);
            return;
        }
    }

    if (appendToString.count > 0)
    {
        SendMessageA(hScintilla, SCI_AUTOCSETCANCELATSTART, false, 0);
        SendMessageA(hScintilla, SCI_USERLISTSHOW, USERLIST_SEXY_AUTOCOMPLETE, (LPARAM)sbString.buffer);
    }
}

#include <vector>

static std::vector<fstring> keywords 
{
    "method"_fstring, "function"_fstring, "class"_fstring, "struct"_fstring
};

bool IsEndOfToken(char c)
{
    switch (c)
    {
    case '(':
    case ')':
    case ' ':
    case '\r':
    case '\n':
    case '\t':
        return true;
    }

    return false;
}

bool TryParseToken(HWND hScintilla, substring_ref candidate)
{
    using namespace Rococo;

    size_t len = candidate.end - candidate.start;
    
    for (auto keyword : keywords)
    {
        if (StartsWith(candidate, keyword))
        {
            if (len > keyword.length && IsEndOfToken(candidate.start[keyword.length]))
            {
                // We found a keyword, but we do not need to parse it
                return false;
            }
        }
    }

    if (islower(*candidate.start))
    {
        ShowAutocompleteDataForVariable(hScintilla, candidate);
        return true;
    }
    else if (isupper(*candidate.start) && len > 2)
    {
        ShowAutocompleteDataForType(hScintilla, candidate);
        return true;
    }

    return false;
}

class ScintillaLine
{
private:
    enum { MAX_LINE_LENGTH = 1024 };
    char line[MAX_LINE_LENGTH];
    size_t lineLength;

public:
    ScintillaLine() : lineLength(0)
    {
        line[0] = 0;
    }

    operator substring_ref() const
    {
        return { line, line + lineLength };
    }

    bool TryGetCurrentLine(HWND hScintilla)
    {
        size_t bufferLength = SendMessageA(hScintilla, SCI_GETCURLINE, 0, 0);

        if (bufferLength == 0 || bufferLength > MAX_LINE_LENGTH)
        {
            return false;
        }

        SendMessageA(hScintilla, SCI_GETCURLINE, bufferLength, (LPARAM)line);

        lineLength = bufferLength - 1;
        line[lineLength] = 0;

        return true;
    }

    cstr begin() const
    {
        return line;
    }

    cstr end() const
    {
        return line + lineLength;
    }
};

class ScintillaCursor
{
private:
    ptrdiff_t caretPos = 0;
    size_t lineNumber = 0;
    ptrdiff_t lineStartPosition = 0;
    ptrdiff_t column;

public:
    ScintillaCursor(HWND hScintilla)
    {
        caretPos = SendMessageA(hScintilla, SCI_GETCURRENTPOS, 0, 0);
        lineNumber = SendMessageA(hScintilla, SCI_LINEFROMPOSITION, caretPos, 0);
        lineStartPosition = SendMessageA(hScintilla, SCI_POSITIONFROMLINE, lineNumber, 0);
        column = caretPos - lineStartPosition;
    }

    ptrdiff_t CaretPos() const
    {
        return caretPos;
    }

    size_t LineNumber() const
    {
        return lineNumber;
    }

    ptrdiff_t LineStartPosition() const
    {
        return lineStartPosition;
    }

    ptrdiff_t ColumnNumber() const
    {
        return column;
    }
};

void UpdateAutoComplete(HWND hScintilla)
{
    ScintillaLine currentLine;
    if (!currentLine.TryGetCurrentLine(hScintilla))
    {
        return;
    }

    ScintillaCursor cursor(hScintilla);

    autoCompleteCandidatePosition = cursor.CaretPos();

    cstr lastCandidateInLine = currentLine.end();

    for(cstr p = currentLine.end(); p >= currentLine.begin(); p--)
    {
        switch (*p)
        {
        case '(':
            {
                cstr candidate = p + 1;
                if (candidate == lastCandidateInLine)
                {
                    return;
                }
                else if (TryParseToken(hScintilla, { candidate,  lastCandidateInLine }))
                {
                    auto candidateColumn = candidate - currentLine.begin();
                    lastCandidateInLine = currentLine.begin() + candidateColumn;
                    autoCompleteCandidatePosition = cursor.LineStartPosition() + candidateColumn;
                    return;
                }
            }
        }
    }

    autoCompleteCandidatePosition = 0;
}

void onUserItemSelected(HWND hScintilla, int idList, cstr item)
{
    if (idList == USERLIST_SEXY_AUTOCOMPLETE)
    {
        ptrdiff_t caretPos = SendMessageA(hScintilla, SCI_GETCURRENTPOS, 0, 0);
        if (autoCompleteCandidatePosition > 0 && autoCompleteCandidatePosition < caretPos)
        {
            SendMessageA(hScintilla, SCI_SETSELECTIONSTART, autoCompleteCandidatePosition, 0);
            SendMessageA(hScintilla, SCI_SETSELECTIONEND, caretPos, 0);
            SendMessageA(hScintilla, SCI_REPLACESEL, 0, (LPARAM)item);

            UpdateAutoComplete(hScintilla);
        }
    }
}

void onCharAdded(HWND hScintilla, char c)
{
    UpdateAutoComplete(hScintilla);
}

void onModified(SCNotification& notifyCode)
{

}

void helloDlg()
{
    AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(1024);
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
