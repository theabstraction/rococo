#include <rococo.allocators.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.os.h>
#include <rococo.strings.h>

#include <vector>

#include "unreal-sexy-gen.h"

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Strings;
using namespace Rococo::Unreal;

int g_nClassesParsed = 0;
int g_nMethodsParsed = 0;
int g_nMethodsNotMarshaled = 0;

void ParseClassDef(cr_sex sClassDef);
void ParseStructDef(cr_sex sDef);
void ParseClassTree(cr_sex sRoot);
void BuildCPPInputsAndOutputs(std::vector<Rococo::Unreal::IUnrealArg*>& inputs, std::vector<Rococo::Unreal::IUnrealArg*>& outputs, Rococo::Unreal::IUnrealFunction& method);
void BuildSexyInputsAndOutputs(std::vector<Rococo::Unreal::IUnrealArg*>& inputs, std::vector<Rococo::Unreal::IUnrealArg*>& outputs, Rococo::Unreal::IUnrealFunction& method);

bool DoExpressionsMatchRecursive(cr_sex a, cr_sex b, int startingIndex)
{
	if (a.NumberOfElements() != b.NumberOfElements())
	{
		return false;
	}

	if (a.Type() != b.Type())
	{
		return false;
	}

	switch (a.Type())
	{
	case Sex::EXPRESSION_TYPE_ATOMIC:
	case Sex::EXPRESSION_TYPE_STRING_LITERAL:
		return Strings::Eq(a.c_str(), b.c_str());
	case Sex::EXPRESSION_TYPE_NULL:
		return true;
	default:
		// Compound
		break;
	}

	for (int i = startingIndex; i < a.NumberOfElements(); i++)
	{
		if (!DoExpressionsMatchRecursive(a[i], b[i], 0))
		{
			return false;
		}
	}

	return true;
}

void ParseClassFile(crwstr filename, ISParser& parser)
{
	Auto<ISourceCode> src = parser.LoadSource(filename, { 1,1 });

	try
	{
		Auto<ISParserTree> tree = parser.CreateTree(*src);
		ParseClassTree(tree->Root());

		printf("\nNumber of classes: %d\n", g_nClassesParsed);
		printf("Number of methods in API: %d\n", g_nMethodsParsed);
		printf("Number of methods that could not be marshaled: %d\n", g_nMethodsNotMarshaled);

		double failureRate = 100.0 * (double)g_nMethodsNotMarshaled / (double)g_nMethodsParsed;
		printf("API coverage: %2.2f%%\n", 100.0 - failureRate);
	}
	catch (ParseException& ex)
	{
		printf("S-Parser Exception\n");
		printf("Source: %ls\n", filename);
		printf("Error line %d pos %d to line %d pos %d\n", ex.Start().y, ex.Start().x, ex.End().y, ex.End().x);
		throw;
	}
}

void PrintUnknownsAscending();

int mainProtected(int, char*[])
{
	AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(32768, 0, "main");
	Auto<ISParser> sParser = CreateSexParser_2_0(*allocator, SEXY_STANDARD_MAX_ATOMIC_STRING_LENGTH);

	crwstr filename = L"D:\\work\\Rococo.Reflect\\S-API\\all-classes.sexml";
	ParseClassFile(filename, *sParser);
	PrintUnknownsAscending();
	
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
	int structCount = 0;
	for (int i = 0; i < sRoot.NumberOfElements(); i++)
	{
		cr_sex sStructDef = sRoot[i];
		cr_sex sDirective = GetAtomicArg(sStructDef, 0);

		if (Eq(sDirective.c_str(), "UStruct"))
		{
			structCount++;
		}
	}

	printf("Processing %d UStructs", structCount);

	for (int i = 0; i < sRoot.NumberOfElements(); i++)
	{
		cr_sex sStructDef = sRoot[i];
		cr_sex sDirective = GetAtomicArg(sStructDef, 0);

		if (Eq(sDirective.c_str(), "UStruct"))
		{
			ParseStructDef(sStructDef);
		}

		if ((i % 100) == 0)
		{
			printf(".");
		}
	}

	int classCount = 0;
	for (int i = 0; i < sRoot.NumberOfElements(); i++)
	{
		cr_sex sClassDef = sRoot[i];
		cr_sex sDirective = GetAtomicArg(sClassDef, 0);
		if (Eq(sDirective.c_str(), "UClass"))
		{
			classCount++;
		}
	}

	printf("\nGenerating %d classes", classCount);

	for (int i = 0; i < sRoot.NumberOfElements(); i++)
	{
		cr_sex sClassDef = sRoot[i];
		cr_sex sDirective = GetAtomicArg(sClassDef, 0);
		if (Eq(sDirective.c_str(), "UClass"))
		{
			ParseClassDef(sClassDef);
		}

		if ((i % 100) == 0)
		{
			printf(".");
		}
	}
}

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

#include <ctype.h>

#include <rococo.hashtable.h>

ROCOCO_INTERFACE IMarshalType
{
	virtual cstr CPPName() const = 0;
};

Rococo::stringmap<IMarshalType*> marshalArgTypes;

struct MarshalType_Int32: IMarshalType
{
	cstr CPPName() const override
	{
		return "Int32";
	}
} s_mt_Int32;

struct MarshalType_Int64 : IMarshalType
{
	cstr CPPName() const override
	{
		return "Int64";
	}
} s_mt_Int64;

struct MarshalType_float : IMarshalType
{
	cstr CPPName() const override
	{
		return "float";
	}
} s_mt_float;

struct MarshalType_double : IMarshalType
{
	cstr CPPName() const override
	{
		return "double";
	}
} s_mt_double;

struct MarshalType_bool : IMarshalType
{
	cstr CPPName() const override
	{
		return "bool";
	}
} s_mt_bool;

struct MarshalType_FString : IMarshalType
{
	cstr CPPName() const override
	{
		return "Rococo::UE5::Marshal::FStringImage";
	}
} s_mt_FString;

struct MarshalType_FName : IMarshalType
{
	cstr CPPName() const override
	{
		return "Rococo::UE5::Marshal::FStringImage";
	}
} s_mt_FName;

struct MarshalType_FVector : IMarshalType
{
	cstr CPPName() const override
	{
		return "Rococo::UE5::Marshal::DVector3";
	}
} s_mt_FVector;

struct MarshalType_FTransform : IMarshalType
{
	cstr CPPName() const override
	{
		return "Rococo::UE5::Marshal::DTransform";
	}
} s_mt_FTransform;

struct MarshalType_FRotator : IMarshalType
{
	cstr CPPName() const override
	{
		return "Rococo::UE5::Marshal::DRotator";
	}
} s_mt_FRotator;

const IMarshalType* FindPrimitiveType(cstr argType)
{
	if (marshalArgTypes.size() == 0)
	{
		marshalArgTypes.insert("Int32", &s_mt_Int32);
		marshalArgTypes.insert("Int32^", &s_mt_Int32);
		marshalArgTypes.insert("Int64", &s_mt_Int64);
		marshalArgTypes.insert("Int64^", &s_mt_Int64);
		marshalArgTypes.insert("float", &s_mt_float);
		marshalArgTypes.insert("float^", &s_mt_float);
		marshalArgTypes.insert("double", &s_mt_double);
		marshalArgTypes.insert("double^", &s_mt_double);
		marshalArgTypes.insert("bool", &s_mt_bool);
		marshalArgTypes.insert("bool^", &s_mt_bool);
		marshalArgTypes.insert("FString", &s_mt_FString);
		marshalArgTypes.insert("FStringe^", &s_mt_FString);
		marshalArgTypes.insert("FName", &s_mt_FName);
		marshalArgTypes.insert("FName^", &s_mt_FName);
		marshalArgTypes.insert("FVector", &s_mt_FVector);
		marshalArgTypes.insert("FVector^", &s_mt_FVector);
		marshalArgTypes.insert("FTransform", &s_mt_FTransform);
		marshalArgTypes.insert("FTransform^", &s_mt_FTransform);
		marshalArgTypes.insert("FRotator", &s_mt_FRotator);
		marshalArgTypes.insert("FRotator^", &s_mt_FRotator);
	}

	auto i = marshalArgTypes.find(argType);
	return i != marshalArgTypes.end() ? i->second : nullptr;
}

stringmap<int> mapUnknownToUsage;

void MarkUnknown(cstr type)
{
	auto i = mapUnknownToUsage.find(type);
	if (i == mapUnknownToUsage.end())
	{
		mapUnknownToUsage.insert(type, 1);
	}
	else
	{
		i->second++;
	}
}

#include <algorithm>

void PrintUnknownsAscending()
{
	struct Usage
	{
		cstr item;
		int count;
	};
	std::vector<Usage> elements;
	for (auto& i : mapUnknownToUsage)
	{
		Usage u{ i.first, i.second };
		elements.push_back(u);
	}

	std::sort(elements.begin(), elements.end(), 
		[](const Usage& a, const Usage& b)
		{
			return a.count < b.count;
		}
	);

	printf("\n-------------------------------\n");
	printf("\nUnknown Type - [Occurence Rate]\n");

	for (auto& u : elements)
	{
		printf("%s - [%d]\n", u.item, u.count);
	}

	printf("\n-------------------------------\n");
}

void AppendIdentifier(StringBuilder& sb, cstr rawName)
{
	char firstChar = *rawName;

	if (isupper(firstChar))
	{
		if (isupper(rawName[1]))
		{
			// We have two or more capital letters, this could be an acronym, so we would not want to create a name like this: hAL, instead we add a prefix,  e.g lHAL
			cstr prefix = "l"; // for 'local'
			sb << prefix;
		}
		else
		{
			// Convert pascal case to camel case
			sb.AppendChar((char)tolower(firstChar));
			rawName++;
		}
	}

	sb << rawName;
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

	bool HasSexyCounterpart() const override
	{
		cstr p = argType.c_str();

		if (*p == 'U' && EndsWith(p, "*"))
		{
			return true;
		}

		if (*p == 'A' && EndsWith(p, "*"))
		{
			return true;
		}

		auto* primitive = FindPrimitiveType(p);
		if (primitive)
		{
			return true;
		}
				
		return false;
	}

	void AppendName(StringBuilder& sb, bool makeSexyVariableName = false) const override
	{
		if (!makeSexyVariableName)
		{
			sb << argName.c_str();
			return;
		}

		cstr rawName = argName.c_str();

		AppendIdentifier(sb, rawName);
	}

	void AppendTypeSansRef(StringBuilder& sb) const
	{
		cstr p = argType.c_str();
		while (*p != 0 && *p != '^')
		{
			sb.AppendChar(*p++);
		}
	}

	void MarshalNameTypeAsHandle(StringBuilder& sb, cstr nameType, bool makeSexyVariableType) const
	{
		if (makeSexyVariableType)
		{
			sb << "H";
		}

		sb << nameType;

		if (makeSexyVariableType)
		{
			sb.Undo(-1);
		}
	}

	void AppendType(StringBuilder& sb, bool makeSexyVariableType) const override
	{
		cstr p = argType.c_str();

		if (*p == 'U' && EndsWith(p, "*"))
		{
			MarshalNameTypeAsHandle(sb, p, makeSexyVariableType);
			return;
		}

		if (*p == 'A' && EndsWith(p, "*"))
		{
			MarshalNameTypeAsHandle(sb, p, makeSexyVariableType);
			return;
		}

		auto* primitive = FindPrimitiveType(p);
		if (primitive)
		{
			sb << primitive->CPPName();
			return;
		}

		MarkUnknown(p);
			
		sb << "UnknownType /*";
		AppendTypeSansRef(sb);
		sb << "*/";
	}

	bool GetObjectPointerType(char* buffer, size_t capacity) const override
	{
		cstr p = argType.c_str();
		if (*p == 'U' && EndsWith(p, "*"))
		{
			SecureFormat(buffer, capacity, "%s", p);
			return true;
		}

		if (*p == 'A' && EndsWith(p, "*"))
		{
			SecureFormat(buffer, capacity, "%s", p);
			return true;
		}

		return false;
	}

	bool IsPtr() const override
	{
		return EndsWith(argType.c_str(), "*");
	}

	bool IsRef() const override
	{
		return EndsWith(argType.c_str(), "^");
	}

	bool IsConst() const override
	{
		return IsRef() && isConst;
	}

	bool IsCPPOutput() const override
	{
		cr_sex argDef = *argName.Parent();
		for (int i = 0; i < argDef.NumberOfElements(); i++)
		{
			cr_sex s = argDef[i];
			if (IsAtomic(s))
			{
				if (Eq(s.c_str(), "out"))
				{
					return true;
				}

				if (Eq(s.c_str(), "return"))
				{
					return true;
				}
			}
		}

		return false;
	}

	bool IsSexyOutput() const override
	{
		if (!IsCPPOutput())
		{
			return false;
		}

		if (IsPtr())
		{
			// We want to emit a handle, but handles are derived types passed by mutable reference
			return false;
		}

		cstr p = argType.c_str();
		if (Eq(p, "FString") || Eq(p, "FString^"))
		{
			// FString is marshalled as FStringImage, a derived type, passed by mutable reference
			return false;
		}

		return true;
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

	bool HasSexyCounterpart() const override
	{
		for (auto* a : args)
		{
			if (!a->HasSexyCounterpart())
			{
				return false;
			}
		}

		return true;
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
		cstr fNameStr = GetAtomicArg(fDef, 2).c_str();
		sb << fNameStr;

		if (Eq(fNameStr, "Construct") || Eq(fNameStr, "Destruct"))
		{
			// Reserved method names in Sexy. We add a suffix unlikely to conflict with other method names
			sb << "QQQ";
		}
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
						// Check for duplicate functions. A duplicate may indicate two virtual functions each have their own UFUNCTION, but map to the same implementation
						// This will mean the algorithm speed is O(N^2), but since N = number of methods = small, nothing to worry about
						for (const auto* f : functions)
						{
							if (DoExpressionsMatchRecursive(f->fDef, sDirective, 2))
							{
								goto next;
							}
						}

						// Raw method definition. e.g (' Method0 AllowSelectionModifiers (const FScriptTypedElementHandle^ InElementHandle) (return bool ReturnValue))
						functions.push_back(new UnrealFunctionDef(sDirective));
					}
				}

			next:
				continue;
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

	cstr PackageName() const override
	{
		return package;
	}
};

void GenClassDef(IUnrealClass& classDef, crwstr nativeDirectory, crwstr sexyDirectory);

void ParseClassDef(cr_sex sDef)
{
	UnrealClassDef def(sDef);
	GenClassDef(def,
		L"D:\\work\\rococo\\source\\rococo\\sexy.UE5.API\\natives\\",
		L"D:\\work\\rococo\\source\\rococo\\sexy.UE5.API\\sexy-files\\"
	);

	g_nClassesParsed++;
	g_nMethodsParsed += (int) def.MethodCount();

	for (int i = 0; i < def.MethodCount(); i++)
	{
		auto& method = def.GetFunction(i);
		if (!method.HasSexyCounterpart())
		{
			g_nMethodsNotMarshaled++;
		}
	}
}

struct UnrealStructElement: IUnrealStructElement
{
	HString typeName;
	HString fieldName;
	HString innerValueType;
	int offset = 0;
	int sizeofStruct = 0;

	UnrealStructElement(cr_sex eDef)
	{
		// (Property ([] Def <sNameSpec>))
		cr_sex sNameSpec = eDef[1];
		ValidateToken(sNameSpec[0], "[]", __FUNCTION__);
		ValidateToken(sNameSpec[1], "Def", __FUNCTION__);
		typeName = GetAtomicArg(sNameSpec, 2).c_str();
		fieldName = GetAtomicArg(sNameSpec, sNameSpec.NumberOfElements() - 1).c_str();
		if (Eq(typeName, "TArray") || Eq(typeName, "TSet"))
		{
			innerValueType = GetAtomicArg(sNameSpec, 3).c_str();
		}

		if (eDef.NumberOfElements() > 2)
		{
			int i = 2;

			if (Eq(GetAtomicArg(eDef[2], 0).c_str(), "NameCPP"))
			{
				i++;
			}

			cr_sex sOffset = eDef[i];
			ValidateToken(sOffset[0], "Offset", __FUNCTION__);
			cstr txtOffset = GetAtomicArg(sOffset, 1).c_str();

			cr_sex sSizeOfStruct = eDef[i+1];
			ValidateToken(sSizeOfStruct[0], "SizeOf", __FUNCTION__);
			cstr txtSizeOfStruct = GetAtomicArg(sSizeOfStruct, 1).c_str();

			offset = atoi(txtOffset);
			sizeofStruct = atoi(txtSizeOfStruct);
		}
	}

	int Offset() const override
	{
		return offset;
	}

	int SizeOf() const override
	{
		return sizeofStruct;
	}

	cstr TypeName() const override
	{
		return typeName;
	}

	cstr FieldName() const override
	{
		return fieldName;
	}

	cstr InnerValueType() const override
	{
		return innerValueType;
	}
};

struct UnrealStructDef : IUnrealStruct
{
	HString typeName;
	HString pathName;
	int alignment = 0;
	int sizeofStruct = 0;

	std::vector<UnrealStructElement*> elements;

	cr_sex sDef;
	UnrealStructDef(cr_sex _sDef) : sDef(_sDef)
	{
		cr_sex sNameDirective = sDef[1];
		ValidateToken(sNameDirective[0], "FullName", __FUNCTION__);
		cstr path = GetAtomicArg(sNameDirective, 2).c_str();

		NamespaceSplitter splitter(path);

		cstr package, name;
		if (!splitter.SplitHead(OUT package, OUT name))
		{
			Throw(sDef, "Expecting /<package>.<name>");
		}

		cr_sex sOffsetDirective = sDef[2];
		ValidateToken(sOffsetDirective[0], "SizeOf", __FUNCTION__);
		cstr txtSize = sOffsetDirective[1].c_str();

		cr_sex sAlignDirective = sDef[3];
		ValidateToken(sAlignDirective[0], "Alignment", __FUNCTION__);
		cstr txtAlign = sAlignDirective[0].c_str();

		pathName = package;
		typeName = name;
		sizeofStruct = atoi(txtSize);
		alignment = atoi(txtAlign);

		if (sDef.NumberOfElements() > 5)
		{
			cr_sex sColon = sDef[4];
			ValidateToken(sColon, ":", __FUNCTION__);
		}

		for (int i = 5; i < sDef.NumberOfElements(); i++)
		{
			cr_sex sDirective = sDef[i];
			AssertCompound(sDirective);

			cstr command = GetAtomicArg(sDirective, 0).c_str();
			if (Eq(command, "Property"))
			{
				elements.push_back(new UnrealStructElement(sDirective));
			}
		}
	}

	~UnrealStructDef()
	{
		for (auto* e : elements)
		{
			delete e;
		}
	}

	cstr TypeName() const override
	{
		return typeName;
	}

	cstr Package() const override
	{
		return pathName;
	}

	int Alignment() const override
	{
		return alignment;
	}

	int SizeOf() const override
	{
		return sizeofStruct;
	}

	size_t ElementCount() const override
	{
		return elements.size();
	}

	IUnrealStructElement& operator[](size_t index) override
	{
		return *elements[index];
	}
};

void GenStructDef(IUnrealStruct& structDef, crwstr nativeDirectory);

void ParseStructDef(cr_sex sDef)
{
	UnrealStructDef def(sDef);
	GenStructDef(def, L"D:\\work\\rococo\\source\\rococo\\sexy.UE5.API\\natives\\");
}