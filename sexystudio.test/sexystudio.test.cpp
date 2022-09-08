#include <rococo.os.win32.h>
#include <rococo.libs.inl>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.sxytype-inference.h>

#include <rococo.sexystudio.api.h>
#include <rococo.functional.h>

#include <rococo.auto-complete.h>

#include <rococo.window.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::SexyStudio;


#ifdef _DEBUG
static auto DLL_NAME = L"sexystudio.debug.dll";
#else
static auto DLL_NAME = L"sexystudio.dll";
#endif

cstr ErrorCaption = "SexyStudio Standalone App - error!";

#include <rococo.sexystudio.api.h>
#include "..\sexystudio\sexystudio.api.h"

#include <cstdio>

void TestDeduceVec2Fields(ISexyDatabase& database)
{
	cstr file = 
R"<CODE>(
	(using Sys.Type)
	(using Sys.Maths)
	(function DoStuffOnVec2 (Vec2 v) -> (Float32 y):
		(v
	)
)<CODE>";

	Substring sfile = { file, file + strlen(file) };

	cstr lastV = Strings::ReverseFind('v', sfile);

	Substring v = { lastV, lastV + 1 };

	printf("*** Start of %s ***\n", __FUNCTION__);

	printf("File: %s\n", file);

	struct ANON : ISexyFieldEnumerator
	{
		void OnField(cstr fieldName) override
		{
			printf("Field: %s\n", fieldName);
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
		}

		void OnFieldType(cr_substring fieldName, cr_substring searchRoot) override
		{

		}
	} fieldEnumerator;

	Substring type;
	bool isThis;
	if (!(type = Rococo::Sexy::GetLocalTypeFromCurrentDocument(isThis, v, sfile)))
	{
		Throw(0, "Bad inference - type should be Vec2");
	}

	database.EnumerateVariableAndFieldList(v, type, fieldEnumerator);

	printf("*** End of %s ***\n", __FUNCTION__);
}

void TestDeduceVec2Fields2(ISexyDatabase& database)
{
	cstr file =
		R"<CODE>(
	(using Sys.Type)
	(using Sys.Maths)
	(function DoStuffOnVec2 (Vec2 v) -> (Float32 y):
		(v.
	)
)<CODE>";

	Substring sfile = { file, file + strlen(file) };

	cstr lastV = Strings::ReverseFind('v', sfile);

	Substring v = { lastV, lastV + 2 };

	printf("*** Start of %s ***\n", __FUNCTION__);

	printf("File: %s\n", file);

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName) override
		{
			printf("Field: %s\n", fieldName);
			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);

			hintCount++;
		}

		void OnFieldType(cr_substring fieldName, cr_substring searchRoot) override
		{

		}
	} fieldEnumerator;

	Substring type;
	bool isThis;
	if (!(type = Rococo::Sexy::GetLocalTypeFromCurrentDocument(isThis, v, sfile)))
	{
		Throw(0, "Bad inference - type should be Vec2");
	}

	database.EnumerateVariableAndFieldList(v, type, fieldEnumerator);

	if (fieldEnumerator.hintCount != 1) Throw(0, "Bad hint count: %s", __FUNCTION__);
	if (fieldEnumerator.fieldCount != 2) Throw(0, "Bad field count: %s", __FUNCTION__);

	printf("*** End of %s ***\n", __FUNCTION__);
}

void TestDeduceMatrix4x4Fields(ISexyDatabase& database)
{
	cstr file =
		R"<CODE>(
	(using Sys.Type)
	(using Sys.Maths)
	(function DoStuffOnM4x4 (Matrix4x4 m) -> (Float32 y):
		(m.r0
	)
)<CODE>";

	Substring sfile = { file, file + strlen(file) };

	cstr lastV = Strings::ReverseFind('m', sfile);

	Substring v = { lastV, lastV + 4 };

	printf("*** Start of %s ***\n", __FUNCTION__);

	printf("File: %s\n", file);

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName) override
		{
			printf("Field: %s\n", fieldName);
			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			hintCount++;
		}

		void OnFieldType(cr_substring fieldName, cr_substring searchRoot) override
		{

		}
	} fieldEnumerator;

	Substring type;
	bool isThis;
	if (!(type = Rococo::Sexy::GetLocalTypeFromCurrentDocument(isThis, v, sfile)))
	{
		Throw(0, "Bad inference - type should be Vec4");
	}

	database.EnumerateVariableAndFieldList(v, type, fieldEnumerator);

	if (fieldEnumerator.fieldCount != 4) Throw(0, "Bad fieldCount: %s", __FUNCTION__);
	if (fieldEnumerator.hintCount == 0) Throw(0, "Bad hint count: %s", __FUNCTION__);

	printf("*** End of %s ***\n", __FUNCTION__);
}

void TestHintVec2(ISexyDatabase& database)
{
	cstr file =
		R"<CODE>(
	(using Sys.Type)
	(using Sys.Maths)
	(function DoStuffOnM4x4 (Matrix4x4 m) -> (Float32 y):
		(Sys.Maths.
	)
)<CODE>";

	Substring sfile = { file, file + strlen(file) };

	cstr lastV = Strings::ReverseFind('S', sfile);

	Substring v = { lastV, lastV + 10 };

	printf("*** Start of %s ***\n", __FUNCTION__);

	printf("File: %s\n", file);

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		bool found = false;

		void OnField(cstr fieldName) override
		{
			if (Eq(fieldName, "Sys.Maths.Vec2"))
			{
				found = true;
			}
			printf("Field: %s\n", fieldName);
			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			hintCount++;
		}

		void OnFieldType(cr_substring fieldName, cr_substring searchRoot) override
		{

		}
	} fieldEnumerator;

	database.ForEachAutoCompleteCandidate(v, fieldEnumerator);

	if (!fieldEnumerator.found)
	{
		Throw(0, "Could not find Vec2 in Sys.Maths.");
	}

	printf("*** End of %s ***\n", __FUNCTION__);
}

void TestDeduceMethods(ISexyDatabase& database)
{
	cstr file =
		R"<CODE>(
	(using Sys.Type)
	(using Sys.Maths)
	(function Main (Int32 id) -> (Int32 exitCode):
		(IStringBuilder sb = (NewParagraphBuilder))
		(sb.
	)
)<CODE>";

	printf("*** Start of %s ***\n", __FUNCTION__);

	printf("File: %s\n", file);

	Substring sfile = { file, file + strlen(file) };

	cstr lastS = Strings::ReverseFind('s', sfile);

	Substring sb = { lastS, lastS + 3 };

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName) override
		{
			printf("Method: %s\n", fieldName);
			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			hintCount++;
		}

		void OnFieldType(cr_substring fieldName, cr_substring searchRoot) override
		{

		}
	} fieldEnumerator;

	Substring type;
	bool isThis;
	if (!(type = Rococo::Sexy::GetLocalTypeFromCurrentDocument(isThis, sb, sfile)) || !Eq(type, "IStringBuilder"))
	{
		Throw(0, "Bad inference - type should be IStringBuilder");
	}

	database.EnumerateVariableAndFieldList(sb, type, fieldEnumerator);

	// We give a bit of range on this method in the case that IStringBuilder is modified in the future
	if (fieldEnumerator.fieldCount < 15 || fieldEnumerator.fieldCount > 30) Throw(0, "Bad fieldCount: %s", __FUNCTION__);
	if (fieldEnumerator.hintCount != 1) Throw(0, "Bad hint count: %s", __FUNCTION__);

	printf("*** End of %s ***\n", __FUNCTION__);
}

void TestDeduceMethods2(ISexyDatabase& database)
{
	cstr file =
		R"<CODE>(
	(using Sys.Type)
	(using Sys.Maths)
	(function Main (Int32 id) -> (Int32 exitCode):
		(Sys.Type.IStringBuilder sb = (NewParagraphBuilder))
		(sb.
	)
)<CODE>";

	printf("*** Start of %s ***\n", __FUNCTION__);

	printf("File: %s\n", file);

	Substring sfile = { file, file + strlen(file) };

	cstr lastS = Strings::ReverseFind('s', sfile);

	Substring sb = { lastS, lastS + 3 };

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName) override
		{
			printf("Method: %s\n", fieldName);
			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			hintCount++;
		}

		void OnFieldType(cr_substring fieldName, cr_substring searchRoot) override
		{

		}
	} fieldEnumerator;

	Substring type;
	bool isThis;
	if (!(type = Rococo::Sexy::GetLocalTypeFromCurrentDocument(isThis, sb, sfile)) || !Eq(type, "Sys.Type.IStringBuilder"))
	{
		Throw(0, "Bad inference - type should be IStringBuilder");
	}

	database.EnumerateVariableAndFieldList(sb, type, fieldEnumerator);

	// We give a bit of range on this method in the case that IStringBuilder is modified in the future
	if (fieldEnumerator.fieldCount < 15 || fieldEnumerator.fieldCount > 30) Throw(0, "Bad fieldCount: %s", __FUNCTION__);
	if (fieldEnumerator.hintCount != 1) Throw(0, "Bad hint count: %s", __FUNCTION__);

	printf("*** End of %s ***\n", __FUNCTION__);
}

void TestLocalStruct(ISexyDatabase& database)
{
	cstr file =
		R"<CODE>(
	(using Sys.Type)
	(using Sys.Maths)

	(struct Thing
		(Int32 maJig)
		(Int32 maJigEx)
		(array Int32 maJigArgs)
	)

	(struct Thing2
		(Int32 maJig)
		(Int32 maJigEx)	
	)

	(function Main (Int32 id) -> (Int32 exitCode):
		(Thing thing)
		(thing.
	)
)<CODE>";

	printf("*** Start of %s ***\n", __FUNCTION__);

	printf("File: %s\n", file);

	Substring sfile = { file, file + strlen(file) };

	cstr lastS = Strings::ReverseFind('t', sfile);

	Substring sb = { lastS, lastS + 6 };

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName) override
		{
			printf("Method: %s\n", fieldName);

			if (!StartsWith(fieldName, "maJig"))
			{
				Throw(0, "Bad inference '%s' - type should start with maJig", fieldName);
			}

			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			hintCount++;
		}

		void OnFieldType(cr_substring fieldName, cr_substring searchRoot) override
		{

		}
	} fieldEnumerator;

	Substring type;
	bool isThis;
	if (!(type = Rococo::Sexy::GetLocalTypeFromCurrentDocument(isThis, sb, sfile)))
	{
		if (!Eq(type, "Thing"))
		{
			Throw(0, "Bad inference '%s' - type should be Thing", type);
		}
	}

	Rococo::Sexy::EnumerateLocalFields(fieldEnumerator, sb, type, sfile);

	if (fieldEnumerator.fieldCount != 3)
	{
		Throw(0, "Bad inference '%d' - expecting one field of Thing", fieldEnumerator.fieldCount);
	}

	printf("*** End of %s ***\n", __FUNCTION__);
}

void TestLocalStruct2(ISexyDatabase& database)
{
	cstr file =
		R"<CODE>(
	(using Sys.Type)
	(using Sys.Maths)

	(struct Thing
		(Vec3 maJig)
		(Vec2 maJigEx)
		(array Int32 maJigArgs)
	)

	(struct Thing2
		(Int32 maJig)
		(Int32 maJigEx)	
	)

	(function Main (Int32 id) -> (Int32 exitCode):
		(Thing thing)
		(thing.maJig.
	)
)<CODE>";

	printf("*** Start of %s ***\n", __FUNCTION__);

	printf("File: %s\n", file);

	Substring sfile = { file, file + strlen(file) };

	cstr lastS = Strings::ReverseFind('t', sfile);

	Substring sb = { lastS, lastS + 12 };

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;
		int typeCount = 0;

		void OnField(cstr fieldName) override
		{
			printf("Method: %s\n", fieldName);

			if (!StartsWith(fieldName, "maJig"))
			{
				Throw(0, "Bad inference '%s' - type should start with maJig", fieldName);
			}

			fieldCount++;
		}

		void OnHintFound(cr_substring hint)
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			hintCount++;
		}

		void OnFieldType(cr_substring fieldType, cr_substring searchRoot) override
		{
			if (!Eq(fieldType, "Vec3"))
			{
				Throw(0, "Bad inference '%s' - type should be Vec3", fieldType);
			}

			typeCount++;
		}
	} fieldEnumerator;

	Substring type;
	bool isThis;
	if (!(type = Rococo::Sexy::GetLocalTypeFromCurrentDocument(isThis, sb, sfile)))
	{
		if (!Eq(type, "Thing"))
		{
			Throw(0, "Bad inference '%s' - type should be Thing", type);
		}
	}

	Rococo::Sexy::EnumerateLocalFields(fieldEnumerator, sb, type, sfile);

	if (fieldEnumerator.fieldCount != 0)
	{
		Throw(0, "Bad inference '%d' - expecting 0 fields", fieldEnumerator.fieldCount);
	}

	if (fieldEnumerator.typeCount != 1)
	{
		Throw(0, "Bad inference '%d' - expecting 1 type", fieldEnumerator.typeCount);
	}

	printf("*** End of %s ***\n", __FUNCTION__);
}

struct SexyStudioEventHandler : ISexyStudioEventHandler
{
	bool TryOpenEditor(cstr filename, int lineNumber) override
	{
		return false;
	}

	EIDECloseResponse OnIDEClose(IWindow& topLevelParent) override
	{
		ShowWindow(topLevelParent, SW_HIDE);
		return EIDECloseResponse::Continue;
	}
};

static SexyStudioEventHandler static_SexyStudioEventHandler;

ISexyStudioFactory1* factory = nullptr;
ISexyStudioInstance1* sexyIDE = nullptr;

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

	topLevelWindow.hWnd = GetConsoleWindow();

	try
	{
		WideFilePath pathToDLL;
		GetDllPath(pathToDLL);

		static HMODULE hFactoryModule = LoadLibraryW(pathToDLL);
		if (hFactoryModule == nullptr)
		{
			Throw(GetLastError(), "Could not load library: %ls", pathToDLL.buf);
		}

		FARPROC proc = GetProcAddress(hFactoryModule, "CreateSexyStudioFactory");
		if (proc == nullptr)
		{
			Throw(GetLastError(), "Could not find CreateSexyStudioFactory in %ls", pathToDLL.buf);
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

class TestEditor : public AutoComplete::ISexyEditor, IAutoCompleteBuilder
{
public:
	Substring doc;
	int64 caretPos;

	TestEditor(fstring document, int64 _caretPos):
		doc { document.buffer, document.buffer + document.length }, caretPos(_caretPos)
	{
		
	}

	int64 GetDocLength() const override
	{
		return doc.Length();
	}

	int64 GetText(int64 len, char* buffer) override
	{
		memcpy(buffer, doc.start, min(len, doc.Length()));

		if (doc.Length() < len)
		{
			buffer[doc.Length()] = 0;
		}

		return min(len, doc.Length());
	}

	int64 GetCaretPos() const override
	{
		return caretPos;
	}

	void ShowCallTipAtCaretPos(cstr tip) const
	{
		printf("CallTip %s\n", tip);
	}

	void SetAutoCompleteCancelWhenCaretMoved() override
	{
		
	}

	void ShowAutoCompleteList(cstr spaceSeparatedItems) override
	{
		for (cstr p = spaceSeparatedItems; *p != 0; p++)
		{
			if (*p <= 32)
			{
				puts("\r\n");
			}
			else
			{
				putc(*p, stdout);
			}
		}
	}

	bool TryGetCurrentLine(EditorLine& line) const override
	{
		cstr previousNewLine = Strings::ReverseFind('\n', { doc.start, doc.start + caretPos });
		if (previousNewLine)
		{
			previousNewLine++;
		}
		else
		{
			previousNewLine = doc.start;
		}

		cstr nextNewLine = Strings::ForwardFind('\n', { doc.start + caretPos, doc.finish });
		if (!nextNewLine)
		{
			return false;
		}

		int64 bufferLength = nextNewLine - previousNewLine;

		if (bufferLength == 0 || bufferLength >= (int64) line.MAX_LINE_LENGTH)
		{
			return false;
		}

		memcpy(line.Data(), previousNewLine, bufferLength);
		line.SetLength(bufferLength);

		return true;
	}

	void GetCursor(EditorCursor& cursor) const override
	{
		cursor.caretPos = caretPos;

		cstr final = doc.start + caretPos;

		int64 lineStartIndex = 0;
		int64 lineNumber = 0;
		for (cstr p = doc.start; p != doc.finish; ++p)
		{
			if (p == final)
			{
				break;
			}

			if (*p == '\n')
			{
				lineNumber++;
				lineStartIndex = p - doc.start + 1;
			}
		}

		cursor.lineNumber = lineNumber;
		cursor.lineStartPosition = lineStartIndex;
		cursor.caretColumn = cursor.caretPos - cursor.lineStartPosition;
	}

	void ReplaceText(int64 startPos, int64 endPos, cstr item) const override
	{
		printf("Replace (%lld, %lld) with %s\n", startPos, endPos, item);
	}

	IAutoCompleteBuilder& AutoCompleteBuilder() override
	{
		return *this;
	}

	void AddItem(cstr item) override
	{
		printf("Item: %s\n", item);
	}

	void ShowAndClearItems() override
	{
		printf("ShowAndClearItems\n");
	}
};

void RunTests(ISexyDatabase& database)
{
	printf("Running tests...\n");

	TestDeduceVec2Fields(database);
	TestDeduceVec2Fields2(database);
	TestDeduceMethods(database);
	TestDeduceMethods2(database);
	TestDeduceMatrix4x4Fields(database);
	TestHintVec2(database);
	TestLocalStruct(database);
	TestLocalStruct2(database);
}

struct FileDesc
{
	fstring fileText;
	char caretFinalChar;
	FileDesc(cstr _fileText, char _caretFinalChar): fileText(to_fstring(_fileText)), caretFinalChar(_caretFinalChar)
	{

	}

	const fstring& Text()
	{
		return fileText;
	}

	Substring Doc() const
	{
		return Substring{ fileText.buffer, fileText.buffer + fileText.length };
	}

	int64 CaretPos() const
	{
		cstr caretPos = ReverseFind(caretFinalChar, Doc());
		if (caretPos == nullptr)
		{
			Throw(0, "Bad caret pos, could not find final char in file. All specimen files should have a dangling dot");
		}

		return caretPos - fileText.buffer + 1;
	}
};

void TestFullEditor_SearchLocalStructForInterface()
{
	cstr file =
		R"<CODE>((using Sys.Type)
(using Sys.Maths)

(struct EventObjectArg
	(IString typename)
	(IString variableName)
	(IString defaultValue)
)

(struct EventObject
	(IString evNamespace)
	(IString evName)
	(array EventObjectArg args)
)

(function Main (Int32 id) -> (Int32 exitCode):
	(EventObject obj)
	(obj.evNamespace.Length 
	(IString cat)
	(cat.Length 
)
)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->UpdateAutoComplete(editor);
}

void TestFullEditor_SearchLocalStructForInterfaceMethod()
{
	cstr file =
		R"<CODE>((using Sys.Type)
(using Sys.Maths)

(struct EventObjectArg
	(IString typename)
	(IString variableName)
	(IString defaultValue)
)

(struct EventObject
	(IString evNamespace)
	(IString evName)
	(array EventObjectArg args)
)

(function Main (Int32 id) -> (Int32 exitCode):
	(EventObject obj)
	(obj.evNamespace.Length 
)

)<CODE>";

	FileDesc desc(file, ' ');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->UpdateAutoComplete(editor);
}

void MainProtected2(HMODULE hLib)
{
	pluginInit(NULL);
	TestFullEditor_SearchLocalStructForInterfaceMethod();
//	TestFullEditor_SearchLocalStructForInterface();
}

void MainProtected(HMODULE hLib)
{
	FARPROC proc = GetProcAddress(hLib, "CreateSexyStudioFactory");
	if (proc == nullptr)
	{
		Throw(GetLastError(), "Could not find CreateSexyStudioFactory in %ls", DLL_NAME);
	}

	auto CreateSexyStudioFactory = (Rococo::SexyStudio::FN_CreateSexyStudioFactory)proc;

	cstr interfaceURL = "Rococo.SexyStudio.ISexyStudioFactory1";

	AutoFree<ISexyStudioFactory1> factory;
	int nErr = CreateSexyStudioFactory((void**)&factory, interfaceURL);
	if (nErr == 0)
	{
		struct ANON : ISexyStudioEventHandler
		{
			bool TryOpenEditor(cstr filename, int lineNumber) override
			{
				return false;
			}

			EIDECloseResponse OnIDEClose(IWindow& topLevelParent) override
			{
				return EIDECloseResponse::Shutdown;
			}
		} eventHandler;

		AutoFree<ISexyStudioInstance1> instance = factory->CreateSexyIDE(Rococo::Windows::NoParent(), eventHandler);

		ISexyDatabase& database = instance->GetDatabase();

		RunTests(database);
	}
	else
	{
		Throw(nErr, "CreateSexyStudioFactory did not recognize interface %s", interfaceURL);
	}
}

int main()
{
	WideFilePath directory;
	if (!GetModuleFileNameW(NULL, directory.buf, directory.CAPACITY)) return GetLastError();
	Rococo::OS::StripLastSubpath(directory.buf);

	// path now contains the directory
	WideFilePath pathToDLL;
	Format(pathToDLL, L"%ls%ls", directory.buf, DLL_NAME);

	auto hLib = LoadLibraryW(pathToDLL);
	if (hLib == nullptr)
	{
		U8FilePath msg;
		Format(msg, "Could not load library: %ls", pathToDLL.buf);
		MessageBoxA(NULL, msg, ErrorCaption, MB_ICONERROR);
		return GetLastError();
	}

	int errorCode = 0;

	try
	{
	//	MainProtected(hLib);
		MainProtected2(hLib);
	}
	catch (IException& ex)
	{
		Rococo::OS::ShowErrorBox(Rococo::Windows::NoParent(), ex, "SexyStudio Standalone App error");
		errorCode = ex.ErrorCode();
	}

	FreeLibrary(hLib);

	return errorCode;
}

