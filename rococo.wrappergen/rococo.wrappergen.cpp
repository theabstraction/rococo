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

#include <rococo.hashtable.h>

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Script;

struct Context
{
	stringmap<HString> mapSexyToCpp;
	HString apiNS;
};

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

bool IsConstant(const IArchetype& archetype, int argumentIndex)
{
    auto& argtype = archetype.GetArgument(argumentIndex);
    if (argtype.VarType() != VARTYPE_Derivative)
    {
        return false;
    }

    static auto fsConst = "const"_fstring;
    cstr argName = archetype.GetArgName(argumentIndex);
    return StartsWith(argName, fsConst) && StringLength(argName) > fsConst.length;
}

static cstr argPrefix = "arg_";

class MarshalBuilder: public IMarshalBuilder, public ICallback<const IFunction,cstr>
{
    int braceCount = 0;
    uint32 namehash;
    HString filename;
    Context& context;
private:
    void pad(int tabCount)
    {
		for (int i = 0; i < tabCount; ++i)
		{
			printf("    "); // If code standards require, you can change this to a tab or whatever
		}
    }

public:
    MarshalBuilder(cstr _filename, Context& _context): filename(_filename), context(_context)
    {
        namehash = (uint32) Hash(_filename);        
    }

    cstr ToCppType(cstr sexyType)
    {
        auto i = context.mapSexyToCpp.find(sexyType);
        if (i == context.mapSexyToCpp.end())
        {
            return sexyType;
        }
        else
        {
            return i->second.c_str();
        }
    }

    ~MarshalBuilder()
    {
    }

    void openbrace()
    {
        pad(braceCount++);
        printf("{\n");
    }

	void closebrace(cstr trailer = "")
	{
        if (braceCount == 0)
        {
            Throw(0, "Bad braceCount");
        }

		pad(--braceCount);
		printf("}%s\n", trailer);
	}

    void write(cstr format, ...)
    {
        pad(braceCount);

        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    }

	void writeraw(cstr format, ...)
	{
		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}

    void write_method_CPP(const IArchetype& method)
    {
		cstr returnType = method.NumberOfOutputs() == 0 ? "void" : method.GetArgument(0).Name();
		writeraw("%s %s(", ToCppType(returnType), method.Name());

		int nElements = method.NumberOfInputs() + method.NumberOfOutputs() - 1;

		// Input #0 is the interface pointer
		for (int j = method.NumberOfOutputs(); j < nElements; ++j)
		{
			if (j > method.NumberOfOutputs())
			{
				writeraw(", ");
			}

			bool isConstant = IsConstant(method, j);

			auto& argType = method.GetArgument(j);
			cstr argName = method.GetArgName(j);

			auto fsConst = "const"_fstring;

			cstr passByQualifier = argType.VarType() == VARTYPE_Derivative ? "&" : "";
			cstr constQualifier = isConstant ? "const " : "";
			cstr modifiedName = isConstant ? (argName + fsConst.length) : argName;

			writeraw("%s%s%s %s%s", constQualifier, ToCppType(argType.Name()), passByQualifier, argPrefix, modifiedName);
		}
    }

    void AddCreateObjectMarshalCode(const Rococo::Compiler::IFunction& function) override
    {
        if (function.NumberOfOutputs() != 1)
        {
            Throw(0, "%s: output count != 1", function.Name());
        }

        auto& arg = function.GetArgument(function.NumberOfInputs());

        if (arg.InterfaceCount() != 1)
        {
            Throw(0, "%s: Expecting output of interface type", arg.Name());
        }

        if (context.apiNS.length() == 0)
        {
            write("namespace Rococo::WrappedAPI");
        }
        else
        {
            write("namespace %s", context.apiNS.c_str());
        }

        openbrace();

        auto& interface = arg.GetInterface(0);
        cstr name = interface.Name();

        write("ROCOCOAPI %s", name);
        openbrace(); // open interface
        for (int i = 0; i < interface.MethodCount(); ++i)
        {
            auto& method = interface.GetMethod(i);
            pad(braceCount);
            writeraw("virtual ");
            write_method_CPP(method);
            writeraw(") = 0;");
            write("");
        }
        closebrace(";"); // close interface

        write("");

        write("ROCOCOAPI %sSupervisor: public %s", name, name);
        openbrace(); // open supervisor interface
        write("virtual void Free() = 0;");
        write("static cstr InterfaceName() { return R\"(%sSupervisor)\"; }", name);
        closebrace(";"); // close supervisor interface

        closebrace(); // close namespace

        write("");

        write("namespace ns%s%u // %s", name, namehash, filename.c_str());// namespace open
        openbrace();
        write("using namespace Rococo;");
        write("using namespace Rococo::Compiler;");
        write("using namespace Rococo::Script;");
        write("using namespace Rococo::VM;");
        write("using namespace %s;", context.apiNS.c_str());

        write("typedef InterfacePointer %sProxyPtr;", name);

        write("struct Concrete%s : public %sSupervisor", name, name);
        openbrace();

        write("AutoFree<IPublicScriptSystem> ss;");
        write("ISexyScriptClasses& ssc;");
        write("IVirtualMachine* vm = nullptr;");
        write("const IFunction* fnCreateObject = nullptr;");
        write("%sProxyPtr sxyNativePtr = nullptr;", name);
        write("");

        write("Concrete%s(ISexyScriptClasses& _ssc): ssc(_ssc)", name);
        openbrace(); // constructor open

        write("ss = ssc.CreateScript(R\"(%s)\");", filename.c_str());
        write("fnCreateObject = ssc.GetEntryFunction(*ss, \"CreateObject\");");
        write("validate(fnCreateObject, ssc);");
		write("vm = ssc.SetProgramAndEntryPoint(*ss, *fnCreateObject);");

        write("vm->Push(nullptr); // Allocate stack space for the interface pointer result");
        write("EXECUTERESULT result = ssc.ExecuteFunction(*vm);");
        write("sxyNativePtr = (%sProxyPtr) vm->PopInt64();", name, name);
        write("validate(sxyNativePtr, ssc);");

        closebrace(); // constructor close

        write("");

		write("virtual ~Concrete%s()", name);
		openbrace(); // destructor open
        write("ReleaseSexyInterface(sxyNativePtr, ssc);");
        closebrace(); // destructor close

        write("");

		write("void Free() override");
		openbrace(); // destructor open
        write("delete this;");
		closebrace(); // destructor close

		write("");

        write("static %sSupervisor* CreateObject(ISexyScriptClasses& ssc)", name);
        openbrace(); // method open
        write("return new Concrete%s(ssc);", name);
        closebrace(); // method close

        for (int i = 0; i < interface.MethodCount(); ++i)
        {
            write("");
            pad(braceCount);

            auto& method = interface.GetMethod(i);

            write_method_CPP(method);

            writeraw(")\n");
           
            openbrace(); // method open

            int nElements = method.NumberOfInputs() + method.NumberOfOutputs() - 1;

            for (int j = 0; j < method.NumberOfOutputs(); ++j)
            {
                auto& outputType = method.GetArgument(j);
                cstr defaultArg = outputType.SizeOfStruct() == 8 ? "(int64)0" : "(int32)0";
                write("vm->Push(%s); // %s", defaultArg, method.GetArgName(j));
            }

			for (int j = method.NumberOfOutputs(); j < nElements; ++j)
			{
				pad(braceCount);
				writeraw("vm->Push(");

				auto& argType = method.GetArgument(j);
				cstr argName = method.GetArgName(j);

				auto fsConst = "const"_fstring;

                bool isConstant = IsConstant(method, j);

				cstr modifiedName = isConstant ? (argName + fsConst.length) : argName;

                if (argType.VarType() == VARTYPE_Derivative)
                {
                    if (isConstant)
                    {
                        writeraw("(void*) const_cast<%s*>(&%s%s)", ToCppType(argType.Name()), argPrefix, modifiedName);
                    }
                    else
                    {
                        writeraw("(void*) &%s%s", argPrefix, modifiedName);
                    }
                }
                else
                {
                    writeraw("%s%s", argPrefix, modifiedName);
                }
                   
                writeraw(");");
                write("");
			}

            write("vm->Push(sxyNativePtr);");
			write("ExecuteMethod(*vm, ssc, sxyNativePtr, %d); // %s.%s", i, name, method.Name());

			for (int j = method.NumberOfOutputs(); j >= 0; --j)
			{
				auto& outputType = method.GetArgument(j);
                if (outputType.SizeOfStruct() == 4)
                {
                    cstr popType =  method.GetArgument(j).SizeOfStruct() == 4 ? "Int32" : "Int64";
                    cstr cppOutputType = ToCppType(outputType.Name());
                    write("%s %s%s = (%s) vm->Pop%s();", cppOutputType, argPrefix, method.GetArgName(j), cppOutputType, popType);
                }
			}

            if (method.NumberOfOutputs() > 0)
            {
                write("return %s%s;", argPrefix, method.GetArgName(0));
            }

            closebrace(); // method close
        }

		closebrace(";"); // class close

        write("");
        write("static Rococo::Script::RegisterWrapper<Concrete%s> register_%s%u(R\"(%s)\"); ", name, name, namehash, filename.c_str());
        write("");

		closebrace(); // namespace close
    }

    void AddFunctionsFromNamespace(const INamespace& ns)
    {
        ns.EnumerateFunctions(*this);
    }

    CALLBACK_CONTROL operator()(const IFunction& value, cstr context)
    {
        if (Eq(value.Name(), "CreateObject"))
        {
            return CALLBACK_CONTROL_CONTINUE;
        }

        return CALLBACK_CONTROL_CONTINUE;
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

void LoadSymbolMap(cstr symbolFile, stringmap<HString>& symbolMap)
{
	WideFilePath path;
	Format(path, L"%hs", symbolFile);

    AutoFree<IAllocatorSupervisor> allocator = Memory::CreateBlockAllocator(0, 0);
    Auto<ISParser> parser = Sexy_CreateSexParser_2_0(*allocator);

    Auto<ISourceCode> src;

    try
    {
        src = parser->LoadSource(path, Vec2i{ 1,1 });
    }
    catch (IException& ex)
    {
        LogGenerationException(ex);
        return;
    }
  
	Auto<ISParserTree> tree;

	try
	{
        tree = parser->CreateTree(*src);

        cr_sex root = tree->Root();
        for (int i = 0; i < root.NumberOfElements(); ++i)
        {
            cr_sex child = root[i];
            if (child.NumberOfElements() != 2)
            {
                Throw(child, "Expected 2 elements (<sxy-type> <c++-type>)");
            }

            cr_sex ssxyType = child[0];
            cr_sex scppType = child[1];

            if (ssxyType.Type() != Rococo::Sex::EXPRESSION_TYPE_ATOMIC)
            {
                Throw(ssxyType, "Expected atomic element for sexy type)");
            }

			if (scppType.Type() != Rococo::Sex::EXPRESSION_TYPE_ATOMIC && scppType.Type() != Rococo::Sex::EXPRESSION_TYPE_STRING_LITERAL)
			{
				Throw(scppType, "Expected atomic or string literal element for c++ type)");
			}

            cstr sexyType = ssxyType.String()->Buffer;
            cstr cppType = scppType.String()->Buffer;

            if (!symbolMap.insert(sexyType, cppType).second)
            {
                Throw(ssxyType, "Duplicate entry in symbol file %s for %s", symbolFile, sexyType);
            }
        }
	}
	catch (ParseException& pex)
	{
        LogGenerationException(pex);
		return;
	}
	catch (IException& ex)
	{
		LogGenerationException(ex);
		return;
	}

}

void PrintUsage(int argc, char* argv[])
{
	printf("Usage: %s <sexy-native-path> <options> <filepath.sxy> ... <filepathN.sxy>\n", argv[0]);
	printf("<options>: -symbols:<symbol-file-path>");
}

int main(int argc, char* argv[])
{
    AutoFree<IScriptSystemFactory> ssFactory = CreateScriptSystemFactory_1_5_0_0();

    if (argc < 3)
    {
        PrintUsage(argc, argv);
        return -1;
    }

    Context context;

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
            MarshalBuilder builder(argi, context);

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
            PrintUsage(argc, argv);
            return -1;
        }
        else
        {
			static auto fsSymbolPrefix = "-symbols:"_fstring;
			if (StartsWith(argi, fsSymbolPrefix))
			{
				cstr symbolFile = argi + fsSymbolPrefix.length;
				LoadSymbolMap(symbolFile, context.mapSexyToCpp);
			}

			static auto fsNSPrefix = "-ns:"_fstring;
			if (StartsWith(argi, fsNSPrefix))
			{
				cstr apiNS = argi + fsNSPrefix.length;
                context.apiNS = apiNS;
			}
        }
    }

    return 0;
}

