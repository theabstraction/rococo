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

void Run(IPublicScriptSystem& ss, cstr target);

int s_argument_Count = 0;

std::vector<HString> s_args;

int main(int argc, char* argv[])
{
	s_argument_Count = argc;

	s_args.reserve(argc);
	for (int i = 0; i < argc; i++)
	{
		s_args.push_back(argv[i]);
	}

	ProgramInitParameters pip;
	pip.addCoroutineLib = true;
	pip.useDebugLibs = IS_USING_DEBUG_LIBS;
	pip.MaxProgramBytes = 8_megabytes;

#ifdef _DEBUG
	pip.useDebugLibs = true;
#endif

	CScriptSystemProxy ssp(pip, s_logger);

	auto& ss = ssp();

	try
	{
		auto prefix = "-run:"_fstring;
		for (int i = 0; i < argc; i++)
		{
			cstr arg = argv[i];

			if (StartsWith(arg, prefix))
			{
				cstr target = arg + prefix.length;
				Run(ss, target);
				size_t leakCount = ss.PublicProgramObject().FreeLeakedObjects();
				if (leakCount > 0)
				{
					Throw(0, "Warning %llu leaked objects", leakCount);
				}
			}
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
		if (ose.ErrorCode() != 0)
		{
			char osMessage[256];
			Rococo::OS::FormatErrorMessage(osMessage, sizeof osMessage, ose.ErrorCode());
			printf("Error code%d~0x%X,%s\r\n%s\r\n", ose.ErrorCode(), ose.ErrorCode(), ose.Message(), osMessage);
		}
		else
		{
			printf("%s", ose.Message());
			return -1;
		}

		return ose.ErrorCode();
	}
	catch (std::exception& stdex)
	{
		printf("std::exception: %s\r\n", stdex.what());
		exit(-1);
	}

    return 0;
}

void NativeSysCmdArgCount(NativeCallEnvironment& _nce)
{
	Rococo::uint8* _sf = _nce.cpu.SF();
	ptrdiff_t _offset = 2 * sizeof(size_t);
	
	int argCount = s_argument_Count;
	_offset += sizeof(argCount);
	WriteOutput(argCount, _sf, -_offset);
}

FastStringBuilder& ToFastStringBuilder(InterfacePointer ip)
{
	ObjectStub* stub = InterfaceToInstance(ip);
	if (!Eq(stub->Desc->TypeInfo->Name(), "FastStringBuilder"))
	{
		Throw(0, "Expecting argument to be of type FastStringBuilder");
	}

	return *reinterpret_cast<FastStringBuilder*>(stub);
}

fstring ToString(InterfacePointer ip)
{
	ObjectStub* stub = InterfaceToInstance(ip);
	auto* s = reinterpret_cast<InlineString*>(stub);
	return fstring{ s->buffer, s->length };
}

void NativeSysCmdAppendArg(NativeCallEnvironment& _nce)
{
	Rococo::uint8* _sf = _nce.cpu.SF();
	ptrdiff_t _offset = 2 * sizeof(size_t);

	int32 argIndex;
	_offset += sizeof(argIndex);
	ReadInput(argIndex, _sf, _offset);

	if (argIndex < 0 || argIndex >= s_argument_Count)
	{
		Throw(0, "Argument index out of bounds");
	}

	InterfacePointer ip;
	_offset += sizeof(ip);
	ReadInput(ip, _sf, _offset);

	auto& sb = ToFastStringBuilder(ip);
	if (sb.capacity > 0)
	{
		CopyString(sb.buffer, sb.capacity, s_args[argIndex]);
	}
}

void NativeSysCmdAppendError(NativeCallEnvironment& _nce)
{
	Rococo::uint8* _sf = _nce.cpu.SF();
	ptrdiff_t _offset = 2 * sizeof(size_t);

	int32 errNumber;
	_offset += sizeof(errNumber);
	ReadInput(errNumber, _sf, _offset);

	InterfacePointer ip;
	_offset += sizeof(ip);
	ReadInput(ip, _sf, _offset);

	auto& sb = ToFastStringBuilder(ip);
	if (sb.capacity > 0)
	{
		Rococo::OS::FormatErrorMessage(sb.buffer, sb.capacity, errNumber);
	}
}

void NativeSysCmdPrintError(NativeCallEnvironment& _nce)
{
	Rococo::uint8* _sf = _nce.cpu.SF();
	ptrdiff_t _offset = 2 * sizeof(size_t);

	int32 errNumber;
	_offset += sizeof(errNumber);
	ReadInput(errNumber, _sf, _offset);

	char err[128];
	Rococo::OS::FormatErrorMessage(err, sizeof err, errNumber);
	
	puts(err);
}

void NativeSysCmdOpenForRead(NativeCallEnvironment& _nce)
{
	Rococo::uint8* _sf = _nce.cpu.SF();
	ptrdiff_t _offset = 2 * sizeof(size_t);

	InterfacePointer ipFilename;
	_offset += sizeof(ipFilename);
	ReadInput(ipFilename, _sf, _offset);

	fstring filename = ToString(ipFilename);

	if (filename.length == 0 || filename.buffer == nullptr)
	{
		Throw(0, "Filename was blank");
	}

	HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Throw(GetLastError(), "%s", filename.buffer);
	}

	_offset += sizeof(hFile);
	WriteOutput(hFile, _sf, _offset);
}

void Run(IPublicScriptSystem& ss, cstr sourceCode, cstr targetFile)
{
	Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(sourceCode, -1, Vec2i{ 0,0 }, targetFile);
	Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	IModule* srcModule = ss.AddTree(*tree);

	auto& object = ss.PublicProgramObject();
	const INamespace& nsCMD = ss.AddNativeNamespace("Sys.Cmd");

	ss.AddNativeCall(nsCMD, NativeSysCmdArgCount, nullptr, "CmdArgCount -> (Int32 argCount)", __FILE__, __LINE__);
	ss.AddNativeCall(nsCMD, NativeSysCmdAppendArg, nullptr, "AppendCmdArg (Int32 index)(IStringBuilder sb)->", __FILE__, __LINE__);
	ss.AddNativeCall(nsCMD, NativeSysCmdAppendError, nullptr, "AppendError (Int32 osErrorNumber)(IStringBuilder sb)->", __FILE__, __LINE__);
	ss.AddNativeCall(nsCMD, NativeSysCmdPrintError, nullptr, "PrintError (Int32 osErrorNumber)->", __FILE__, __LINE__);
	ss.AddNativeCall(nsCMD, NativeSysCmdOpenForRead, nullptr, "OpenFileForRead (IString filename)->(Pointer hFile)", __FILE__, __LINE__);

	ss.Compile();

	const INamespace* nsEntryPoint = object.GetRootNamespace().FindSubspace("EntryPoint");

	const IFunction* f = nsEntryPoint->FindFunction("Main");
	if (f == NULL)
	{
		Throw(0, "No function defined: EntryPoint.Main ");
	}
	else
	{
		object.SetProgramAndEntryPoint(*f);
	}

	auto& vm = object.VirtualMachine();

	// Expecting int32 Main(int32 id)
	vm.Push(0); // Allocate stack space for the int32 result
	EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

	if (result != EXECUTERESULT_TERMINATED)
	{
		Throw(0, "Script did not terminate correctly.");
	}
	else
	{
		int32 result = vm.PopInt32();
	}
}

void Run(IPublicScriptSystem& ss, cstr target)
{
	struct : IEventCallback<cstr>
	{
		IPublicScriptSystem* ss;
		cstr target;
		void OnEvent(cstr text) override
		{
			try
			{
				Run(*ss, text, target);
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
}

