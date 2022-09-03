// rococo.sexy.cmd.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <rococo.api.h>
#include <rococo.io.h>
#include <rococo.ide.h>
#include <rococo.sexy.ide.h>
#include <stdio.h>

#include "sexy.types.h"
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"

#include <rococo.os.win32.h>
#include <rococo.window.h>

#include <limits>
#include <vector>

#include <rococo.libs.inl>

#include "sexy.lib.util.h"
#include "sexy.lib.script.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.script.h"
#include "sexy.s-parser.h"
#include "sexy.compiler.h"

#include <rococo.strings.h>
#include <rococo.os.h>

#include <cstring>

#ifdef _WIN32
# ifdef _DEBUG
#  pragma comment(lib, "sexy.compiler.Debug.lib")
#  pragma comment(lib, "sexy.vm.Debug.lib")
# else
#  pragma comment(lib, "sexy.compiler.Release.lib")
#  pragma comment(lib, "sexy.vm.Debug.lib")
# endif
#endif

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Script;
using namespace Rococo::Compiler;

#ifdef _DEBUG
#define IS_USING_DEBUG_LIBS true
#else
#define IS_USING_DEBUG_LIBS false
#endif

struct CLogger : public ILog
{
	CLogger()
	{
	}

	~CLogger()
	{
	}

	bool TryGetNextException(OUT ParseException& ex)
	{
		if (exceptions.empty()) return false;

		ex = exceptions.back();
		exceptions.pop_back();

		return true;
	}

	int ExceptionCount() const
	{
		return (int32)exceptions.size();
	}

	void Write(cstr text)
	{
		puts(text);
	}

	void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance)
	{
		ParseException ex(Vec2i{ 0,0 }, Vec2i{ 0,0 }, exceptionType, message, "", NULL);
		exceptions.push_back(ex);
	}

	void OnJITCompileException(Sex::ParseException& ex)
	{
		exceptions.push_back(ex);
	}

	void Clear()
	{
		exceptions.clear();
	}

	typedef std::vector<ParseException> TExceptions;
	TExceptions exceptions;
} s_logger;

void PrintExpression(cr_sex s, int& totalOutput, int maxOutput)
{
	switch (s.Type())
	{
	case EXPRESSION_TYPE_ATOMIC:
		totalOutput += printf(" %s", (cstr)s.String()->Buffer);
		break;
	case EXPRESSION_TYPE_NULL:
		totalOutput += printf("()");
		break;
	case EXPRESSION_TYPE_STRING_LITERAL:
		totalOutput += printf(" \"%s\"", (cstr)s.String()->Buffer);
		break;
	case EXPRESSION_TYPE_COMPOUND:

		totalOutput += printf("( ");

		for (int i = 0; i < s.NumberOfElements(); ++i)
		{
			if (totalOutput > maxOutput)
			{
				return;
			}

			cr_sex child = s.GetElement(i);
			PrintExpression(child, totalOutput, maxOutput);
		}

		totalOutput += printf(" )");
	}
}

void PrintParseException(const ParseException& e)
{
	printf("\r\nParse error:\r\nSource: %s\r\nExpression: (%d,%d) to (%d,%d)\r\nReason: %s\r\n", e.Name(), e.Start().x, e.Start().y, e.End().x, e.End().y, e.Message());

	int depth = 0;
	for (const ISExpression* s = e.Source(); s != NULL; s = s->GetOriginal())
	{
		if (depth++ > 0)  printf("Macro expansion %d:\r\n", depth);

		int totalOutput = 0;
		PrintExpression(*s, totalOutput, 1024);

		if (totalOutput > 1024) printf("...");

		printf("\r\n");
	}
}

int32 Run(IPublicScriptSystem& ss, cstr target);

int s_argument_Count = 0;

std::vector<HString> s_args;

int PrintError(IException& ex)
{
	if (ex.ErrorCode() != 0)
	{
		cstr message = ex.Message();

		char osMessage[256];
		Rococo::OS::FormatErrorMessage(osMessage, sizeof osMessage, ex.ErrorCode());

		fprintf(stderr, "\r\nError code %d (0x%X):%s\r\n", ex.ErrorCode(), ex.ErrorCode(), osMessage);

		for (const char* c = message; *c != 0; c++)
		{
			switch (*c)
			{
			case '\n':
				putc('\r', stderr);
				putc('\n', stderr);
				break;
			case '\r':
				break;
			default:
				putc(*c, stderr);
			}
		}

		fputs("\r\n", stderr);
	}
	else
	{
		printf("%s", ex.Message());
		return 0;
	}

	return ex.ErrorCode();
}

using namespace Rococo;
using namespace Rococo::Windows;

struct ScriptContext : public IEventCallback<ScriptCompileArgs>, public Rococo::Windows::IDE::IScriptExceptionHandler, public OS::IAppControl
{
	bool isInteractive;
	int nArgs;
	char** args;

	IInstallation& installation;

	void Free() override
	{

	}

	IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message) override
	{
		return isRunning && isInteractive ? IDE::EScriptExceptionFlow_Retry : IDE::EScriptExceptionFlow_Terminate;
	}

	void OnEvent(ScriptCompileArgs& ssArgs) override
	{
		ssArgs.ss.SetCommandLine(nArgs, args);
	}

	ScriptContext(IInstallation& _installation, int argc, char** argv) :installation(_installation)
	{
		this->nArgs = argc;
		this->args = argv;
	}

	int32 Execute(cstr pingPath, ScriptPerformanceStats& stats, int32 id, IScriptSystemFactory& ssFactory, IDebuggerWindow& debuggerWindow, ISourceCache& sourceCache)
	{
		try
		{
			int32 exitCode = IDE::ExecuteSexyScriptLoop(stats,
				4096_kilobytes,
				ssFactory,
				sourceCache,
				debuggerWindow,
				pingPath,
				id,
				(int32)128_kilobytes,
				*this,
				*this,
				*this,
				false,
				nullptr);

			return exitCode;
		}
		catch (...)
		{
			throw;
		}
	}

	bool isRunning = true;

	bool IsRunning() const override
	{
		return isRunning;
	}

	void ShutdownApp() override
	{
		isRunning = false;
	
	}
};

int mainProtected(int argc, char* argv[]);

int main(int argc, char* argv[])
{
	try
	{
		return mainProtected(argc, argv);
	}
	catch (IException& ex)
	{
		printf("Cmd line: %s\n", GetCommandLineA());
		PrintError(ex);
		return ex.ErrorCode() == 0 ? E_FAIL : ex.ErrorCode();
	}
	catch (...)
	{
		printf("Cmd line: %s\n", GetCommandLineA());
		printf("Unhandled exception of unknown type\n");
		return E_FAIL;
	}
}

// Search the command line for a name value pair x=y, with x as prefix, and y as the out value result. Search begins at startIndex, and the result index is returned.
int GetNextCmdArgValue(int argc, char* argv[], int startIndex, cstr prefix, cstr& result)
{
	result = nullptr;

	if (startIndex < 0 || startIndex >= argc)
	{
		return argc;
	}

	for (int i = startIndex; i < argc; i++)
	{
		cstr arg = argv[i];

		if (StartsWith(arg, prefix))
		{
			cstr target = arg + strlen(prefix);
			result = target;
			return i;
		}
	}

	return argc;
}

struct AppControl : public OS::IAppControlSupervisor
{
	void ShutdownApp() override
	{
		isRunning = false;
		exit(E_FAIL);
	}

	bool IsRunning() const
	{
		return isRunning;
	}

	void Free() override
	{
		
	}

	bool isRunning = true;
};

enum class ESwitch
{
	Interactive
};

struct SwitchBind
{
	char key;
	ESwitch value;
	cstr help;
};

SwitchBind switchMap[] =
{
	{'I', ESwitch::Interactive, "(I)nteractive Mode - use an interactive debugger if an error occurs or a (debug) directive is hit." }
};

bool HasSwitch(cstr switches, ESwitch switchValue)
{
	for (cstr p = switches; *p != 0; p++)
	{
		for (auto& i : switchMap)
		{
			if (i.key == *p && i.value == switchValue)
			{
				return true;
			}
		}
	}

	return false;
}

int mainProtected(int argc, char* argv[])
{
	if (argc == 1)
	{
		printf("Usage: <%s> natives=<NATIVE_SRC_PATH> switches=<switch-characters> installation=<CONTENT_FOLDER> run=<SEXY_SCRIPT_FILE>\n", argv[0]);
		printf("Known switches:\n");

		for (auto& s : switchMap)
		{
			printf("\t%c: %s\n", s.key, s.help);
		}

		return 0;
	}

	struct ConsoleWindow : public IWindow
	{
		operator HWND() const override
		{
			return GetConsoleWindow();
		}
	} console;

	cstr natives = nullptr;
	GetNextCmdArgValue(argc, argv, 0, "natives=", natives);
	if (natives)
	{
		WideFilePath wNativeSrcPath;
		Format(wNativeSrcPath, L"%hs", natives);
		Rococo::Script::SetDefaultNativeSourcePath(wNativeSrcPath);
	}

	bool isInteractive = false;

	cstr switches = nullptr;
	GetNextCmdArgValue(argc, argv, 0, "switches=", switches);
	if (switches)
	{
		if (HasSwitch(switches, ESwitch::Interactive))
		{
			isInteractive = true;
		}
	}

	cstr installationPath;
	GetNextCmdArgValue(argc, argv, 0, "installation=", installationPath);

	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);
	AutoFree<IOSSupervisor> os = GetOS();
	AutoFree<IInstallationSupervisor> installation;
	
	if (installationPath == nullptr)
	{
		installation = CreateInstallation(L"content.indicator.txt", *os);
	}
	else
	{
		WideFilePath wInstallation;
		Format(wInstallation, L"%hs", installationPath);
		installation = CreateInstallationDirect(wInstallation, *os);
	}

	ScriptContext sc(*installation, argc, argv);
	sc.isInteractive = isInteractive;

	if (isInteractive)
	{
		enum
		{
			GWL_HINSTANCE = -6
		};

		Rococo::Windows::InitRococoWindows(NULL, NULL, NULL, NULL, NULL);
	}

	AutoFree<Windows::IDE::IDebuggerEventHandler> debuggerEventHandler(Windows::IDE::CreateDebuggerEventHandler(*installation, console));
	AppControl appControl;

	AutoFree<IDebuggerWindow> debuggerWindow;
	if (isInteractive)
	{
		debuggerWindow = Windows::IDE::CreateDebuggerWindow(console, debuggerEventHandler->GetMenuCallback(), appControl);
	}
	else
	{
		debuggerWindow = Windows::IDE::GetConsoleAsDebuggerWindow();
	}

	AutoFree<IScriptSystemFactory> ssFactory(CreateScriptSystemFactory_1_5_0_0());
	AutoFree<ISourceCache> sourceCache(CreateSourceCache(*installation));

	WideFilePath nativeSourcePath = { 0 };

	try
	{
		int count = 0;
		for (int startIndex = 0; startIndex < argc;)
		{
			cstr result = nullptr;
			startIndex = GetNextCmdArgValue(argc, argv, startIndex, "run=", result) + 1;
			if (result)
			{
				count++;
				ScriptPerformanceStats stats;
				int32 exitCode = sc.Execute(result, stats, 0, *ssFactory, *debuggerWindow, *sourceCache);
				if (exitCode != 0)
				{
					printf("Script '%s' returned an error code.", result);
					return exitCode;
				}
			}

			if (startIndex == argc)
			{
				break;
			}
		}
		if (count == 0)
		{
			printf("Warning. No script run. Usage %s run=<script-file> <args...>\n", argv[0]);
			return E_FAIL;
		}
	}
	catch (STCException& e)
	{
		printf("Error: %s\r\nSource: %s\r\n.Code %d", e.Message(), e.Source(), e.Code());
		return e.Code();
	}
	catch (ParseException& e)
	{
		PrintParseException(e);
		return E_FAIL;
	}
	catch (IException& ose)
	{
		PrintError(ose);
		return ose.ErrorCode() == 0 ? E_FAIL : ose.ErrorCode();
	}
	catch (std::exception& stdex)
	{
		printf("std::exception: %s\r\n", stdex.what());
		return E_FAIL;
	}

    return 0;
}

int Run(IPublicScriptSystem& ss, cstr sourceCode, cstr targetFile)
{
	Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(sourceCode, -1, Vec2i{ 0,0 }, targetFile);
	Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	IModule* srcModule = ss.AddTree(*tree);

	auto& object = ss.PublicProgramObject();

	ss.Compile();

	const INamespace* nsEntryPoint = object.GetRootNamespace().FindSubspace("EntryPoint");
	if (nsEntryPoint == NULL)
	{
		Throw(0, "(namespace EntryPoint) not found");
	}

	const IFunction* f = nsEntryPoint->FindFunction("Main");
	if (f == NULL)
	{
		Throw(0, "No function defined: EntryPoint.Main ");
	}
	
	if (f->NumberOfInputs() != 0)
	{
		Throw(0, "function EntryPoint.Main should take no inputs");
	}

	if (f->NumberOfOutputs() != 1)
	{
		Throw(0, "function EntryPoint.Main should have 1 output -> (Int32 exitCode)");
	}

	auto& output = f->Arg(0);
	if (output.ResolvedType()->VarType() != VARTYPE_Int32)
	{
		Throw(0, "function EntryPoint.Main should have 1 output-> (Int32 exitCode)");
	}
	
	object.SetProgramAndEntryPoint(*f);
	
	auto& vm = object.VirtualMachine();

	// Expecting (function Main -> (Int32 id): ...)
	vm.Push(0); // Allocate stack space for the int32 result
	EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

	ParseException ex;
	while (s_logger.TryGetNextException(OUT ex))
	{
		PrintParseException(ex);
	}

	if (result != EXECUTERESULT_TERMINATED)
	{
		Throw(0, "Script did not terminate correctly.");
	}

	int32 exitCode = vm.PopInt32();
	return exitCode;
}

int Run(IPublicScriptSystem& ss, cstr target)
{
	struct : IEventCallback<cstr>
	{
		IPublicScriptSystem* ss;
		cstr target;
		int exitCode = 0;
		void OnEvent(cstr text) override
		{
			try
			{
				exitCode = Run(*ss, text, target);
			}
			catch (...)
			{
				printf("\nError with script file: %s\n", target);
				throw;
			}
		}
	} cb;

	cb.target = target;
	cb.ss = &ss;

	WideFilePath sysPath;
	Format(sysPath, L"%hs", target);

	Rococo::OS::LoadAsciiTextFile(cb, sysPath);

	return cb.exitCode;
}

