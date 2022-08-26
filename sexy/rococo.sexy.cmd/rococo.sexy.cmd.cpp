// rococo.sexy.cmd.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <rococo.api.h>
#include <stdio.h>

#include "sexy.types.h"
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"

#include <rococo.os.win32.h>

#include <limits>
#include <vector>

#include "sexy.lib.util.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.script.h"
#include "sexy.s-parser.h"
#include "sexy.compiler.h"

#include <rococo.strings.h>
#include <rococo.os.h>

#include <cstring>

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

int main(int argc, char* argv[])
{
	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);

	s_argument_Count = argc;

	s_args.reserve(argc);
	for (int i = 0; i < argc; i++)
	{
		s_args.push_back(argv[i]);
	}

	ProgramInitParameters pip;
	pip.addCoroutineLib = true;
	pip.addIO = true;
	pip.useDebugLibs = IS_USING_DEBUG_LIBS;
	pip.MaxProgramBytes = 8_megabytes;

#ifdef _DEBUG
	pip.useDebugLibs = true;
#endif

	WideFilePath nativeSourcePath = { 0 };

	try
	{
		auto prefix = "natives="_fstring;
		for (int i = 0; i < argc; i++)
		{
			cstr arg = argv[i];

			if (StartsWith(arg, prefix))
			{
				cstr target = arg + prefix.length;
				Format(nativeSourcePath, L"%hs", target);
				pip.NativeSourcePath = nativeSourcePath;
			}
		}

		CScriptSystemProxy ssp(pip, s_logger);

		auto& ss = ssp();
		ss.SetCommandLine(argc, argv);

		try
		{
			int count = 0;
			auto prefix = "run="_fstring;
			for (int i = 0; i < argc; i++)
			{
				cstr arg = argv[i];

				if (StartsWith(arg, prefix))
				{
					count++;
					cstr target = arg + prefix.length;
					int exitCode = Run(ss, target);
					size_t leakCount = ss.PublicProgramObject().FreeLeakedObjects();
					if (leakCount > 0)
					{
						Throw(0, "Warning %llu leaked objects", leakCount);
					}
					return exitCode;
				}
			}
			if (count == 0)
			{
				Throw(0, "Warning. No script run. Usage %s run=<script-file> <args...>", argv[0]);
			}
		}
		catch (STCException& e)
		{
			printf("Error: %s\r\nSource: %s\r\n.Code %d", e.Message(), e.Source(), e.Code());
			exit(e.Code());
		}
		catch (ParseException& e)
		{
			PrintParseException(e);
			exit(-1);
		}
		catch (IException& ose)
		{
			PrintError(ose);
			return ose.ErrorCode() == 0 ? E_FAIL : ose.ErrorCode();
		}
		catch (std::exception& stdex)
		{
			printf("std::exception: %s\r\n", stdex.what());
			exit(-1);
		}
	}
	catch (IException& ex)
	{
		if (std::strstr(ex.Message(), "SEXY_NATIVE_SRC_DIR") != NULL)
		{
			for (int i = 0; i < argc; ++i)
			{
				fprintf(stderr, "Arg #%d: %s\r\n", i, argv[i]);
			}

			if (*nativeSourcePath == 0)
			{
				fprintf(stderr, "No native source directory specified. Try adding 'natives=<SEXY_NATIVE_SRC_DIR>' to the command line\r\n");
				return 2; // File not found
			}
			else
			{
				fprintf(stderr, "Native source directory '%ls' was not identified: \r\n", nativeSourcePath.buf);
				return 2; // File not found
			}
		}
		PrintError(ex);
		return ex.ErrorCode() == 0 ? E_FAIL : ex.ErrorCode();
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

