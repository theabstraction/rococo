#include <rococo.allocators.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.os.h>
#include <rococo.strings.h>

#include <vector>

#include "unreal-sexy-gen.h"

using namespace Rococo;
using namespace Rococo::Sex;

void ParseClassDef(cr_sex sClassDef);
void ParseClassTree(cr_sex sRoot);

void ParseClassFile(crwstr filename, ISParser& parser)
{
	Auto<ISourceCode> src = parser.LoadSource(filename, { 1,1 });

	try
	{
		Auto<ISParserTree> tree = parser.CreateTree(*src);
		ParseClassTree(tree->Root());
	}
	catch (ParseException& ex)
	{
		printf("S-Parser Exception\n");
		printf("Source: %ls\n", filename);
		printf("Error line %d pos %d to line %d pos %d\n", ex.Start().y, ex.Start().x, ex.End().y, ex.End().x);
		throw;
	}
}

int mainProtected(int, char*[])
{
	AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(32768, 0, "main");
	Auto<ISParser> sParser = CreateSexParser_2_0(*allocator, SEXY_STANDARD_MAX_ATOMIC_STRING_LENGTH);

	crwstr filename = L"D:\\work\\Rococo.Reflect\\S-API\\all-classes.sexml";
	ParseClassFile(filename, *sParser);
	
	return 0;
}

int main(int argc, char* argv[])
{
	Rococo::OS::SetBreakPoints(Rococo::OS::Flags::BreakFlag_All);

	try
	{
		return mainProtected(argc, argv);
	}
	catch (IException& ex)
	{
		std::vector<char> err;
		err.resize(64_kilobytes);
		Rococo::OS::BuildExceptionString(err.data(), err.size(), ex, true);
		printf("%s", err.data());
		return ex.ErrorCode();
	}
}

void ParseClassTree(cr_sex sRoot)
{
	for (int i = 0; i < sRoot.NumberOfElements(); i++)
	{
		cr_sex sClassDef = sRoot[i];
		ParseClassDef(sClassDef);
	}
}

using namespace Rococo::Strings;
using namespace Rococo::Unreal;

void ValidateToken(cr_sex s, cstr matchThis, cstr context)
{
	if (!IsAtomic(s))
	{
		Throw(s, "%s: Expecting atomic token: %s", context, matchThis);
	}

	if (!Eq(s.c_str(), matchThis))
	{
		Throw(s, "%s: Expecting atomic token: %s. But saw %s", context, matchThis, s.c_str());
	}
}

struct UnrealFunctionArg : IUnrealArg
{
	cr_sex argName;
	cr_sex argType;
	bool isConst = false;
	bool isRef = false;

	UnrealFunctionArg(cr_sex _argName, cr_sex _argType): argName(_argName), argType(_argType)
	{
		cr_sex sParent = *_argName.Parent();
		for (int i = 0; i < sParent.NumberOfElements(); i++)
		{
			if (Eq(sParent[i].c_str(), "const"))
			{
				isConst = true;
			}
		}
	}

	void AppendName(StringBuilder& sb) const override
	{
		sb << argName.c_str();
	}

	void AppendTypeSansRef(StringBuilder& sb) const
	{
		cstr p = argType.c_str();
		while (*p != 0 && *p != '^')
		{
			sb.AppendChar(*p++);
		}
	}

	void AppendType(StringBuilder& sb) const override
	{
		cstr p = argType.c_str();

		if (Eq(p, "int32") || Eq(p, "int32^"))
		{
			sb << "int32";
		}
		else if (Eq(p, "int64") || Eq(p, "int64^"))
		{
			sb << "int64";
		}
		else if (Eq(p, "double") || Eq(p, "double64^"))
		{
			sb << "double";
		}
		else
		{
			sb << "UnknownType /*";
			AppendTypeSansRef(sb);
			sb << "*/";
		}
	}

	bool IsRef() const override
	{
		return EndsWith(argType.c_str(), "^");
	}

	bool IsConst() const override
	{
		return IsRef() && isConst;
	}
};

struct UnrealFunctionDef : IUnrealFunction
{
	cr_sex fDef;

	std::vector<UnrealFunctionArg*> args;

	UnrealFunctionDef(cr_sex f): fDef(f)
	{
		// Example: (' Method0 DivideFloats (visible read-only double A) (visible read-only double B) (out double NewParam))

		for (int i = 3; i < fDef.NumberOfElements(); i++)
		{
			cr_sex s = fDef[i];
			if (IsCompound(s) && s.NumberOfElements() > 2)
			{
				auto& argName = s[s.NumberOfElements() - 1];
				auto& argType = s[s.NumberOfElements() - 2];

				args.push_back(new UnrealFunctionArg(argName, argType));
			}
		}
	}

	IUnrealArg* GetArg(size_t index) override
	{
		if (index >= args.size()) return nullptr;
		return args[index];
	}

	~UnrealFunctionDef()
	{
		for (auto* arg : args)
		{
			delete arg;
		}
	}

	void AppendFunctionName(StringBuilder& sb) const override
	{
		sb << GetAtomicArg(fDef, 2).c_str();
	}
};

struct UnrealClassDef : IUnrealClass
{
	HString name;
	HString package;

	std::vector<UnrealFunctionDef*> functions;

	UnrealClassDef(cr_sex sDef)
	{
		if (sDef.NumberOfElements() < 3)
		{
			Throw(sDef, "Expecting at least 3 elements in a UClass def");
		}

		ValidateToken(sDef[0], "UClass", "ClassDef");

		cr_sex sPath = sDef[1];
		AssertCompound(sPath);
		ValidateToken(sPath[0], "[]", "ClassDef-Path");

		package = GetAtomicArg(sPath, 2).c_str();
		name = GetAtomicArg(sPath, 3).c_str();

		int colonIndicator = -1;

		for (int i = 2; i < sDef.NumberOfElements(); i++)
		{
			cr_sex sColon = sDef[i];
			if (IsAtomic(sColon) && Eq(sColon.c_str(), ":"))
			{
				colonIndicator = i;
				break;
			}
		}

		if (colonIndicator > 0)
		{
			for (int i = colonIndicator + 1; i < sDef.NumberOfElements(); i++)
			{
				cr_sex sDirective = sDef[i];
				if (IsCompound(sDirective))
				{
					if (Eq(GetAtomicArg(sDirective, 0).c_str(), "'"))
					{
						// Raw method definition. e.g (' Method0 AllowSelectionModifiers (const FScriptTypedElementHandle^ InElementHandle) (return bool ReturnValue))
						functions.push_back(new UnrealFunctionDef(sDirective));
					}
				}
			}
		}
	}

	~UnrealClassDef()
	{
		for (auto* f : functions)
		{
			delete f;
		}
	}

	size_t MethodCount() const override
	{
		return functions.size();
	}

	IUnrealFunction& GetFunction(size_t index) override
	{
		return *functions[index];
	}

	cstr ShortName() const override
	{
		return name;
	}
};

void GenClassDef(IUnrealClass& classDef, crwstr rootDirectory);

void ParseClassDef(cr_sex sDef)
{
	UnrealClassDef def(sDef);
	GenClassDef(def, L"D:\\work\\rococo\\source\\rococo\\sexy.UE5.API\\natives\\");
}