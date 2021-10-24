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

static auto DLL_NAME = L"sexystudio.dll";

cstr ErrorCaption = "SexyStudio 4 Notepad++ - error!";

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
        Format(pathToDLL, L"C:\\work\\rococo\\bin\\sexystudio.dll");
    }
}

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
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

class EditorLine
{
public:
    enum { MAX_LINE_LENGTH = 1024 };

private:    
    char line[MAX_LINE_LENGTH];
    size_t lineLength;

public:
    EditorLine() : lineLength(0)
    {
        line[0] = 0;
    }

    char* Data()
    {
        return line;
    }

    void SetLength(size_t length)
    {
        lineLength = length;
        line[length] = 0;
    }

    operator substring_ref() const
    {
        return { line, line + lineLength };
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

struct EditorCursor
{
    int64 caretPos = 0;
    size_t lineNumber = 0;
    int64 lineStartPosition = 0;
    int64 column;

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

ROCOCOAPI ISexyEditor
{
    // Return the length of the document
    virtual int64 GetDocLength() const = 0;

    // Retrieve the first len-1 chars in the document and null terminate it 
    virtual int64 GetText(int64 len, char* buffer) = 0;

    // Get the caret position of the editor
    virtual int64 GetCaretPos() const = 0;

    // Show call tip at caret position
    virtual void ShowCallTipAtCaretPos(cstr tip) const = 0;

    // Make autocomplete disappear when the caret moved
    virtual void SetAutoCompleteCancelWhenCaretMoved() = 0;

    // Show the autocomplete list
    virtual void ShowAutoCompleteList(cstr spaceSeparatedItems) = 0;

    // Get cursor metrics
    virtual void GetCursor(EditorCursor& cursor) const = 0;

    // Get the current line being edited
    virtual bool TryGetCurrentLine(EditorLine& line) const = 0;

    // Remove text and startPos,endPos and replace with the null terminated item text
    virtual void ReplaceText(int64 startPos, int64 endPos, cstr item) const = 0;
};

enum { USERLIST_SEXY_AUTOCOMPLETE = 12345678 };

class SexyEditor_Scintilla: public ISexyEditor
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
};

bool TryGetType(ISexyEditor& editor, substring_ref candidate, char type[256], ptrdiff_t caretPos)
{
    if (caretPos <= 0 || candidate.start == nullptr || !islower(*candidate.start))
    {
        return false;
    }

    src_buffer.resize(caretPos+1);

    Substring token = { candidate.start, GetFirstNonAlphaPointer(candidate) };

    editor.GetText(caretPos + 1, src_buffer.data());

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

void ShowAutocompleteDataForVariable(ISexyEditor& editor, substring_ref candidate)
{
    int64 caretPos = editor.GetCaretPos();

    char type[256];
    if (TryGetType(editor, candidate, type, caretPos))
    {
        editor.ShowCallTipAtCaretPos(type);
    }
}

thread_local AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(1024);

void ShowAutocompleteDataForType(ISexyEditor& editor, substring_ref candidate)
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
            editor.ShowCallTipAtCaretPos(callTipArgs);
            return;
        }
    }

    if (appendToString.count > 0)
    {
        editor.SetAutoCompleteCancelWhenCaretMoved();
        editor.ShowAutoCompleteList(sbString.buffer);
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

bool TryParseToken(ISexyEditor& editor, substring_ref candidate)
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
        ShowAutocompleteDataForVariable(editor, candidate);
        return true;
    }
    else if (isupper(*candidate.start) && len > 2)
    {
        ShowAutocompleteDataForType(editor, candidate);
        return true;
    }

    return false;
}

void UpdateAutoComplete(ISexyEditor& editor)
{
    EditorLine currentLine;
    if (!editor.TryGetCurrentLine(currentLine))
    {
        return;
    }

    EditorCursor cursor;
    editor.GetCursor(cursor);
  
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
                else if (TryParseToken(editor, { candidate,  lastCandidateInLine }))
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
        SexyEditor_Scintilla editor(hScintilla);
        ptrdiff_t caretPos = editor.GetCaretPos();
        if (autoCompleteCandidatePosition > 0 && autoCompleteCandidatePosition < caretPos)
        {
            editor.ReplaceText(autoCompleteCandidatePosition, caretPos, item);
            UpdateAutoComplete(editor);
        }
    }
}

void onCharAdded(HWND hScintilla, char c)
{
    SexyEditor_Scintilla editor(hScintilla);
    UpdateAutoComplete(editor);
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

        ::MessageBoxW(nppData._nppHandle, fullDesc, L"SexyIDE 4 Notepad++", MB_OK);
    }
    else
    {
        WideFilePath pathToDLL;
        GetDllPath(pathToDLL);

        ::MessageBoxW(nppData._nppHandle, pathToDLL, L"SexyIDE 4 Notepad++. Could not load DLL:", MB_OK);
    }
}
