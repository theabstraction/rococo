#include <rococo.os.win32.h>
#include <rococo.libs.inl>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.sxytype-inference.h>

#include <rococo.sexystudio.api.h>

using namespace Rococo;
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

using namespace Rococo::SexyStudio;

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

	cstr lastV = Rococo::ReverseFind('v', sfile);

	Substring v = { lastV, lastV + 1 };

	printf("*** Start of %s ***\n", __FUNCTION__);

	printf("File: %s\n", file);

	struct ANON : ISexyFieldEnumerator
	{
		void OnField(cstr fieldName) override
		{
			printf("Field: %s\n", fieldName);
		}

		void OnHintFound(cstr hint) override
		{
			printf("Hint: %s\n", hint);
		}
	} fieldEnumerator;

	char type[256];
	bool isThis;
	if (!Rococo::Sexy::TryGetLocalTypeFromCurrentDocument(type, isThis, v, sfile))
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

	cstr lastV = Rococo::ReverseFind('v', sfile);

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

		void OnHintFound(cstr hint) override
		{
			printf("Hint: %s\n", hint);
			hintCount++;
		}
	} fieldEnumerator;

	char type[256];
	bool isThis;
	if (!Rococo::Sexy::TryGetLocalTypeFromCurrentDocument(type, isThis, v, sfile))
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

	cstr lastV = Rococo::ReverseFind('m', sfile);

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

		void OnHintFound(cstr hint) override
		{
			printf("Hint: %s\n", hint);
			hintCount++;
		}
	} fieldEnumerator;

	char type[256];
	bool isThis;
	if (!Rococo::Sexy::TryGetLocalTypeFromCurrentDocument(type, isThis, v, sfile))
	{
		Throw(0, "Bad inference - type should be Vec4");
	}

	database.EnumerateVariableAndFieldList(v, type, fieldEnumerator);

	if (fieldEnumerator.fieldCount != 4) Throw(0, "Bad fieldCount: %s", __FUNCTION__);
	if (fieldEnumerator.hintCount == 0) Throw(0, "Bad hint count: %s", __FUNCTION__);

	printf("*** End of %s ***\n", __FUNCTION__);
}

void RunTests(ISexyDatabase& database)
{
	printf("Running tests...\n");

//	TestDeduceVec2Fields(database);
//	TestDeduceVec2Fields2(database);
	TestDeduceMatrix4x4Fields(database);

	printf("\nTests completed\n");
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
		MainProtected(hLib);
	}
	catch (IException& ex)
	{
		Rococo::OS::ShowErrorBox(Rococo::Windows::NoParent(), ex, "SexyStudio Standalone App error");
		errorCode = ex.ErrorCode();
	}

	FreeLibrary(hLib);

	return errorCode;
}

