// rococo.wrappergen.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <rococo.api.h>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.debugging.h>

#include <sexy.lib.util.h>
#include <sexy.lib.script.h>

#include <sexy.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>
#include <sexy.S-Parser.h>

#include <stdio.h>

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Script;

struct Logger : Rococo::ILog
{
    void Write(cstr text) override
    {
        printf("Log: %s\n", text);
    }

	void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance) override
	{
        printf("Unhandled exception\n");
	}

	void OnJITCompileException(Sex::ParseException& ex) override
	{
        printf("JIT compile exception\n");
	}
};

Logger logger;

cstr GetFilenameFromPath(cstr path)
{
    cstr filename = path + StringLength(path) - 1;
	while (*filename != '\\')
	{
        if (filename <= path) return path;
        filename--;
	}
    return filename+1;
}

void LogStackFrames(IException& ex)
{
	using namespace Rococo::Debugging;

	struct ANON : IStackFrameFormatter
	{
		void Format(const StackFrame& sf) override
		{
            cstr moduleFilename = GetFilenameFromPath(sf.moduleName);
            cstr sourceFile = *sf.sourceFile == 0 ? "<os>" : GetFilenameFromPath(sf.sourceFile);
			printf("\t%-24.24s : %-24.24s  - %s line %d\n", moduleFilename, sf.functionName, sourceFile, sf.lineNumber);
		}
	} formatter;

	auto* frames = ex.StackFrames();
	if (frames)
	{
		frames->FormatEachStackFrame(formatter);
	}
}

void LogGenerationException(ParseException& pex)
{
    printf("ParseException\n");
    auto* srcExpression = pex.Source();
    if (srcExpression)
    {
        cstr name = srcExpression->Tree().Source().Name();
        printf(" Source File: %s\n", name);
    }

    printf(" Message: %s\n", pex.Message());

    printf("Line %d, Column %d to Line %d, Column %d\n", pex.Start().y, pex.Start().x, pex.End().y, pex.End().x);

    printf("Specimen:...................................\n\n%.128s\n............................................\n", pex.Specimen());

    LogStackFrames(pex);
}

void LogGenerationException(IException& ex)
{
    printf("IException\n");

    if (ex.ErrorCode() != 0)
    {
        printf("%s code %d\n", ex.Message(), ex.ErrorCode());
    }
    else
	{
        printf("%s\n", ex.Message());
	}

    LogStackFrames(ex);
}

ROCOCOAPI IMarshalBuilder
{
    virtual void AddCreateObjectMarshalCode(const Rococo::Compiler::IFunction& function) = 0;
};

class MarshalBuilder: public IMarshalBuilder
{
public:
    void AddCreateObjectMarshalCode(const Rococo::Compiler::IFunction& function) override
    {
        if (function.NumberOfOutputs() != 1)
        {
            Throw(0, "%s: output count != 1", function.Name());
        }

        auto& arg = function.GetArgument(function.NumberOfInputs());

        cstr argTypeName = arg.Name();

        if (arg.InterfaceCount() != 1)
        {
            Throw(0, "%s: Expecting output of interface type", argTypeName);
        }

        auto& interface = arg.GetInterface(0);
        cstr name = interface.Name();
        printf("%s* CreateObject();\n", name);
    }
};

void GenerateWrapper(IPublicScriptSystem& ss, cstr filename, IMarshalBuilder& builder)
{
    WideFilePath path;
    Format(path, L"%hs", filename);

    Auto<ISourceCode> src = ss.SParser().LoadSource(path, Vec2i{ 1,1 });
    Auto<ISParserTree> tree;

    try
    {
        tree = ss.SParser().CreateTree(*src);
    }
    catch (ParseException& pex)
    {
        LogGenerationException(pex);
    }
    catch (IException& ex)
    {
        LogGenerationException(ex);
    }

    ss.AddTree(*tree);

    try
    {
        ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		if (!ns)
		{
            Throw(0, "Could not find (namespace EntryPoint) inside script file");
		}

        cstr createFunctionName = "CreateObject";

        auto* createFunction = ns->FindFunction(createFunctionName);
        if (createFunction == nullptr)
        {
            Throw(0, "Could not find %s inside script file", createFunctionName);
        }

        builder.AddCreateObjectMarshalCode(*createFunction);
    }
	catch (ParseException& pex)
	{
		LogGenerationException(pex);
	}
	catch (IException& ex)
	{
		LogGenerationException(ex);
	}
}

int main(int argc, char* argv[])
{
    AutoFree<IAllocatorSupervisor> allocator = Memory::CreateBlockAllocator(0, 0);
    AutoFree<IScriptSystemFactory> ssFactory = CreateScriptSystemFactory_1_5_0_0(*allocator);

    if (argc < 3)
    {
        printf("Usage: wrappergen.exe <sexy-native-path> <filepath.sxy> ... <filepathN.sxy>\n");
        return -1;
    }

	Rococo::Compiler::ProgramInitParameters pip;
	pip.addCoroutineLib = true;

    WideFilePath nativeSourcePath;
    Format(nativeSourcePath, L"%hs", argv[1]);

    pip.NativeSourcePath = nativeSourcePath;

    for(int i = 2; i < argc; ++i)
    {
        cstr argi = argv[i];
        if (EndsWith(argi, ".sxy"))
        {
            MarshalBuilder builder;

            try
            {
				AutoFree<IPublicScriptSystem> ss = ssFactory->CreateScriptSystem(pip, logger);
				GenerateWrapper(*ss, argi, builder);
            }
            catch (IException& ex)
            {
                LogGenerationException(ex);
            }
        }
        else if (Eq(argi, "?") || Eq(argi, "-help"))
        {
            printf("Usage: wrappergen.exe <filepath.sxy> ... <filepathN.sxy>\n");
        }
    }

    return 0;
}

