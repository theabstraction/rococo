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

#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.script.h"
#include "sexy.s-parser.h"
#include "sexy.compiler.h"

#include <rococo.strings.h>
#include <rococo.os.h>

#include <cstring>

#include <rococo.task.queue.h>
#include <rococo.hashtable.h>

#include <rococo.time.h>

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Sex;
using namespace Rococo::Script;
using namespace Rococo::Compiler;

#ifdef _DEBUG
#define IS_USING_DEBUG_LIBS true
#else
#define IS_USING_DEBUG_LIBS false
#endif

struct CmdInstallation : Rococo::IO::IInstallationSupervisor
{
	AutoFree<IO::IOSSupervisor> os = IO::GetIOS();
	AutoFree<IO::IInstallationSupervisor> baseInstallation;
	
	CmdInstallation(cstr installationPath)
	{
		if (installationPath == nullptr)
		{
			baseInstallation = IO::CreateInstallation(L"content.indicator.txt", *os);
		}
		else
		{
			WideFilePath wInstallation;
			Format(wInstallation, L"%hs", installationPath);
			baseInstallation = IO::CreateInstallationDirect(wInstallation, *os);
		}
	}

	~CmdInstallation()
	{

	}

	void Free() override
	{
		delete this;
	}

	bool TryExpandMacro(cstr macroPrefixPlusPath, U8FilePath& expandedPath) override
	{
		return baseInstallation->TryExpandMacro(macroPrefixPlusPath, expandedPath);
	}

	const wchar_t* Content() const override
	{
		return baseInstallation->Content();
	}

	void LoadResource(cstr pingPath, IExpandingBuffer& buffer, int64 maxFileLength) override
	{
		baseInstallation->LoadResource(pingPath, buffer, maxFileLength);
	}

	void LoadResource(cstr resourcePath, ILoadEventsCallback& cb) override
	{
		baseInstallation->LoadResource(resourcePath, cb);
	}

	bool TryLoadResource(cstr pingPath, IExpandingBuffer& buffer, int64 maxFileLength) override
	{
		return baseInstallation->TryLoadResource(pingPath, buffer, maxFileLength);
	}

	void ConvertPingPathToSysPath(cstr pingPath, WideFilePath& path) const override
	{
		baseInstallation->ConvertPingPathToSysPath(pingPath, path);
	}

	void ConvertPingPathToSysPath(cstr pingPath, U8FilePath& path) const override
	{
		baseInstallation->ConvertPingPathToSysPath(pingPath, path);
	}

	void ConvertSysPathToMacroPath(const wchar_t* sysPath, U8FilePath& pingPath, cstr macro) const override
	{
		baseInstallation->ConvertSysPathToMacroPath(sysPath, pingPath, macro);
	}

	void ConvertSysPathToPingPath(const wchar_t* sysPath, U8FilePath& pingPath) const override
	{
		baseInstallation->ConvertSysPathToPingPath(sysPath, pingPath);
	}

	bool DoPingsMatch(cstr a, cstr b) const override
	{
		return baseInstallation->DoPingsMatch(a, b);
	}

	void Macro(cstr name, cstr pingFolder) override
	{
		baseInstallation->Macro(name, pingFolder);
	}

	void CompressPingPath(cstr pingPath, U8FilePath& resultPath) const override
	{
		baseInstallation->CompressPingPath(pingPath, resultPath);
	}

	IOS& OS() override
	{
		return *os;
	}
};

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
		UNUSED(exceptionInstance);
		UNUSED(errorCode);
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

void PrintErrorExpression(cr_sex s, int& totalOutput, int maxOutput)
{
	switch (s.Type())
	{
	case EXPRESSION_TYPE_ATOMIC:
		totalOutput += fprintf(stderr, " %s", (cstr)s.c_str());
		break;
	case EXPRESSION_TYPE_NULL:
		totalOutput += fprintf(stderr, "()");
		break;
	case EXPRESSION_TYPE_STRING_LITERAL:
		totalOutput += fprintf(stderr, " \"%s\"", (cstr)s.c_str());
		break;
	case EXPRESSION_TYPE_COMPOUND:

		totalOutput += fprintf(stderr, "( ");

		for (int i = 0; i < s.NumberOfElements(); ++i)
		{
			if (totalOutput > maxOutput)
			{
				return;
			}

			cr_sex child = s.GetElement(i);
			PrintErrorExpression(child, totalOutput, maxOutput);
		}

		totalOutput += fprintf(stderr, " )");
	}
}

void PrintParseException(const ParseException& e)
{
	fprintf(stderr, "\r\nParse error:\r\nSource: %s\r\nExpression: (%d,%d) to (%d,%d)\r\nReason: %s\r\n", e.Name(), e.Start().x, e.Start().y, e.End().x, e.End().y, e.Message());

	int depth = 0;
	for (const ISExpression* s = e.Source(); s != NULL; s = s->GetOriginal())
	{
		if (depth++ > 0)  printf("Macro expansion %d:\r\n", depth);

		int totalOutput = 0;
		PrintErrorExpression(*s, totalOutput, 1024);

		if (totalOutput > 1024) fprintf(stderr, "...");

		fprintf(stderr, "\r\n");
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
		fprintf(stderr, "%s", ex.Message());
		return 0;
	}

	return ex.ErrorCode();
}

using namespace Rococo;
using namespace Rococo::Windows;

struct ScriptContext : public IScriptCompilationEventHandler, public Rococo::Windows::IDE::IScriptExceptionHandler, public OS::IAppControl, public Tasks::ITaskQueue, public ISecuritySystem
{
	bool isInteractive;
	int nArgs;
	char** args;

	IO::IInstallation& installation;

	void Free() override
	{

	}

	IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message) override
	{
		UNUSED(source);
		UNUSED(message);
		return isRunning && isInteractive ? IDE::EScriptExceptionFlow::Retry : IDE::EScriptExceptionFlow::Terminate;
	}

	void OnCompile(ScriptCompileArgs& ssArgs) override
	{
		Rococo::Script::AddNativeCallSecurity_ToSysNatives(ssArgs.ss);
		ssArgs.ss.SetCommandLine(nArgs, args);
		ssArgs.ss.SetSecurityHandler(*this);
	}

	ScriptContext(IO::IInstallation& _installation, int argc, char** argv) :installation(_installation)
	{
		this->nArgs = argc;
		this->args = argv;
	}

	Tasks::ITaskQueue& MainThreadQueue() override
	{
		return *this;
	}

	void ValidateSafeToWrite(IPublicScriptSystem& ss, cstr pathname) override
	{
		UNUSED(ss);
		UNUSED(pathname);
		// The cmd script is no better or worse than any other command line utility for security, so if the user wants to write to any file, we have no objections here
		// result = dandy
	}

	void AddTask(Rococo::Function<void()> lambda) override
	{
		Throw(0, "%s: Not implemented", __FUNCTION__);
	}

	bool ExecuteNext() override
	{
		Throw(0, "%s: Not implemented", __FUNCTION__);
	}

	int32 Execute(cstr pingPath, ScriptPerformanceStats& stats, int32 id, IScriptSystemFactory& ssFactory, IDebuggerWindow& debuggerWindow, ISourceCache& sourceCache, IScriptEnumerator& implicitIncludes)
	{
		try
		{
			int32 exitCode = IDE::ExecuteSexyScriptLoop(stats,
				4096_kilobytes,
				ssFactory,
				sourceCache,
				implicitIncludes,
				debuggerWindow,
				pingPath,
				id,
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

#include <rococo.allocators.h>

int main(int argc, char* argv[])
{
	using namespace Rococo::Memory;
	AllocatorLogFlags flags;
	flags.LogDetailedMetrics = false;
	flags.LogLeaks = false;
	flags.LogOnModuleExit = false;
	SetAllocatorLogFlags(flags);

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

struct AppControl : public OS::IAppControlSupervisor, public Tasks::ITaskQueue
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

	Tasks::ITaskQueue& MainThreadQueue() override
	{
		return *this;
	}

	void AddTask(Rococo::Function<void()> lambda) override
	{
		Throw(0, "%s: Not implemented", __FUNCTION__);
	}

	bool ExecuteNext() override
	{
		Throw(0, "%s: Not implemented", __FUNCTION__);
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
	{'I', ESwitch::Interactive, "(I)nteractive Mode - use an interactive debugger if an error occurs or a (debug) directive is hit." },	
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

struct CmdIncludes : IScriptEnumerator
{
	size_t Count() const override
	{
		return 0;
	}

	cstr ResourceName(size_t index) const
	{
		UNUSED(index);
		Throw(0, "No resource defined for cmd");
	}
} s_CmdIncludes;

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

	Rococo::OS::SetBreakPoints(Rococo::OS::Flags::BreakFlag_All);
	AutoFree<IO::IInstallationSupervisor> installation = new CmdInstallation(installationPath);
	
	ScriptContext sc(*installation, argc, argv);
	sc.isInteractive = isInteractive;

	if (isInteractive)
	{
		Rococo::Windows::InitRococoWindows(NULL, NULL, NULL, NULL, NULL);
	}

	AppControl appControl;

	AutoFree<IDebuggerWindow> debuggerWindow;
	if (isInteractive)
	{
		debuggerWindow = Windows::IDE::CreateDebuggerWindow(console, appControl, *installation);
	}
	else
	{
		debuggerWindow = Windows::IDE::GetConsoleAsDebuggerWindow(Windows::IDE::GetStdoutFormatter(), Windows::IDE::GetConsoleColourController());
	}

	AutoFree<IScriptSystemFactory> ssFactory(CreateScriptSystemFactory_1_5_0_0());

	AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(16384, 0, "sexy-cmd");
	AutoFree<ISourceCache> sourceCache(CreateSourceCache(*installation, *allocator));

	WideFilePath nativeSourcePath;

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
				int32 exitCode = sc.Execute(result, stats, 0, *ssFactory, *debuggerWindow, *sourceCache, s_CmdIncludes);
				if (exitCode != 0)
				{
					fprintf(stderr, "Script '%s' returned an error code. %d (0x%8.8x)", result, exitCode, exitCode);
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
			fprintf(stderr, "Warning.No script run.Usage % s run = <script - file> <args...>\n", argv[0]);
			return E_FAIL;
		}
	}
	catch (STCException& e)
	{
		fprintf(stderr, "Error: % s\r\nSource: % s\r\n.Code % d", e.Message(), e.Source(), e.Code());
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
		fprintf(stderr, "std::exception: %s\r\n", stdex.what());
		return E_FAIL;
	}

    return 0;
}

int Run(IPublicScriptSystem& ss, cstr sourceCode, cstr targetFile)
{
	Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(sourceCode, -1, Vec2i{ 0,0 }, targetFile);
	Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	IModule* srcModule = ss.AddTree(*tree);
	UNUSED(srcModule);

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

