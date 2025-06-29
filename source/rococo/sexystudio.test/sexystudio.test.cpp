#include <rococo.os.win32.h>
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.sxytype-inference.h>
#include <rococo.sexystudio.api.h>
#include <rococo.functional.h>
#include <rococo.auto-complete.h>
#include <rococo.window.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::SexyStudio;
using namespace Rococo::AutoComplete;
using namespace Rococo::Windows;

auto DLL_NAME = L"sexystudio.dll";

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

	printf("*** Start of %s ***\n", __ROCOCO_FUNCTION__);

	printf("File: %s\n", file);

	struct ANON : ISexyFieldEnumerator
	{
		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
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
			UNUSED(fieldName);
			UNUSED(searchRoot);
		}
	} fieldEnumerator;

	Rococo::Sex::Inference::TypeInference type;
	bool isThis;
	type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(OUT isThis, v, sfile);

	if (!type.declarationType)
	{
		Throw(0, "Bad inference - type should be Vec2");
	}

	database.EnumerateVariableAndFieldList(v, type.declarationType, fieldEnumerator);

	printf("*** End of %s ***\n", __ROCOCO_FUNCTION__);
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

	printf("*** Start of %s ***\n", __ROCOCO_FUNCTION__);

	printf("File: %s\n", file);

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
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
			UNUSED(fieldName);
			UNUSED(searchRoot);
		}
	} fieldEnumerator;

	Rococo::Sex::Inference::TypeInference type;
	bool isThis;
	type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(OUT isThis, v, sfile);

	if (!type.declarationType)
	{
		Throw(0, "Bad inference - type should be Vec2");
	}

	database.EnumerateVariableAndFieldList(v, type.declarationType, fieldEnumerator);

	if (fieldEnumerator.hintCount != 1) Throw(0, "Bad hint count: %s", __ROCOCO_FUNCTION__);
	if (fieldEnumerator.fieldCount != 2) Throw(0, "Bad field count: %s", __ROCOCO_FUNCTION__);

	printf("*** End of %s ***\n", __ROCOCO_FUNCTION__);
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

	printf("*** Start of %s ***\n", __ROCOCO_FUNCTION__);

	printf("File: %s\n", file);

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
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
			UNUSED(fieldName);
			UNUSED(searchRoot);
		}
	} fieldEnumerator;

	Rococo::Sex::Inference::TypeInference type;
	bool isThis;
	type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(OUT isThis, v, sfile);

	if (!type.declarationType)
	{
		Throw(0, "Bad inference - type should be Vec4");
	}

	database.EnumerateVariableAndFieldList(v, type.declarationType, fieldEnumerator);

	if (fieldEnumerator.fieldCount != 4) Throw(0, "Bad fieldCount: %s", __ROCOCO_FUNCTION__);
	if (fieldEnumerator.hintCount == 0) Throw(0, "Bad hint count: %s", __ROCOCO_FUNCTION__);

	printf("*** End of %s ***\n", __ROCOCO_FUNCTION__);
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

	printf("*** Start of %s ***\n", __ROCOCO_FUNCTION__);

	printf("File: %s\n", file);

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		bool found = false;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
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
			UNUSED(fieldName);
			UNUSED(searchRoot);
		}
	} fieldEnumerator;

	database.ForEachAutoCompleteCandidate(v, fieldEnumerator);

	if (!fieldEnumerator.found)
	{
		Throw(0, "Could not find Vec2 in Sys.Maths.");
	}

	printf("*** End of %s ***\n", __ROCOCO_FUNCTION__);
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

	printf("*** Start of %s ***\n", __ROCOCO_FUNCTION__);

	printf("File: %s\n", file);

	Substring sfile = { file, file + strlen(file) };

	cstr lastS = Strings::ReverseFind('s', sfile);

	Substring sb = { lastS, lastS + 3 };

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
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
			UNUSED(fieldName);
			UNUSED(searchRoot);
		}
	} fieldEnumerator;

	Rococo::Sex::Inference::TypeInference type;
	bool isThis;
	type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(OUT isThis, sb, sfile);

	if (!type.declarationType || !Eq(type.declarationType, "IStringBuilder"))
	{
		Throw(0, "Bad inference - type should be IStringBuilder");
	}

	database.EnumerateVariableAndFieldList(sb, type.declarationType, fieldEnumerator);

	// We give a bit of range on this method in the case that IStringBuilder is modified in the future
	if (fieldEnumerator.fieldCount < 15 || fieldEnumerator.fieldCount > 30) Throw(0, "Bad fieldCount: %s", __ROCOCO_FUNCTION__);
	if (fieldEnumerator.hintCount != 1) Throw(0, "Bad hint count: %s", __ROCOCO_FUNCTION__);

	printf("*** End of %s ***\n", __ROCOCO_FUNCTION__);
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

	printf("*** Start of %s ***\n", __ROCOCO_FUNCTION__);

	printf("File: %s\n", file);

	Substring sfile = { file, file + strlen(file) };

	cstr lastS = Strings::ReverseFind('s', sfile);

	Substring sb = { lastS, lastS + 3 };

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
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
			UNUSED(fieldName);
			UNUSED(searchRoot);
		}
	} fieldEnumerator;

	Rococo::Sex::Inference::TypeInference type;
	bool isThis;
	type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(OUT isThis, sb, sfile);
	
	if (!type.declarationType || !Eq(type.declarationType, "Sys.Type.IStringBuilder"))
	{
		Throw(0, "Bad inference - type should be IStringBuilder");
	}

	database.EnumerateVariableAndFieldList(sb, type.declarationType, fieldEnumerator);

	// We give a bit of range on this method in the case that IStringBuilder is modified in the future
	if (fieldEnumerator.fieldCount < 15 || fieldEnumerator.fieldCount > 30) Throw(0, "Bad fieldCount: %s", __ROCOCO_FUNCTION__);
	if (fieldEnumerator.hintCount != 1) Throw(0, "Bad hint count: %s", __ROCOCO_FUNCTION__);

	printf("*** End of %s ***\n", __ROCOCO_FUNCTION__);
}

void TestLocalStruct(ISexyDatabase&)
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

	printf("*** Start of %s ***\n", __ROCOCO_FUNCTION__);

	printf("File: %s\n", file);

	Substring sfile = { file, file + strlen(file) };

	cstr lastS = Strings::ReverseFind('t', sfile);

	Substring sb = { lastS, lastS + 6 };

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
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
			UNUSED(fieldName);
			UNUSED(searchRoot);
		}
	} fieldEnumerator;

	Rococo::Sex::Inference::TypeInference type;
	bool isThis;
	type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(OUT isThis, sb, sfile);

	if (!type.declarationType)
	{
		if (!Eq(type.declarationType, "Thing"))
		{
			char typeStr[128];
			type.declarationType.CopyWithTruncate(typeStr, sizeof typeStr);
			Throw(0, "Bad inference '%s' - type should be Thing", typeStr);
		}
	}

	Rococo::Sex::Inference::EnumerateLocalFields(fieldEnumerator, sb, type.declarationType, sfile);

	if (fieldEnumerator.fieldCount != 3)
	{
		Throw(0, "Bad inference '%d' - expecting one field of Thing", fieldEnumerator.fieldCount);
	}

	printf("*** End of %s ***\n", __ROCOCO_FUNCTION__);
}

void TestLocalStruct2(ISexyDatabase&)
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

	printf("*** Start of %s ***\n", __ROCOCO_FUNCTION__);

	printf("File: %s\n", file);

	Substring sfile = { file, file + strlen(file) };

	cstr lastS = Strings::ReverseFind('t', sfile);

	Substring sb = { lastS, lastS + 12 };

	struct ANON : ISexyFieldEnumerator
	{
		int fieldCount = 0;
		int hintCount = 0;
		int typeCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
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
			UNUSED(searchRoot);
			if (!Eq(fieldType, "Vec3"))
			{
				char typeStr[128];
				fieldType.CopyWithTruncate(typeStr, sizeof typeStr);
				Throw(0, "Bad inference '%s' - type should be Vec3", typeStr);
			}

			typeCount++;
		}
	} fieldEnumerator;

	Rococo::Sex::Inference::TypeInference type;
	bool isThis;
	type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(OUT isThis, sb, sfile);

	if (!type.declarationType)
	{
		if (!Eq(type.declarationType, "Thing"))
		{
			char typeStr[128];
			type.declarationType.CopyWithTruncate(typeStr, sizeof typeStr);
			Throw(0, "Bad inference '%s' - type should be Thing", typeStr);
		}
	}

	Rococo::Sex::Inference::EnumerateLocalFields(fieldEnumerator, sb, type.declarationType, sfile);

	if (fieldEnumerator.fieldCount != 0)
	{
		Throw(0, "Bad inference '%d' - expecting 0 fields", fieldEnumerator.fieldCount);
	}

	if (fieldEnumerator.typeCount != 1)
	{
		Throw(0, "Bad inference '%d' - expecting 1 type", fieldEnumerator.typeCount);
	}

	printf("*** End of %s ***\n", __ROCOCO_FUNCTION__);
}


struct SexyStudioEventHandler : ISexyStudioEventHandler
{
	bool TryOpenEditor(cstr filename, int lineNumber) override
	{
		UNUSED(filename);
		UNUSED(lineNumber);
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
	LSTATUS status = RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Rococo - 19th Century Software\\SexyStudio", &hSexy4Npp);
	if (status == ERROR_SUCCESS)
	{
		enum { MAX_ROOT_LEN = 128 };
		static_assert((int) MAX_ROOT_LEN < (int) WideFilePath::CAPACITY);

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
		Format(pathToDLL, L"C:\\work\\rococo\\SexyStudioBin\\sexystudio.dll");
	}
}

void pluginInit(HANDLE /* hModule */)
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
			sexyIDE->Gui().SetTitle("SexyStudio For Notepad++");
		}
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(topLevelWindow, ex, ErrorCaption);
	}
}

class TestEditor : public AutoComplete::ISexyEditor, IAutoCompleteBuilder
{
public:
	Substring doc;
	int64 caretPos;

	HString lastDefinedPath;
	int lastDefinitionLineNumber = 0;

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

	cstr LastDefinitionFile()
	{
		return lastDefinedPath.c_str();
	}

	int LastDefintionLineNumber()
	{
		return lastDefinitionLineNumber;
	}

	void GotoDefinition(cstr searchToken, const char* path, int lineNumber) override
	{
		lastDefinedPath = path;
		lastDefinitionLineNumber = lineNumber;

		if (sexyStudio)
		{
			struct: IPreviewEventHandler
			{
				void OnBackButtonClicked() override
				{

				}

				bool OnJumpToCode(const char* path, int lineNumber) override
				{
					UNUSED(path);
					UNUSED(lineNumber);
					return true;
				}
			} eventHandler;

			sexyStudio->Gui().PopupPreview(sexyStudio->Gui().GetIDEFrame(), searchToken, path, lineNumber, eventHandler);
		}
		
		printf("\tGotoDefinition('%s', %d)\n", path, lineNumber);
	}

	void ShowCallTipAtCaretPos(cstr tip) const
	{
		printf("\tCallTip %s\n", tip);
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

	void AddHint(cr_substring item) override
	{
		char hintBuffer[1024];
		item.CopyWithTruncate(hintBuffer, sizeof hintBuffer);
		printf("Hint: %s\n", hintBuffer);
	}

	void AddItem(cstr item) override
	{
		printf("Item: %s\n", item);
	}

	void ShowAndClearItems() override
	{
		printf("<--- ShowAndClearItems --->\n");
	}

	ISexyStudioInstance1* sexyStudio = nullptr;

	void SetHostWindow(ISexyStudioInstance1* sexyStudio)
	{
		this->sexyStudio = sexyStudio;
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

void Intro(cstr text)
{
	printf("<<<<<<< %s\n", text);
}

void TestFullEditor_SearchLocalStructForInterface()
{
	Intro(__ROCOCO_FUNCTION__);

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

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchLocalStructForInterfaceMethod()
{
	Intro(__ROCOCO_FUNCTION__);

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

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchLocalStructForM4x4()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
 (alias Main EntryPoint.Main)

(using Sys.Type)

(struct EventObject
	(IString name)
	(Matrix4x4 m)
)

(function Main -> (Int32 exitCode):
	(EventObject obj)
	(obj.m.
))<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchLocalStructForIString()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
 (alias Main EntryPoint.Main)

(using Sys.Type)

(struct EventObject
	(IString name)
	(Matrix4x4 m)
)

(function Main -> (Int32 exitCode):
	(EventObject obj)
	(obj.n
))<CODE>";

	FileDesc desc(file, 'n');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchLocalStructForIStringAndDot()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
 (alias Main EntryPoint.Main)

(using Sys.Type)

(struct EventObject
	(IString name)
	(Matrix4x4 m)
)

(function Main -> (Int32 exitCode):
	(EventObject obj)
	(obj.name.
))<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchIStringInLocalStructInLocalStruct()
{
	Intro(__ROCOCO_FUNCTION__);

cstr file =
		R"<CODE>((namespace EntryPoint)
 (alias Main EntryPoint.Main)

(using Sys.Type)

(struct Dog
	(IString bark)
)

(struct EventObject
	(Dog dog)
)

(function Main -> (Int32 exitCode):
	(EventObject obj)
	(obj.dog.
))<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchIStringInLocalStructInLocalStructWithMethodCue()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(using Sys.Type)

(struct Dog
	(IString bark)
)

(struct EventObject
	(Dog dog)
)

(function Main -> (Int32 exitCode):
	(EventObject obj)
	(obj.dog.bark.
))<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchIStringInLocalStructInLocalStructWithMethodCompleteCue()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(using Sys.Type)

(struct Dog
	(IString bark)
)

(struct EventObject
	(Dog dog)
)

(function Main -> (Int32 exitCode):
	(EventObject obj)
	(obj.dog.bark.Length 
))<CODE>";

	FileDesc desc(file, ' ');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_ReturnIStringCompleteCue_FromIStringBuilder()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(using Sys.Type)

(function Main -> (Int32 exitCode):
	(IStringBuilder sb)
	(sb.Length 
))<CODE>";

	FileDesc desc(file, ' ');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchFQType()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(function Main -> (Int32 exitCode):
	(Sys.Maths.Matrix4x4 m)
	(m.
))<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchForFactories()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(function Main -> (Int32 exitCode):
	(Sys.Type.IException ex 
))<CODE>";

	FileDesc desc(file, ' ');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchForFactories2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)
(using Sys.Type)

(function Main -> (Int32 exitCode):
	(IException ex 
))<CODE>";

	FileDesc desc(file, ' ');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_GotoDefinitionFail()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)
(using Sys.Type)

// This will *not* yield a definition. The type inference system does not match identifiers to type at the point of creation. E.g (IException ex) does not match ex to IException, as this would cause unwanted code prompts/completion in the constructor. 
// ex will only get matched to IException following construction/declaration
(function Main -> (Int32 exitCode):
	(IException ex 
))<CODE>";

	FileDesc desc(file, 'x');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);
}

void TestFullEditor_GotoDefinitionAtFunction()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)
(using Sys.Type)

// This *will* yield a definition. The type inference system match identifiers to type after the point of creation. E.g (IException ex) match ex to IException, as long as the string 'ex' is used after the close parenthesis
(function Main (IException ex) -> (Int32 exitCode):
	(ex 
))<CODE>";

	FileDesc desc(file, 'x');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!Eq(editor.LastDefinitionFile(), "<this>"))
	{
		Throw(0, "Expecting <this>");
	}

	if (editor.LastDefintionLineNumber() != 6)
	{
		Throw(0, "Expecting line 6");
	}
}

void TestFullEditor_GotoDefinitionFail2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)
(using Sys.Type)
	(expialodocious 
))<CODE>";

	FileDesc desc(file, 'x');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!Eq(editor.LastDefinitionFile(), ""))
	{
		Throw(0, "Expecting blank");
	}

	if (editor.LastDefintionLineNumber() != 0)
	{
		Throw(0, "Expecting line 0");
	}
}

void TestFullEditor_GotoDefinitionOfInterface()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)
(using Sys.Type)
	(IException ex 
))<CODE>";

	FileDesc desc(file, 'o'); // This should match IException and goto the definition of Sys.Type.IException
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!EndsWith(editor.LastDefinitionFile(), "Sys.Type.sxy"))
	{
		Throw(0, "Expecting blank");
	}

	if (editor.LastDefintionLineNumber() < 0)
	{
		Throw(0, "Expecting line 0");
	}
}

void TestFullEditor_GotoDefinitionOfInterface2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)
(using Sys.Type)
	(Sys.Type.IException ex 
))<CODE>";

	FileDesc desc(file, 'o'); // This should match IException and goto the definition of Sys.Type.IException
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!EndsWith(editor.LastDefinitionFile(), "Sys.Type.sxy"))
	{
		Throw(0, "Expecting Sys.Type.sxy");
	}

	if (editor.LastDefintionLineNumber() < 0)
	{
		Throw(0, "Expecting line > 0");
	}
}

void TestFullEditor_GotoDefinitionOfInterface3()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(using Sys)
(using Sys.Type)
	(Type.IException ex 
))<CODE>";

	FileDesc desc(file, 'o'); // This should match IException and goto the definition of Sys.Type.IException
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!EndsWith(editor.LastDefinitionFile(), "Sys.Type.sxy"))
	{
		Throw(0, "Expecting Sys.Type.sxy");
	}

	if (editor.LastDefintionLineNumber() < 1)
	{
		Throw(0, "Expecting line > 0");
	}
}

void TestFullEditor_GotoDefinitionOfStruct()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(using Sys)
(using Sys.Type)
	(Vec3 ex 
))<CODE>";

	FileDesc desc(file, '3'); // This should match Vec3 and goto the definition of Sys.Maths.Vec3
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!EndsWith(editor.LastDefinitionFile(), "Sys.Maths.sxy"))
	{
		Throw(0, "Expecting Sys.Maths.sxy");
	}

	if (editor.LastDefintionLineNumber() <= 0)
	{
		Throw(0, "Expecting line > 0");
	}
}

void TestFullEditor_GotoDefinitionOfFunction()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(using Sys)
(using Sys.Type)
	(Float32 ooroot2 = Sin 45)
))<CODE>";

	FileDesc desc(file, 'i'); // This should match Sin and goto the definition of Sin
	TestEditor editor(desc.Text(), desc.CaretPos());

	editor.SetHostWindow(sexyIDE);

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!EndsWith(editor.LastDefinitionFile(), "sys.maths.f32.inl"))
	{
		Throw(0, "Expecting sys.maths.f32.inl");
	}

	if (editor.LastDefintionLineNumber() <= 0)
	{
		Throw(0, "Expecting line > 0");
	}
}

void TestFullEditor_GotoDefinitionOfFunction2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(using Sys)
(using Sys.Type)
	(Float32 ooroot2 = F64.Sin 45)
))<CODE>";

	FileDesc desc(file, 'i'); // This should match Sin and goto the definition of F64.Sin
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!EndsWith(editor.LastDefinitionFile(), "sys.maths.f64.inl"))
	{
		Throw(0, "Expecting sys.maths.f32.inl");
	}

	if (editor.LastDefintionLineNumber() <= 0)
	{
		Throw(0, "Expecting line > 0");
	}
}

void TestFullEditor_GotoDefinitionOfFunction3()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(using Sys)
(using Sys.Type)
	(Float32 ooroot2 = Sys.Maths.F64.Sin 45)
))<CODE>";

	FileDesc desc(file, 'i'); // This should match Sys.Maths.F64.Sin and goto the definition of Sys.Maths.F32.Sin
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!EndsWith(editor.LastDefinitionFile(), "sys.maths.f64.inl"))
	{
		Throw(0, "Expecting sys.maths.f32.inl");
	}

	if (editor.LastDefintionLineNumber() <= 0)
	{
		Throw(0, "Expecting line > 0");
	}
}

void TestFullEditor_GotoDefinitionOfStruct2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(using Sys)
(using Sys.Type)
	(Maths.Vec3 ex 
))<CODE>";

	FileDesc desc(file, '3'); // This should match Vec3 and goto the definition of Sys.Maths.Vec3
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!EndsWith(editor.LastDefinitionFile(), "Sys.Maths.sxy"))
	{
		Throw(0, "Expecting Sys.Maths.sxy");
	}

	if (editor.LastDefintionLineNumber() <= 0)
	{
		Throw(0, "Expecting line > 0");
	}
}

void TestFullEditor_GotoDefinitionOfStruct3()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)

(using Sys)
(using Sys.Type)
	(Sys.Maths.Vec3 ex 
))<CODE>";

	FileDesc desc(file, '3'); // This should match Vec3 and goto the definition of Sys.Maths.Vec3
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().GotoDefinitionOfSelectedToken(editor);

	if (!EndsWith(editor.LastDefinitionFile(), "Sys.Maths.sxy"))
	{
		Throw(0, "Expecting Sys.Maths.sxy");
	}

	if (editor.LastDefintionLineNumber() <= 0)
	{
		Throw(0, "Expecting line > 0");
	}
}

void TestFullEditor_SearchForMacro()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>((namespace EntryPoint)
(alias Main EntryPoint.Main)
(using Sys.Type)

(function Main -> (Int32 exitCode):
	(#E
))<CODE>";

	FileDesc desc(file, 'E');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestComplexCase1()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
(method OverlayButtonHandler.BuildMenus -> :
(IGuiOverlay overlay(MHost.GuiOverlay))
(overlay.ClearMenus)

(Sys.Reflection.IStructure t = typeof MHost.Events.IOverlayEventHandler)
(Int32 nMethods = t.GetMethodCount)

(Sys.Type.IStringBuilder methodName = (Sys.Type.NewTokenBuilder))
(Sys.Type.IStringBuilder buttonName = (Sys.Type.NewTokenBuilder))

(for (Int32 i = 0)(i < nMethods)(i += 1)
	(methodName.Clear)
	(buttonName.Clear)
	(t.AppendMethodName methodName 0 i)
	(if (Sys.Type.Strings.StartsWith methodName "On"))
	(Int32 clickedPos = (Sys.Type.Strings.FindRightWithCase methodName 2 "Clicked"))
	(if (clickedPos > 0)
		(methodName.
))<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestSuggestAtDot()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
(method Do -> :
	(Rococo.Graphics.IQuadStackTesselator qst)

	(qst.
))<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestSuggestBeyondDot()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
(method Do -> :
	(Rococo.Graphics.IQuadStackTesselator qst)

	(qst.Move
))<CODE>";

	FileDesc desc(file, 'e');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestThisArrayInference()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

R"<CODE>(

(interface Sys.IMagic
	(Enchant (Int32 id)->(Int32 value))
)

(class Magic (implements Sys.IMagic)
	(array Int32 berries)
)

(method Magic.Enchant (Int32 id)->(Int32 value):
	(this.berries.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestLocalArrayInference()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

		R"<CODE>(

(function Main (Int32 id)->(Int32 value):
	(array Int32 berries 2)
	(berries.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestLocalArrayInference2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

		R"<CODE>(

(function Main (Int32 id)->(Int32 value):
	(array Sys.Type.Int32 berries 2)
	(berries.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestLocalListInference()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

		R"<CODE>(

(function Main (Int32 id)->(Int32 value):
	(list Int32 berries 2)
	(berries.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestLocalListInference2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

		R"<CODE>(

(function Main (Int32 id)->(Int32 value):
	(list Sys.Type.Int32 berries 2)
	(berries.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestLocalMapInference()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

		R"<CODE>(

(function Main (Int32 id)->(Int32 value):
	(map Int32 IString berries 2)
	(berries.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestLocalMapInference2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

		R"<CODE>(

(function Main (Int32 id)->(Int32 value):
	(map Sys.Type.Int32 Sys.Type.IString berries 2)
	(berries.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}


void TestThisArrayInference2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

R"<CODE>(

(class SEXMLReader 
	(implements Sys.Sexml.ISEXMLReader)
	
	(IExpression sexmlRoot)
	(array Sys.Sexml.ISEXMLDirective rootDirectives)
	(Int32 nDirectives)
)

(method SEXMLReader.DirectiveCount -> (Int32 numberOfDirectives):
	(numberOfDirectives = this.nDirectives)
	(this.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestThisMapInference()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

		R"<CODE>(

(interface Sys.IMagic
	(Enchant (Int32 id)->(Int32 value))
)

(class Magic (implements Sys.IMagic)
	(map Int32 IString berries)
)

(method Magic.Enchant (Int32 id)->(Int32 value):
	(this.berries.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestThisMapInference2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

		R"<CODE>(

(interface Sys.IMagic
	(Enchant (Int32 id)->(Int32 value))
)

(class Magic (implements Sys.IMagic)
	(map Sys.Type.Int32 Sys.Type.IString berries)
)

(method Magic.Enchant (Int32 id)->(Int32 value):
	(this.berries.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestThisListInference()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

		R"<CODE>(

(interface Sys.IMagic
	(Enchant (Int32 id)->(Int32 value))
)

(class Magic (implements Sys.IMagic)
	(list Int32 berries)
)

(method Magic.Enchant (Int32 id)->(Int32 value):
	(this.berries.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestThisListInference2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =

		R"<CODE>(

(class SEXMLReader 
	(implements Sys.Sexml.ISEXMLReader)
	
	(IExpression sexmlRoot)
	(list Sys.Sexml.ISEXMLDirective rootDirectives)
	(Int32 nDirectives)
)

(method SEXMLReader.DirectiveCount -> (Int32 numberOfDirectives):
	(numberOfDirectives = this.nDirectives)
	(this.

)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFullEditor_SearchFQType2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
(namespace EntryPoint)
(alias Main EntryPoint.Main)

(function Main -> (Int32 exitCode):
	(Sys.Maths.Matrix4x4 m)
	(m.r
)
)<CODE>";

	FileDesc desc(file, 'r');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestDeepMemberInference()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
	(method Dog.Bark -> :
		(Rococo.QuadVertices topQuad)
		(topQuad.positions.a = -0.1  0.1 0.1)
		(topQuad.positions.b = 0.1  0.1 0.1)
		(topQuad.positions.c = 0.1 - 0.1 0.1)
		(topQuad.positions.d = -0.1 - 0.1 0.1)
		(topQuad.uv = 0 0 1 1)
		(topQuad.normals = up up up up)
		(topQuad.colours = white white white white)
		(topQuad.positions.a
	))<CODE>";

	FileDesc desc(file, 'a');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestDeepMemberInference2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
	(method Dog.Bark -> :
		(Rococo.QuadVertices topQuad)
		(topQuad.positions.a = -0.1  0.1 0.1)
		(topQuad.positions.b = 0.1  0.1 0.1)
		(topQuad.positions.c = 0.1 - 0.1 0.1)
		(topQuad.positions.d = -0.1 - 0.1 0.1)
		(topQuad.uv = 0 0 1 1)
		(topQuad.normals = up up up up)
		(topQuad.colours = white white white white)
		(topQuad.positions.
	))<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestFQStruct()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
R"<CODE>(
	(method Dog.Bark -> :
		(Rococo.Graphics.SoftBoxVertex v)
		(v.
	)
)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}


void TestNamespaceAppendsStruct()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
	(method Dog.Bark -> :
		(Rococo.Graphics.
	)
)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestPromptForMacro()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
	(function Bark -> :
		(#S
	)
)<CODE>";

	FileDesc desc(file, 'S');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestPromptForMacro2()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
	(function Bark -> :
		(#Sys
	)
)<CODE>";

	FileDesc desc(file, 's');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestPromptForMacro3()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
	(function Bark -> :
		(#Sys.
	)
)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestPromptForMacro4()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
	(function Bark -> :
		(#Rococo.Graphics.
	)
)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestPromptForMacro5()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
	(function Bark -> :
		(#Sys.Type.
	)
)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestPromptForMacro6()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
	(function Bark -> :
		(#Sys.Type.build )
	)
)<CODE>";

	FileDesc desc(file, ' '); // Ensure the <CODE> section has space after the build token but not after the build token's parenthesis
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestShowTipForFunction()
{
	cstr file =
		R"<CODE>("
	(function Main -> :
		(Sys.IO.GetCmdArgCount 
	)
)<CODE>";

	FileDesc desc(file, ' '); // Ensure the <CODE> section has space after the GetCmdArgCount token but not after the build token's parenthesis
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestShowCompletionsForStruct()
{
	cstr file = 
R"<CODE>(
(struct DommeContext
	(Sys.Type.IStringBuilder cppBuilder)
	(Sys.Type.IStringBuilder headerBuilder)
	(IString factoryPrefix)
)

(function Meow -> :
	(DommeContext dc)
	(dc.
)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void TestShowCompletionsForeachReferenceVariable()
{
	cstr file =
		R"<CODE>(

(function Meow -> :
	(array IString names 1)
	(foreach name # names
		(name.
)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);
}

void MainProtected2(HMODULE /* hLib */)
{
	pluginInit(NULL);

	goto skip;
	TestFullEditor_GotoDefinitionOfFunction();

	TestFullEditor_GotoDefinitionOfFunction2();
	TestFullEditor_GotoDefinitionOfFunction3();

	TestFullEditor_GotoDefinitionOfStruct();
	TestFullEditor_GotoDefinitionOfStruct2();
	TestFullEditor_GotoDefinitionOfStruct3();

	TestFullEditor_GotoDefinitionFail2();
	TestFullEditor_GotoDefinitionOfInterface();
	TestFullEditor_GotoDefinitionOfInterface2();
	TestFullEditor_GotoDefinitionOfInterface3();
	TestFullEditor_GotoDefinitionOfStruct();

	TestPromptForMacro();
	TestShowTipForFunction();

	TestPromptForMacro2();
	TestPromptForMacro3();
	TestPromptForMacro4();
	TestPromptForMacro5();
	TestPromptForMacro6();

	TestNamespaceAppendsStruct();
	TestFQStruct();

	TestDeepMemberInference2();
	TestDeepMemberInference();
	TestFullEditor_SearchFQType();
	TestFullEditor_SearchIStringInLocalStructInLocalStructWithMethodCue();
	TestFullEditor_SearchFQType2();
	TestSuggestAtDot();
	TestSuggestBeyondDot();
	TestFullEditor_SearchForMacro();
	TestFullEditor_ReturnIStringCompleteCue_FromIStringBuilder();
	TestFullEditor_SearchIStringInLocalStructInLocalStructWithMethodCompleteCue();
	TestFullEditor_SearchIStringInLocalStructInLocalStructWithMethodCue();
	TestFullEditor_SearchIStringInLocalStructInLocalStruct();
	TestFullEditor_SearchLocalStructForIStringAndDot();
	TestFullEditor_SearchLocalStructForIString();
	TestFullEditor_SearchLocalStructForM4x4();
	TestFullEditor_SearchLocalStructForInterfaceMethod();
	TestFullEditor_SearchLocalStructForInterface();
	TestFullEditor_SearchForFactories();
	TestFullEditor_SearchForFactories2();
	TestFullEditor_GotoDefinitionFail();
	TestFullEditor_GotoDefinitionAtFunction();

	TestComplexCase1();

	TestThisArrayInference();
	TestThisArrayInference2();

	TestThisMapInference();
	TestThisMapInference2();

	TestThisListInference();
	TestThisListInference2();

	TestLocalArrayInference();
	TestLocalArrayInference2();

	TestLocalListInference();
	TestLocalListInference2();

	TestLocalMapInference();
	TestLocalMapInference2();

	TestShowCompletionsForStruct();

skip:
	TestShowCompletionsForeachReferenceVariable();
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

	AutoFree<ISexyStudioFactory1> lfactory;
	int nErr = CreateSexyStudioFactory((void**)&lfactory, interfaceURL);
	if (nErr == 0)
	{
		struct ANON : ISexyStudioEventHandler
		{
			bool TryOpenEditor(cstr filename, int lineNumber) override
			{
				UNUSED(filename);
				UNUSED(lineNumber);
				return false;
			}

			EIDECloseResponse OnIDEClose(IWindow& topLevelParent) override
			{
				UNUSED(topLevelParent);
				return EIDECloseResponse::Shutdown;
			}
		} eventHandler;

		AutoFree<ISexyStudioInstance1> instance = lfactory->CreateSexyIDE(Rococo::Windows::NoParent(), eventHandler);

		ISexyDatabase& database = instance->GetDatabase();

		RunTests(database);
	}
	else
	{
		Throw(nErr, "CreateSexyStudioFactory did not recognize interface %s", interfaceURL);
	}
}

#include <rococo.debugging.h>

int main()
{
	WideFilePath directory;
	if (!GetModuleFileNameW(NULL, directory.buf, directory.CAPACITY)) return GetLastError();
	Rococo::IO::StripLastSubpath(directory.buf);

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
		Rococo::Debugging::ValidateCriticalLog();
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(Rococo::Windows::NoParent(), ex, "SexyStudio Standalone App error");
		errorCode = ex.ErrorCode();
	}

	FreeLibrary(hLib);

	return errorCode;
}

