#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <rococo.target.h>
#include <Windows.h>
#include <rococo.window.h>

#include <winerror.h>
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.sxytype-inference.h>
#include <rococo.sexystudio.api.h>
#include <rococo.functional.h>
#include <rococo.auto-complete.h>
#include <string.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::SexyStudio;
using namespace Rococo::AutoComplete;
using namespace Rococo::Windows;

auto DLL_NAME = L"sexystudio.dll";

cstr ErrorCaption = "SexyStudio Standalone App - error!";

#include <rococo.sexystudio.api.h>
#include "..\sexystudio-lib\sexystudio.api.h"
#include "..\sexystudio-lib\sexystudio.database.h"

#include <cstdio>
#include <vector>

// Records what the code-complete system emits, one line per event, so each test can assert on it:
//   "Field: <name>" and "Hint: <text>" from ISexyFieldEnumerator, "Item: <text>", "Hint: <text>" and "Show" (ShowAndClearItems)
//   from IAutoCompleteBuilder, "CallTip: <tip>", "List: <items>" and "Replace: (<start>, <end>) <text>" from ISexyEditor
struct EmitLog
{
	std::vector<HString> lines;

	void Add(cstr prefix, cr_substring text)
	{
		char buffer[1024];
		text.CopyWithTruncate(buffer, sizeof buffer);
		Add(prefix, buffer);
	}

	void Add(cstr prefix, cstr text)
	{
		HString line;
		Format(line, "%s%s", prefix, text);

		// Some member lookups report the same hint twice in a row. Notepad++ shows each hint as a call tip that replaces the last one,
		// so a repeat is invisible to the user. Collapse repeats so the expectations do not depend on them
		if (StartsWith(prefix, "Hint") && !lines.empty() && lines.back() == line)
		{
			return;
		}

		lines.push_back(line);
	}
};

// Asserts that the log matches the expected text exactly and in order. The expected text has one entry per line.
// Leading tabs and blank lines are ignored, so the expectation can be indented with the code.
// Exact order is safe: the database emits from sorted maps or fixed lists, and the order is the same on every run
void ValidateEmitted(cstr testName, const EmitLog& log, cstr expectedText)
{
	std::vector<HString> expected;

	cstr p = expectedText;
	while (*p != 0)
	{
		while (*p == '\t') p++;

		cstr lineEnd = p;
		while (*lineEnd != 0 && *lineEnd != '\n') lineEnd++;

		cstr finish = lineEnd;
		if (finish > p && finish[-1] == '\r') finish--;

		if (finish > p)
		{
			char buffer[1024];
			Substring{ p, finish }.CopyWithTruncate(buffer, sizeof buffer);
			expected.push_back(HString(buffer));
		}

		p = (*lineEnd == '\n') ? lineEnd + 1 : lineEnd;
	}

	size_t firstDifference = 0;
	while (firstDifference < expected.size() && firstDifference < log.lines.size() && expected[firstDifference] == log.lines[firstDifference])
	{
		firstDifference++;
	}

	if (firstDifference == expected.size() && firstDifference == log.lines.size())
	{
		return;
	}

	printf("!!! %s: emitted output differs from expected\n--- expected (%llu lines) ---\n", testName, (unsigned long long)expected.size());
	for (auto& line : expected) printf("%s\n", line.c_str());
	printf("--- actual (%llu lines) ---\n", (unsigned long long)log.lines.size());
	for (auto& line : log.lines) printf("%s\n", line.c_str());
	printf("---\n");

	cstr expectedLine = firstDifference < expected.size() ? expected[firstDifference].c_str() : "<nothing>";
	cstr actualLine = firstDifference < log.lines.size() ? log.lines[firstDifference].c_str() : "<nothing>";
	Throw(0, "%s: emitted output differs at line %llu. Expected '%s', got '%s'", testName, (unsigned long long)(firstDifference + 1), expectedLine, actualLine);
}

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
		EmitLog log;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
			printf("Field: %s\n", fieldName);
			log.Add("Field: ", fieldName);
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			log.Add("Hint: ", hint);
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

	ValidateEmitted(__ROCOCO_FUNCTION__, fieldEnumerator.log, R"EXPECT(
		Hint: Vec2
		Field: .x
		Field: .y
	)EXPECT");

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
		EmitLog log;

		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
			printf("Field: %s\n", fieldName);
			log.Add("Field: ", fieldName);
			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			log.Add("Hint: ", hint);

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

	ValidateEmitted(__ROCOCO_FUNCTION__, fieldEnumerator.log, R"EXPECT(
		Hint: Vec2
		Field: x
		Field: y
	)EXPECT");

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
		EmitLog log;

		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
			printf("Field: %s\n", fieldName);
			log.Add("Field: ", fieldName);
			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			log.Add("Hint: ", hint);
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

	ValidateEmitted(__ROCOCO_FUNCTION__, fieldEnumerator.log, R"EXPECT(
		Hint: Matrix4x4
		Hint: Vec4f
		Field: .x
		Field: .y
		Field: .z
		Field: .w
	)EXPECT");

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
		EmitLog log;

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
			log.Add("Field: ", fieldName);
			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			log.Add("Hint: ", hint);
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

	ValidateEmitted(__ROCOCO_FUNCTION__, fieldEnumerator.log, R"EXPECT(
		Field: Sys.Maths.F32
		Field: Sys.Maths.F64
		Field: Sys.Maths.FPSAngles
		Field: Sys.Maths.GetCentre
		Field: Sys.Maths.GetCentredRect
		Field: Sys.Maths.GetSpan
		Field: Sys.Maths.I32
		Field: Sys.Maths.I64
		Field: Sys.Maths.InflateRect
		Field: Sys.Maths.Linef
		Field: Sys.Maths.Linelf
		Field: Sys.Maths.Matrix4x4
		Field: Sys.Maths.Quadf
		Field: Sys.Maths.Quadlf
		Field: Sys.Maths.Quat
		Field: Sys.Maths.Quatlf
		Field: Sys.Maths.Rectf
		Field: Sys.Maths.Recti
		Field: Sys.Maths.Rectlf
		Field: Sys.Maths.SeedRandom
		Field: Sys.Maths.Triangle
		Field: Sys.Maths.Triangle2d
		Field: Sys.Maths.Trianglef
		Field: Sys.Maths.Trianglelf
		Field: Sys.Maths.Vec2
		Field: Sys.Maths.Vec2i
		Field: Sys.Maths.Vec2lf
		Field: Sys.Maths.Vec3
		Field: Sys.Maths.Vec3i
		Field: Sys.Maths.Vec3lf
		Field: Sys.Maths.Vec4
		Field: Sys.Maths.Vec4lf
	)EXPECT");

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
		EmitLog log;

		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
			printf("Method: %s\n", fieldName);
			log.Add("Field: ", fieldName);
			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			log.Add("Hint: ", hint);
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

	ValidateEmitted(__ROCOCO_FUNCTION__, fieldEnumerator.log, R"EXPECT(
		Hint: IStringBuilder
		Field: AppendAsDecimal
		Field: AppendAsHex
		Field: AppendAsNewLine
		Field: AppendAsSpecE
		Field: AppendAsSpecF
		Field: AppendAsSpecG
		Field: AppendBool
		Field: AppendChar
		Field: AppendFloat32
		Field: AppendFloat64
		Field: AppendIString
		Field: AppendIStringBuilder
		Field: AppendInt32
		Field: AppendInt64
		Field: AppendPointer
		Field: AppendSubstring
		Field: Clear
		Field: Replace
		Field: SetFormat
		Field: SetLength
		Field: StripLeft
		Field: ThrowIfAppendWouldTruncate
		Field: ToLower
		Field: ToUpper
		Field: Buffer
		Field: Length
	)EXPECT");

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
		EmitLog log;

		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
			printf("Method: %s\n", fieldName);
			log.Add("Field: ", fieldName);
			fieldCount++;
		}

		void OnHintFound(cr_substring hint) override
		{
			fputs("Hint: ", stdout);
			fwrite(hint.start, 1, hint.Length(), stdout);
			fputs("\n", stdout);
			log.Add("Hint: ", hint);
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

	ValidateEmitted(__ROCOCO_FUNCTION__, fieldEnumerator.log, R"EXPECT(
		Hint: Sys.Type.IStringBuilder
		Field: AppendAsDecimal
		Field: AppendAsHex
		Field: AppendAsNewLine
		Field: AppendAsSpecE
		Field: AppendAsSpecF
		Field: AppendAsSpecG
		Field: AppendBool
		Field: AppendChar
		Field: AppendFloat32
		Field: AppendFloat64
		Field: AppendIString
		Field: AppendIStringBuilder
		Field: AppendInt32
		Field: AppendInt64
		Field: AppendPointer
		Field: AppendSubstring
		Field: Clear
		Field: Replace
		Field: SetFormat
		Field: SetLength
		Field: StripLeft
		Field: ThrowIfAppendWouldTruncate
		Field: ToLower
		Field: ToUpper
		Field: Buffer
		Field: Length
	)EXPECT");

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
		EmitLog log;

		int fieldCount = 0;
		int hintCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
			printf("Method: %s\n", fieldName);
			log.Add("Field: ", fieldName);

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
			log.Add("Hint: ", hint);
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

	if (!Eq(type.declarationType, "Thing"))
	{
		char typeStr[128];
		type.declarationType.CopyWithTruncate(typeStr, sizeof typeStr);
		Throw(0, "Bad inference '%s' - type should be Thing", typeStr);
	}

	Rococo::Sex::Inference::EnumerateLocalFields(fieldEnumerator, sb, type.declarationType, sfile);

	if (fieldEnumerator.fieldCount != 3)
	{
		Throw(0, "Bad inference '%d' - expecting 3 fields of Thing", fieldEnumerator.fieldCount);
	}

	ValidateEmitted(__ROCOCO_FUNCTION__, fieldEnumerator.log, R"EXPECT(
		Field: maJig
		Field: maJigEx
		Field: maJigArgs
	)EXPECT");

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
		EmitLog log;

		int fieldCount = 0;
		int hintCount = 0;
		int typeCount = 0;

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			UNUSED(memberSearch);
			printf("Method: %s\n", fieldName);
			log.Add("Field: ", fieldName);

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
			log.Add("Hint: ", hint);
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

	if (!Eq(type.declarationType, "Thing"))
	{
		char typeStr[128];
		type.declarationType.CopyWithTruncate(typeStr, sizeof typeStr);
		Throw(0, "Bad inference '%s' - type should be Thing", typeStr);
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

		HMODULE hFactoryModule = LoadLibraryW(pathToDLL);
		if (!hFactoryModule)
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
				sexyIDE->SetDirectoryContainingConfigFile(R"(D:\work\rococo\content\)");
				sexyIDE->GetDatabase().PopulateViaSearchPaths();
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

	// Everything the code-complete system emitted through this editor, see EmitLog
	mutable EmitLog log;

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
		log.Add("CallTip: ", tip);
		printf("\tCallTip %s\n", tip);
	}

	void SetAutoCompleteCancelWhenCaretMoved() override
	{
		
	}

	void ShowAutoCompleteList(cstr spaceSeparatedItems) override
	{
		log.Add("List: ", spaceSeparatedItems);

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

		HString replacement;
		Format(replacement, "(%lld, %lld) %s", startPos, endPos, item);
		log.Add("Replace: ", replacement.c_str());
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
		log.Add("Hint: ", hintBuffer);
	}

	void AddItem(cstr item) override
	{
		printf("Item: %s\n", item);
		log.Add("Item: ", item);
	}

	void ShowAndClearItems() override
	{
		printf("<--- ShowAndClearItems --->\n");
		log.Add("Show", "");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: IString
		Item: cat.Buffer
		Item: cat.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		CallTip:  -> (Int32 length))
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: Matrix4x4
		Item: obj.m.r0
		Item: obj.m.r1
		Item: obj.m.r2
		Item: obj.m.r3
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: obj.name
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: IString
		Item: obj.name.Buffer
		Item: obj.name.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: obj.dog.bark
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: IString
		Item: obj.dog.bark.Buffer
		Item: obj.dog.bark.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		CallTip:  -> (Int32 length))
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		CallTip:  -> (Int32 length))
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: Sys.Maths.Matrix4x4
		Item: m.r0
		Item: m.r1
		Item: m.r2
		Item: m.r3
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		CallTip: (Sys.Type.NewException(Int32 errorCode)(IString message)))
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		CallTip: (Sys.Type.NewException(Int32 errorCode)(IString message)))
	)EXPECT");
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

	if (!Eq(editor.LastDefinitionFile(), ""))
	{
		Throw(0, "Expecting no definition, got '%s'", editor.LastDefinitionFile());
	}

	if (editor.LastDefintionLineNumber() != 0)
	{
		Throw(0, "Expecting line 0, got %d", editor.LastDefintionLineNumber());
	}
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
		Throw(0, "Expecting Sys.Type.sxy, got '%s'", editor.LastDefinitionFile());
	}

	if (editor.LastDefintionLineNumber() != 4)
	{
		Throw(0, "Expecting line 4, got %d", editor.LastDefintionLineNumber());
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
		Throw(0, "Expecting Sys.Type.sxy, got '%s'", editor.LastDefinitionFile());
	}

	if (editor.LastDefintionLineNumber() != 4)
	{
		Throw(0, "Expecting line 4, got %d", editor.LastDefintionLineNumber());
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
		Throw(0, "Expecting Sys.Type.sxy, got '%s'", editor.LastDefinitionFile());
	}

	if (editor.LastDefintionLineNumber() != 4)
	{
		Throw(0, "Expecting line 4, got %d", editor.LastDefintionLineNumber());
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
		Throw(0, "Expecting Sys.Maths.sxy, got '%s'", editor.LastDefinitionFile());
	}

	if (editor.LastDefintionLineNumber() != 13)
	{
		Throw(0, "Expecting line 13, got %d", editor.LastDefintionLineNumber());
	}
}

// Manual/visual test, not part of the automated run. It attaches the IDE as host window, so a successful goto opens the modal
// preview popup (PopupPreview) and waits until someone confirms it appeared and closes it. Call it by hand to check the preview

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
		Throw(0, "Expecting sys.maths.f64.inl, got '%s'", editor.LastDefinitionFile());
	}

	if (editor.LastDefintionLineNumber() != 483)
	{
		Throw(0, "Expecting line 483, got %d", editor.LastDefintionLineNumber());
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
		Throw(0, "Expecting sys.maths.f64.inl, got '%s'", editor.LastDefinitionFile());
	}

	if (editor.LastDefintionLineNumber() != 483)
	{
		Throw(0, "Expecting line 483, got %d", editor.LastDefintionLineNumber());
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
		Throw(0, "Expecting Sys.Maths.sxy, got '%s'", editor.LastDefinitionFile());
	}

	if (editor.LastDefintionLineNumber() != 13)
	{
		Throw(0, "Expecting line 13, got %d", editor.LastDefintionLineNumber());
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
		Throw(0, "Expecting Sys.Maths.sxy, got '%s'", editor.LastDefinitionFile());
	}

	if (editor.LastDefintionLineNumber() != 13)
	{
		Throw(0, "Expecting line 13, got %d", editor.LastDefintionLineNumber());
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: #EntryPoint
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: Sys.Type.IStringBuilder
		Item: methodName.AppendAsDecimal
		Item: methodName.AppendAsHex
		Item: methodName.AppendAsNewLine
		Item: methodName.AppendAsSpecE
		Item: methodName.AppendAsSpecF
		Item: methodName.AppendAsSpecG
		Item: methodName.AppendBool
		Item: methodName.AppendChar
		Item: methodName.AppendFloat32
		Item: methodName.AppendFloat64
		Item: methodName.AppendIString
		Item: methodName.AppendIStringBuilder
		Item: methodName.AppendInt32
		Item: methodName.AppendInt64
		Item: methodName.AppendPointer
		Item: methodName.AppendSubstring
		Item: methodName.Clear
		Item: methodName.Replace
		Item: methodName.SetFormat
		Item: methodName.SetLength
		Item: methodName.StripLeft
		Item: methodName.ThrowIfAppendWouldTruncate
		Item: methodName.ToLower
		Item: methodName.ToUpper
		Item: methodName.Buffer
		Item: methodName.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: Rococo.Graphics.IQuadStackTesselator
		Item: qst.AddCuboid
		Item: qst.AddCuboidAbs
		Item: qst.Clear
		Item: qst.ClearInput
		Item: qst.ClearOutput
		Item: qst.CopyInputToOutput
		Item: qst.Destruct
		Item: qst.Intrude
		Item: qst.MoveInputToOutput
		Item: qst.MoveInputToOutputWithNormalDotRange
		Item: qst.MoveOutputToInput
		Item: qst.MoveOutputToInputWithMat
		Item: qst.MoveOutputToInputWithNormalDotRange
		Item: qst.PopOutputAsTriangles
		Item: qst.PushQuad
		Item: qst.ScaleEdges
		Item: qst.SetBasis
		Item: qst.SetMaterial
		Item: qst.SetTextureRect
		Item: qst.Shrink
		Item: qst.SplitAcrossTangent
		Item: qst.SplitThreeColumns
		Item: qst.SplitThreeRows
		Item: qst.TileMosaic
		Item: qst.Translate
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: Rococo.Graphics.IQuadStackTesselator
		Item: qst.MoveInputToOutput
		Item: qst.MoveInputToOutputWithNormalDotRange
		Item: qst.MoveOutputToInput
		Item: qst.MoveOutputToInputWithMat
		Item: qst.MoveOutputToInputWithNormalDotRange
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: this.berries.Capacity
		Item: this.berries.Length
		Item: this.berries.Push
		Item: this.berries.Pop
		Item: this.berries.PopOut
		Item: this.berries.Clear
		Item: this.berries.Set
		Item: this.berries.Null
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: berries.Capacity
		Item: berries.Length
		Item: berries.Push
		Item: berries.Pop
		Item: berries.PopOut
		Item: berries.Clear
		Item: berries.Set
		Item: berries.Null
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: berries.Capacity
		Item: berries.Length
		Item: berries.Push
		Item: berries.Pop
		Item: berries.PopOut
		Item: berries.Clear
		Item: berries.Set
		Item: berries.Null
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: berries.Append
		Item: berries.Prepend
		Item: berries.Clear
		Item: berries.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: berries.Append
		Item: berries.Prepend
		Item: berries.Clear
		Item: berries.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: berries.Insert
		Item: berries.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: berries.Insert
		Item: berries.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: this.sexmlRoot
		Item: this.rootDirectives
		Item: this.nDirectives
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: this.berries.Insert
		Item: this.berries.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: this.berries.Insert
		Item: this.berries.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: this.berries.Append
		Item: this.berries.Prepend
		Item: this.berries.Clear
		Item: this.berries.Length
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: this.sexmlRoot
		Item: this.rootDirectives
		Item: this.nDirectives
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: Sys.Maths.Matrix4x4
		Item: m.r0
		Item: m.r1
		Item: m.r2
		Item: m.r3
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: Rococo.QuadVertices
		Hint: Sys.Maths.Quadf
		Hint: Vec3f
		Item: topQuad.positions.a.x
		Item: topQuad.positions.a.y
		Item: topQuad.positions.a.z
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: Rococo.QuadVertices
		Hint: Sys.Maths.Quadf
		Item: topQuad.positions.a
		Item: topQuad.positions.b
		Item: topQuad.positions.c
		Item: topQuad.positions.d
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: Rococo.Graphics.SoftBoxVertex
		Item: v.position
		Item: v.normal
		Item: v.uv
		Item: v.purpose
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: Rococo.Graphics.Camera
		Item: Rococo.Graphics.ESoftBoxVertexPurpose
		Item: Rococo.Graphics.FieldTesselator
		Item: Rococo.Graphics.FontMetrics
		Item: Rococo.Graphics.GetSoftBoxBuilder
		Item: Rococo.Graphics.HQFonts
		Item: Rococo.Graphics.ICamera
		Item: Rococo.Graphics.IFieldTesselator
		Item: Rococo.Graphics.IHQFonts
		Item: Rococo.Graphics.ILandscapeTesselator
		Item: Rococo.Graphics.IMaterialBuilder
		Item: Rococo.Graphics.IMeshBuilder
		Item: Rococo.Graphics.IMessaging
		Item: Rococo.Graphics.IQuadStackTesselator
		Item: Rococo.Graphics.IRendererConfig
		Item: Rococo.Graphics.IRimTesselator
		Item: Rococo.Graphics.IRodTesselator
		Item: Rococo.Graphics.ISceneBuilder
		Item: Rococo.Graphics.IShaderOptionsConfig
		Item: Rococo.Graphics.ISoftBoxBuilder
		Item: Rococo.Graphics.ISpriteBuilder
		Item: Rococo.Graphics.ITextTesselator
		Item: Rococo.Graphics.LandscapeTesselator
		Item: Rococo.Graphics.MaterialBuilder
		Item: Rococo.Graphics.MeshBuilder
		Item: Rococo.Graphics.Messaging
		Item: Rococo.Graphics.QuadStackTesselator
		Item: Rococo.Graphics.RendererConfig
		Item: Rococo.Graphics.RimTesselator
		Item: Rococo.Graphics.RodTesselator
		Item: Rococo.Graphics.RoundCornersShelfSpec
		Item: Rococo.Graphics.SceneBuilder
		Item: Rococo.Graphics.ShaderOptions
		Item: Rococo.Graphics.SoftBoxQuad
		Item: Rococo.Graphics.SoftBoxTopSpec
		Item: Rococo.Graphics.SoftBoxTriangle
		Item: Rococo.Graphics.SoftBoxVertex
		Item: Rococo.Graphics.SpriteBuilder
		Item: Rococo.Graphics.TextTesselator
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: #Sys
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: #Sys
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: #Sys.Geometry
		Item: #Sys.IO
		Item: #Sys.Maths
		Item: #Sys.Native
		Item: #Sys.PushStructArray
		Item: #Sys.Random
		Item: #Sys.Reflection
		Item: #Sys.SI
		Item: #Sys.Strings
		Item: #Sys.Time
		Item: #Sys.Type
		Item: #Sys.optional
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeCentreBottom
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeCentreTop
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeEastBottom
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeEastTop
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeNEBottom
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeNECorner
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeNWBottom
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeNWCorner
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeNone
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeNorthBottom
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeNorthTop
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeSEBottom
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeSECorner
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeSWBottom
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeSWCorner
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeSouthBottom
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeSouthTop
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeWestBottom
		Item: #Rococo.Graphics.ESoftBoxVertexPurposeWestTop
		Item: #Rococo.Graphics.HQFontDebuggerFont
		Item: #Rococo.Graphics.HQFontEditorFont
		Item: #Rococo.Graphics.HQFontEmperorFont
		Item: #Rococo.Graphics.HQFontInfoFont
		Item: #Rococo.Graphics.HQFontMenuFont
		Item: #Rococo.Graphics.HQFontTitleFont
		Item: #Rococo.Graphics.MaterialCategoryMarble
		Item: #Rococo.Graphics.MaterialCategoryMetal
		Item: #Rococo.Graphics.MaterialCategoryRock
		Item: #Rococo.Graphics.MaterialCategoryStone
		Item: #Rococo.Graphics.MaterialCategoryWood
		Item: #Rococo.Graphics.Native
		Item: #Rococo.Graphics.OrientationFlagsElevation
		Item: #Rococo.Graphics.OrientationFlagsHeading
		Item: #Rococo.Graphics.OrientationFlagsNone
		Item: #Rococo.Graphics.OrientationFlagsTilt
		Item: #Rococo.Graphics.SampleFilterBorder
		Item: #Rococo.Graphics.SampleFilterClamp
		Item: #Rococo.Graphics.SampleFilterMirror
		Item: #Rococo.Graphics.SampleFilterWrap
		Item: #Rococo.Graphics.SampleIndexEnvironmentalMap
		Item: #Rococo.Graphics.SampleIndexFonts
		Item: #Rococo.Graphics.SampleIndexHQFontGlyphs
		Item: #Rococo.Graphics.SampleIndexMaterials
		Item: #Rococo.Graphics.SampleIndexShadowMap
		Item: #Rococo.Graphics.SampleIndexSprites
		Item: #Rococo.Graphics.SampleIndexTextureSelector
		Item: #Rococo.Graphics.SampleMethodLinear
		Item: #Rococo.Graphics.SampleMethodPoint
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: #Sys.Type.FiveMacro
		Item: #Sys.Type.Formatters
		Item: #Sys.Type.FridayMacro
		Item: #Sys.Type.Macros
		Item: #Sys.Type.Strings
		Item: #Sys.Type.build
		Item: #Sys.Type.paragraph
		Item: #Sys.Type.path
		Item: #Sys.Type.string
		Item: #Sys.Type.token
		Show
	)EXPECT");
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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint:  Example (#build s 1 2 3 4) maps to (s 1)(s 2)(s 3)(s 4), used with such as stringbuilders and serializers
	)EXPECT");
}

void TestShowTipForFunction()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
	(function Main -> :
		(Sys.IO.GetCmdArgCount 
	)
)<CODE>";

	FileDesc desc(file, ' '); // Ensure the <CODE> section has space after the GetCmdArgCount token but not after the build token's parenthesis
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		CallTip:  -> (Int32 argCount))
	)EXPECT");
}

void TestShowCompletionsForStruct()
{
	Intro(__ROCOCO_FUNCTION__);

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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: dc.cppBuilder
		Item: dc.headerBuilder
		Item: dc.factoryPrefix
		Show
	)EXPECT");
}

void TestShowCompletionsForEnum()
{
	Intro(__ROCOCO_FUNCTION__);

	cstr file =
		R"<CODE>(
(function Meow -> :
	(Sys.Time.Day day)
	(day = .
)<CODE>";

	FileDesc desc(file, '.');
	TestEditor editor(desc.Text(), desc.CaretPos());

	sexyIDE->Gaffer().UpdateAutoComplete(editor);

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Item: .Sunday
		Item: .Monday
		Item: .Tuesday
		Item: .Wednesday
		Item: .Thursday
		Item: .Friday
		Item: .Saturday
		Show
	)EXPECT");
}


void TestShowCompletionsForeachReferenceVariable()
{
	Intro(__ROCOCO_FUNCTION__);

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

	ValidateEmitted(__ROCOCO_FUNCTION__, editor.log, R"EXPECT(
		Hint: IString
		Item: name.Buffer
		Item: name.Length
		Show
	)EXPECT");
}

void MainProtected2(HMODULE /* hLib */)
{
	pluginInit(HANDLE{ 0 });

	// TestFullEditor_GotoDefinitionOfFunction(); Manual/visual test: it opens the modal goto preview, see the comment on the function

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
	TestShowCompletionsForeachReferenceVariable();

	TestShowCompletionsForEnum();
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
		instance->SetDirectoryContainingConfigFile(R"(D:\work\rococo\content\)");
		instance->GetDatabase().PopulateViaSearchPaths();

		ISexyDatabase& database = instance->GetDatabase();

		RunTests(database);
	}
	else
	{
		Throw(nErr, "CreateSexyStudioFactory did not recognize interface %s", interfaceURL);
	}
}

void MainProtected3(HMODULE hLib)
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

		instance->SetDirectoryContainingConfigFile(R"(D:\work\rococo\content\)");
		instance->GetDatabase().PopulateViaSearchPaths();

		char hint[1024];
		instance->Gaffer().GetHintForCandidate(Substring::ToSubstring("UE"), hint);

		printf("Hint: %s\n", hint);
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
	if (!hLib)
	{
		U8FilePath msg;
		Format(msg, "Could not load library: %ls", pathToDLL.buf);
		MessageBoxA(NULL, msg, ErrorCaption, MB_ICONERROR);
		return GetLastError();
	}

	int errorCode = 0;

	try
	{
		MainProtected(hLib);
		MainProtected2(hLib);
		MainProtected3(hLib);
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

