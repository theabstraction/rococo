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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <rococo.window.h>

#include <rococo.strings.h>

#include "PluginDefinition.h"
#include "menuCmdID.h"

#include <vector>

#include <rococo.auto-complete.h>

using namespace Rococo;
using namespace Rococo::SexyStudio;
using namespace Rococo::AutoComplete;
using namespace Rococo::Strings;

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

void GetDllPath(WideFilePath& pathToDLL);

struct SexyStudioEventHandler: ISexyStudioEventHandler
{
    bool TryOpenEditor(cstr filename, int lineNumber) override
    {
        WideFilePath wPath;
        swprintf_s(wPath.buf, wPath.CAPACITY, L"%hs", filename);
        if (SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)wPath.buf))
        {
            SendMessage(nppData._scintillaMainHandle, SCI_GOTOLINE, lineNumber - 1, 0);
            return true;
        }
        return false;
    }

    EIDECloseResponse OnIDEClose(Windows::IWindow& topLevelParent) override
    {
        ShowWindow(topLevelParent, SW_HIDE);
        return EIDECloseResponse::Continue;
    }
};

static SexyStudioEventHandler static_SexyStudioEventHandler;

#include <rococo.debugging.h>

struct NppRococoException : public IException, public Debugging::IStackFrameEnumerator
{
    char msg[2048];
    int32 errorCode;

    cstr Message() const override
    {
        return msg;
    }

    int32 ErrorCode() const override
    {
        return errorCode;
    }

    IStackFrameEnumerator* StackFrames() override
    {
        return this;
    }

    void FormatEachStackFrame(Debugging::IStackFrameFormatter&) override
    {
    }
};

void NppThrow(int32 errorCode, cstr format, ...)
{
    va_list args;
    va_start(args, format);

    NppRococoException ex;

    vsprintf_s(ex.msg, sizeof(ex.msg), format, args);

    ex.errorCode = errorCode;

    throw ex;
}

namespace Local // Copied from the Rococo UTIL lib, so that we dont have to manage the util dll in the notepad++ plugin folder
{
    bool MakeContainerDirectory(wchar_t* filename)
    {
        int len = (int)wcslen(filename);

        for (int i = len - 2; i > 0; --i)
        {
            if (filename[i] == L'\\')
            {
                filename[i + 1] = 0;
                return true;
            }
        }

        return false;
    }

    bool StartsWith(crwstr bigString, crwstr prefix)
    {
        return wcsncmp(bigString, prefix, wcslen(prefix)) == 0;
    }

    bool Eq(crwstr a, crwstr b)
    {
        return wcscmp(a, b) == 0;
    }

    bool EndsWith(crwstr bigString, crwstr suffix)
    {
        size_t len = wcslen(suffix);
        size_t lenBig = wcslen(bigString);
        crwstr t = bigString + lenBig - len;
        return Eq(suffix, t);
    }

    ptrdiff_t Length(cr_substring token)
    {
        return token.finish - token.start;
    }

    bool SubstringToString(char* outputBuffer, size_t sizeofOutputBuffer, cr_substring substring)
    {
        if (Local::Length(substring) >= (ptrdiff_t)sizeofOutputBuffer)
        {
            return false;
        }

        char* writePtr = outputBuffer;
        cstr readPtr = substring.start;
        while (readPtr < substring.finish)
        {
            *writePtr++ = *readPtr++;
        }

        *writePtr = 0;

        return true;
    }
}

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
    UNUSED(hModule);
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

        WideFilePath binDirectory = pathToDLL;
        Local::MakeContainerDirectory(binDirectory.buf);

        DWORD dflags = GetFileAttributesW(binDirectory);
        if (dflags == INVALID_FILE_ATTRIBUTES)
        {
            NppThrow(GetLastError(), "Could not load library: %ls!\r\nPath %ls does not exist.\r\nFix is to set correct path in npp.config.txt", pathToDLL.buf, binDirectory.buf);
        }

        WideFilePath oldDirectory;
        GetDllDirectoryW(oldDirectory.CAPACITY, oldDirectory.buf);

        SetDllDirectoryW(binDirectory);

        hFactoryModule = LoadLibraryW(pathToDLL);

        SetDllDirectoryW(oldDirectory);

        if (hFactoryModule == nullptr)
        {
            DWORD flags = GetFileAttributesW(pathToDLL);
            if (flags != INVALID_FILE_ATTRIBUTES)
            {
                NppThrow(GetLastError(), "Could not load library: %ls", pathToDLL.buf);
            }
            else
            {
                NppThrow(GetLastError(), "Could not load library: %ls. File appears to not exist.", pathToDLL.buf);
            }
        }

        FARPROC proc = GetProcAddress(hFactoryModule, "CreateSexyStudioFactory");
        if (proc == nullptr)
        {
            NppThrow(GetLastError(), "Could not find CreateSexyStudioFactory in %ls", pathToDLL.buf);
        }

        auto CreateSexyStudioFactory = (Rococo::SexyStudio::FN_CreateSexyStudioFactory)proc;

        cstr interfaceURL = "Rococo.SexyStudio.ISexyStudioFactory1";

        int nErr = CreateSexyStudioFactory((void**)&factory, interfaceURL);
        if FAILED(nErr)
        {
            NppThrow(nErr, "CreateSexyStudioFactory with URL %s failed", interfaceURL);
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
            sexyIDE->Gui().SetTitle("SexyStudio For Notepad++");
        }
    }
    catch (IException& ex)
    {
        char message[4096];
        sprintf_s(message, "Code: %d, Err: %s", ex.ErrorCode(), ex.Message());
        MessageBoxA(topLevelWindow.hWnd, message, "SexyStudio For Notepad++ Error", MB_ICONERROR);
    }
    catch (std::exception& stdEx)
    {
        char message[4096];
        sprintf_s(message, "std exception: %s", stdEx.what());
        MessageBoxA(topLevelWindow.hWnd, message, "SexyStudio For Notepad++ Error", MB_ICONERROR);
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

void gotoDefinition();

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
        setCommand(2, TEXT("Goto definition"), gotoDefinition, NULL, false);
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

class SexyEditor_Scintilla : public ISexyEditor, IAutoCompleteBuilder
{
    HWND hScintilla;

    std::vector<char> autocompleteStringBuilder;
    int autoCompleteIndex = 0;
public:
    SexyEditor_Scintilla(HWND _hScintilla) : hScintilla(_hScintilla)
    {
        autocompleteStringBuilder.resize(512);
    }

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

    bool TrySendOpenScintillaAtLine(cstr path, int lineNumber)
    {
        BOOL success;
        if constexpr (sizeof(TCHAR) == sizeof(char))
        {
            success = (BOOL)SendMessageW(nppData._nppHandle, NPPM_DOOPEN, (LPARAM)path, 0);
        }
        else
        {
            WideFilePath wPath;
            swprintf_s(wPath.buf, L"%hs", path);
            success = (BOOL)SendMessageW(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)wPath.buf);
        }

        if (success == TRUE)
        {
            SendMessage(nppData._scintillaMainHandle, SCI_SETFIRSTVISIBLELINE, max(lineNumber - 1, 0), 0);
            return true;
        }

        return false;
    }

    void GotoDefinition(const char* searchToken, const char* path, int lineNumber) override
    {
        if (sexyIDE)
        {
            struct ANON: IPreviewEventHandler
            {
                U8FilePath currentFile;
                int currentLineNumber = 0;

                ANON()
                {
                    currentFile.buf[0] = 0;
                }

                SexyEditor_Scintilla* This = 0;
                bool OnJumpToCode(const char* path, int lineNumber) override
                {
                    if (*currentFile.buf == 0)
                    {
                        TCHAR currentPath[MAX_PATH];
                        if (SendMessageW(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)currentPath) == TRUE)
                        {
                            if constexpr (sizeof TCHAR == sizeof(char))
                            {
                                strncpy(currentFile.buf, (const char*) currentPath, MAX_PATH);
                            }
                            else
                            {
                                _snprintf_s(currentFile.buf, _TRUNCATE, "%ws", currentPath);
                            }

                            currentLineNumber = (int) SendMessage(nppData._nppHandle, NPPM_GETCURRENTLINE, 0, 0);
                        }
                    }

                    if (strcmp("<this>", path) == 0)
                    {
                        return This->TrySendOpenScintillaAtLine(currentFile, lineNumber);
                    }
                    else
                    {
                        return This->TrySendOpenScintillaAtLine(path, lineNumber);
                    }
                }

                void OnBackButtonClicked() override
                {
                    if (*currentFile.buf > 0)
                    {
                        This->TrySendOpenScintillaAtLine(currentFile, currentLineNumber);
                    }
                }
            } eventHandler;

            eventHandler.This = this;

            Windows::THIS_WINDOW topLevelWindow(hScintilla);
            sexyIDE->Gui().PopupPreview(topLevelWindow, searchToken, path, lineNumber, eventHandler);
        }
    }

    void ShowCallTipAtCaretPos(cstr tip) const
    {
        int64 caretPos = GetCaretPos();
        SendMessageA(hScintilla, SCI_CALLTIPCANCEL, 0, 0);

        if (*tip == '!')
        {
            // Exclamation is skipped, as it indicates a tip is for info only, and not substitution
            tip++;
        }

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

        if (bufferLength == 0 || bufferLength >= (size_t) line.MAX_LINE_LENGTH)
        {
            return false;
        }

        SendMessageA(hScintilla, SCI_GETCURLINE, bufferLength, (LPARAM)line.Data());

        // Handle linefeed and newline in any order that may be chosen for the text file in NPP
        if (line.Data()[bufferLength - 1] == '\n' || line.Data()[bufferLength - 1] == '\r')
        {
            // The final value was a newline or linefeed, so trim:
            bufferLength--;
            if (bufferLength > 0 && line.Data()[bufferLength - 1] == '\n' || line.Data()[bufferLength - 1] == '\r')
            {
                bufferLength--;
            }
        }

        line.SetLength(bufferLength);

        return bufferLength > 0;
    }

    void GetCursor(EditorCursor& cursor) const override
    {
        cursor.caretPos = SendMessageA(hScintilla, SCI_GETCURRENTPOS, 0, 0);
        cursor.lineNumber = SendMessageA(hScintilla, SCI_LINEFROMPOSITION, cursor.caretPos, 0);
        cursor.lineStartPosition = SendMessageA(hScintilla, SCI_POSITIONFROMLINE, cursor.lineNumber, 0);
        cursor.caretColumn = cursor.caretPos - cursor.lineStartPosition;
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

    void AddHint(cr_substring item) override
    {
        char buf[1024];
        Local::SubstringToString(buf, sizeof buf, item);
        ShowCallTipAtCaretPos(buf);
    }

    void AddItem(cstr item) override
    {
        size_t requiredSpace = autoCompleteIndex + strlen(item) + 8;

        while (requiredSpace > autocompleteStringBuilder.size())
        {
            autocompleteStringBuilder.resize(autocompleteStringBuilder.size() * 2);
        }

        int nCharsWritten = snprintf(autocompleteStringBuilder.data() + autoCompleteIndex, autocompleteStringBuilder.size() - 1 - autoCompleteIndex, "%s%s", autoCompleteIndex > 0 ? " " : "", item);

        autoCompleteIndex += nCharsWritten;
    }

    void ShowAndClearItems() override
    {
        SetAutoCompleteCancelWhenCaretMoved();
        ShowAutoCompleteList(autocompleteStringBuilder.data());
        if (IsDebuggerPresent())
        {
            OutputDebugStringA("Autocomplete populated:\n");
            OutputDebugStringA(autocompleteStringBuilder.data());
            OutputDebugStringA("\n");
        }
        autocompleteStringBuilder[0] = 0;
        autoCompleteIndex = 0;
    }
};

void gotoDefinition()
{
    if (sexyIDE)
    {
        SexyEditor_Scintilla editor(nppData._scintillaMainHandle);
        sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);
    }
}

void onCalltipClicked(HWND hScintilla, int argValue)
{
    UNUSED(argValue);
    if (sexyIDE)
    {
        SexyEditor_Scintilla editor(hScintilla);
        sexyIDE->Gaffer().ReplaceCurrentSelectionWithCallTip(editor);
        SendMessageA(hScintilla, SCI_CALLTIPCANCEL, 0, 0);
    }
}

void onUserItemSelected(HWND hScintilla, int idList, cstr item)
{
    if (sexyIDE && idList == USERLIST_SEXY_AUTOCOMPLETE)
    {
        SexyEditor_Scintilla editor(hScintilla);
        sexyIDE->Gaffer().ReplaceSelectedText(editor, item);
    }
}

void ValidateMemory()
{
    if (IsDebuggerPresent())
    {
        if (!_CrtCheckMemory())
        {
            __debugbreak();
        }
    }
}

void onCharAdded(HWND hScintilla, char c)
{
    UNUSED(c);
    if (sexyIDE)
    {
        WideFilePath filename;
        if (SendMessageA(nppData._nppHandle, NPPM_GETFILENAME, WideFilePath::CAPACITY, (LPARAM)filename.buf))
        {
            if (Local::EndsWith(filename, L".sxy"))
            {
                SendMessageA(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, WideFilePath::CAPACITY, (LPARAM)filename.buf);
            }
            else if (Local::StartsWith(filename, L"new"))
            {
                for (auto p = filename.buf; *p != 0; p++)
                {
                    if (*p == '.')
                    {
                        // Not a new file but something else
                        return;
                    }
                }
            }
            else // Neither .sxy nor a new file
            {
                return;
            }
        }

        SendMessageA(hScintilla, SCI_CALLTIPCANCEL, 0, 0);
        SexyEditor_Scintilla editor(hScintilla);
        sexyIDE->Gaffer().UpdateAutoComplete(editor, filename);
        ValidateMemory();
    }
}

void onModified(SCNotification& notifyCode)
{
    UNUSED(notifyCode);
}

void helloDlg()
{
    if (factory)
    {
        char intro[1024];

        auto* author = factory->GetMetaDataString(EMetaDataType::Author);
        auto* email = factory->GetMetaDataString(EMetaDataType::Email);
        auto* copyright = factory->GetMetaDataString(EMetaDataType::Copyright);
        auto* buildDate = factory->GetMetaDataString(EMetaDataType::BuildDate);

        sprintf_s(intro, "Written by %s (Email %s)\r\n%s\r\nBuild: %s", author, email, copyright, buildDate);

        ::MessageBoxA(nppData._nppHandle, intro, "SexyStudio 4 Notepad++", MB_OK);
    }
    else
    {
        WideFilePath pathToDLL;
        GetDllPath(pathToDLL);

        ::MessageBoxW(nppData._nppHandle, pathToDLL, L"SexyStudio 4 Notepad++. Could not load DLL:", MB_OK);
    }
}
