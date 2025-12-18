#include <rococo.allocators.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.os.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>
#include <rococo.io.h>

#include <vector>

#include "unreal-sexy-gen.h"

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Strings;
using namespace Rococo::Unreal;

auto subclassOfPrefix = "TSubclassOf<"_fstring;
auto scriptInterfacePrefix = "TScriptInterface<"_fstring;
auto objectPtrPrefix = "TObjectPtr<"_fstring;

int g_nClassesParsed = 0;
int g_nMethodsParsed = 0;
int g_nMethodsNotMarshaled = 0;
Rococo::stringmap<int> knownDelegatesVsSize;

void ParseClassDef(cr_sex sClassDef, IClassSystem& classSystem);
void ParseEnumDef(cr_sex sEnumDef);
void ParseStructDef(cr_sex sDef);
void ParseClassTree(cr_sex sRoot);
void BuildCPPInputsAndOutputs(std::vector<Rococo::Unreal::IUnrealArg*>& inputs, std::vector<Rococo::Unreal::IUnrealArg*>& outputs, Rococo::Unreal::IUnrealFunction& method);
void BuildSexyInputsAndOutputs(std::vector<Rococo::Unreal::IUnrealArg*>& inputs, std::vector<Rococo::Unreal::IUnrealArg*>& outputs, Rococo::Unreal::IUnrealFunction& method);
crwstr GetOutputDirectory();

int FindDelegateSize(cstr name)
{
	auto i = knownDelegatesVsSize.find(name);
	return i == knownDelegatesVsSize.end() ? 0 : i->second;
}

void AddDelegate(cstr elementType, int delegateSize)
{
	auto j = knownDelegatesVsSize.find(elementType);
	if (j == knownDelegatesVsSize.end())
	{
		if (EndsWith(elementType, "^"))
		{
			char valueWithNoRef[256];
			CopyString(valueWithNoRef, sizeof valueWithNoRef, elementType);
			valueWithNoRef[strlen(valueWithNoRef) - 1] = 0;
			knownDelegatesVsSize.insert(valueWithNoRef, delegateSize);

		}
		else
		{
			knownDelegatesVsSize.insert(elementType, delegateSize);
		}
	}
	else
	{
		if (j->second != delegateSize)
		{
			Throw(0, "Expecting delegate size %d to match that of previous definition (%d)", delegateSize, j->second);
		}
	}
}

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

namespace Rococo::IO
{
	void PrintOverwriteReport();
}

std::vector<struct UnrealClassDef*> allClasses;

void ParseDelegateDef(cstr typeName, int sizeInBytes);

void ParseClassTree(cr_sex sRoot)
{
	int enumCount = 0;
	for (int i = 0; i < sRoot.NumberOfElements(); i++)
	{
		cr_sex sEnumDef = sRoot[i];
		cr_sex sEnumDirective = GetAtomicArg(sEnumDef, 0);

		if (Eq(sEnumDirective.c_str(), "UEnum"))
		{
			enumCount++;
		}
	}

	printf("Processing %d UEnums", enumCount);

	enumCount = 0;
	for (int i = 0; i < sRoot.NumberOfElements(); i++)
	{
		cr_sex sEnumDef = sRoot[i];
		cr_sex sEnumDirective = GetAtomicArg(sEnumDef, 0);

		if (Eq(sEnumDirective.c_str(), "UEnum"))
		{
			ParseEnumDef(sEnumDef);
			if ((enumCount++ % 100) == 0)
			{
				printf(".");
			}
		}
	}

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

	printf("\nProcessing %d UStructs", structCount);

	structCount = 0;
	for (int i = 0; i < sRoot.NumberOfElements(); i++)
	{
		cr_sex sStructDef = sRoot[i];
		cr_sex sDirective = GetAtomicArg(sStructDef, 0);

		if (Eq(sDirective.c_str(), "UStruct"))
		{
			ParseStructDef(sStructDef);
			if ((structCount++ % 100) == 0)
			{
				printf(".");
			}
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

	printf("\nProcessing %d UClasses", classCount);

	AutoFree<IClassSystem> classSystem = CreateClassSystem(GetOutputDirectory());

	classCount = 0;
	for (int i = 0; i < sRoot.NumberOfElements(); i++)
	{
		cr_sex sClassDef = sRoot[i];
		cr_sex sDirective = GetAtomicArg(sClassDef, 0);
		if (Eq(sDirective.c_str(), "UClass"))
		{
			ParseClassDef(sClassDef, *classSystem);
			if ((classCount++ % 100) == 0)
			{
				printf(".");
			}
		}
	}

	classSystem->Commit();

	printf("\nProcessing %llu delegates", knownDelegatesVsSize.size());
	for (auto& d : knownDelegatesVsSize)
	{
		cstr typeName = d.first;
		int sizeInBytes = d.second;

		ParseDelegateDef(typeName, sizeInBytes);
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

Rococo::stringmap<IMarshalType*> marshalArgTypes;

struct MarshalType_Int32: IMarshalType
{
	cstr CPPName() const override
	{
		return "int32";
	}
} s_mt_Int32;

struct MarshalType_Int64 : IMarshalType
{
	cstr CPPName() const override
	{
		return "int64";
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
		return "R_FString";
	}
} s_mt_FString;

struct MarshalType_FName : IMarshalType
{
	cstr CPPName() const override
	{
		return "R_FName";
	}
} s_mt_FName;

struct MarshalType_FText : IMarshalType
{
	cstr CPPName() const override
	{
		return "R_FText";
	}
} s_mt_FText;

const IMarshalType* FindPrimitiveType(cstr argType)
{
	if (marshalArgTypes.size() == 0)
	{
		marshalArgTypes.insert("uint8", &s_mt_Int32);
		marshalArgTypes.insert("int32", &s_mt_Int32);
		marshalArgTypes.insert("int64", &s_mt_Int32);
		marshalArgTypes.insert("float", &s_mt_float);
		marshalArgTypes.insert("double", &s_mt_double);
		marshalArgTypes.insert("bool", &s_mt_bool);
		marshalArgTypes.insert("FString", &s_mt_FString);
		marshalArgTypes.insert("FName", &s_mt_FName);
		marshalArgTypes.insert("FText", &s_mt_FText);
	}

	auto i = marshalArgTypes.find(argType);
	return i != marshalArgTypes.end() ? i->second : nullptr;
}

Rococo::stringmap<int> g_mapUnknownToUsage;

void MarkUnknown(cstr type)
{
	auto i = g_mapUnknownToUsage.find(type);
	if (i == g_mapUnknownToUsage.end())
	{
		g_mapUnknownToUsage.insert(type, 1);
	}
	else
	{
		i->second++;
	}
}

#include <algorithm>

void PrintUnknownsAscending()
{
	if (g_mapUnknownToUsage.empty())
	{
		return;
	}

	struct Usage
	{
		cstr item;
		int count;
	};
	std::vector<Usage> elements;
	for (auto& i : g_mapUnknownToUsage)
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


/* Example:
(UEnum
	(FullName Enum /Script/Engine.EDataLayerRuntimeState)
	(MaxValue 3)
	:
	(Values
		(Unloaded 0)
		(Loaded 1)
		(Activated 2)
		(EDataLayerRuntimeState_MAX 3)
	)
)
*/

struct UnrealEnumDef: IUnrealEnumDef
{
	cr_sex sDef;
	HString name;
	HString package;
	int64 maxValue = 0;
	
	struct Entry
	{
		HString key;
		int64 value;
	};

	std::vector<Entry> entries;

	UnrealEnumDef(cr_sex _sDef): sDef(_sDef)
	{
		cr_sex sNameDef = sDef[1];
		cstr nameAndPackage = GetAtomicArg(sNameDef, 2).c_str();

		NamespaceSplitter splitter(nameAndPackage);

		cstr package, shortName;
		if (!splitter.SplitTail(OUT package, shortName))
		{
			Throw(sNameDef[2], "Cannot split %s into <package>.<name>", nameAndPackage);
		}

		this->package = package;
		this->name = shortName;

		cr_sex sMaxDef = sDef[2];
		ValidateToken(sMaxDef[0], "MaxValue", __FUNCTION__);

		maxValue = _atoi64(GetAtomicArg(sMaxDef, 1).c_str());

		cr_sex sValuesDef = sDef[4];
		ValidateToken(sValuesDef[0], "Values", __FUNCTION__);

		entries.reserve(sValuesDef.NumberOfElements() - 1);

		for (int i = 1; i < sValuesDef.NumberOfElements(); i++)
		{
			cr_sex sPair = sValuesDef[i];
			Entry e;
			e.key = GetAtomicArg(sPair, 0).c_str();
			e.value = _atoi64(GetAtomicArg(sPair, 1).c_str());
			entries.push_back(e);
		}
	}

	cstr Name() const override
	{
		return name;
	}

	cstr Package() const override
	{
		return package;
	}

	int64 MaxValue() const override
	{
		return maxValue;
	}

	int32 NumberOfKeys() const override
	{
		return (int32) entries.size();
	}

	cstr GetKey(int32 index) const override
	{
		return entries[index].key;
	}

	int64 GetValue(int32 index) const override
	{
		return entries[index].value;
	}

	size_t GetUnderlyingSize() const override
	{
		// Not sure how the engine works, but guessing this is the way. Compare this against sizeOf fields in struct generation to ensure correct logic
		// Any suggestions on how to get the correct underlying type for the enum, please email me.
		if (maxValue <= 255)
		{
			return sizeof(uint8);
		}

		if (maxValue <= 65535)
		{
			return sizeof(uint16);
		}

		if (maxValue <= 0xFFFF'FFFFLL)
		{
			return sizeof(uint32);
		}

		return sizeof(uint64);
	}
};

void GenEnumDef(IUnrealEnumDef& def, crwstr nativeDirectory);

stringmap<UnrealEnumDef*> g_unrealEnums; // This will leak, but get cleared up at exit.

IUnrealEnumDef* FindEnum(cstr name)
{
	auto i = g_unrealEnums.find(name);
	if (i != g_unrealEnums.end())
	{
		return i->second;
	}

	if (*name != 'E')
	{
		return nullptr;
	}

	if (EndsWith(name, "^"))
	{
		char valueName[256];
		CopyString(valueName, sizeof valueName, name, strlen(name) - 1);
		return FindEnum(valueName);
	}

	cstr firstColon = strstr(name, "::");
	if (firstColon)
	{
		char namespaceEnum[256];
		CopyString(namespaceEnum, sizeof namespaceEnum, name, firstColon - name);
		return FindEnum(namespaceEnum);
	}

	return nullptr;
}

void ParseEnumDef(cr_sex sDef)
{
	UnrealEnumDef* def = new UnrealEnumDef(sDef);
	GenEnumDef(*def, GetOutputDirectory());

	if (!g_unrealEnums.insert(def->Name(), def).second)
	{
		Throw(sDef, "Duplicate enum name: %s", def->Name());
	}
}

struct UnrealStructElement : IUnrealStructElement
{
	cr_sex eDef;
	HString typeName;
	HString fieldName;
	HString innerValueType;
	HString innerKeyType;
	int offset = 0;
	int sizeofStruct = 0;
	bool isBitField = false;

	UnrealStructElement(cr_sex _eDef): eDef(_eDef)
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

		if (Eq(typeName, "TMap"))
		{
			innerKeyType = GetAtomicArg(sNameSpec, 3).c_str();
			innerValueType = GetAtomicArg(sNameSpec, 4).c_str();
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

			cr_sex sSizeOfStruct = eDef[i + 1];
			ValidateToken(sSizeOfStruct[0], "SizeOf", __FUNCTION__);
			cstr txtSizeOfStruct = GetAtomicArg(sSizeOfStruct, 1).c_str();

			offset = atoi(txtOffset);
			sizeofStruct = atoi(txtSizeOfStruct);
		}

		cr_sex sFlags = eDef[eDef.NumberOfElements() - 1];
		ValidateToken(sFlags[0], "[]", __FUNCTION__);
		ValidateToken(sFlags[1], "Flags", __FUNCTION__);

		for (int i = 2; i < sFlags.NumberOfElements(); i++)
		{
			cstr flag = GetAtomicArg(sFlags, i).c_str();
			if (Eq(flag, "boolean"))
			{
				isBitField = true;
			}
		}
	}

	bool IsBitfield() const override
	{

		return isBitField;
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

	cstr InnerKeyType() const override
	{
		return innerKeyType;
	}

	cstr InnerValueType() const override
	{
		return innerValueType;
	}

	void Throw(cstr message) override
	{
		Rococo::Sex::Throw(eDef, "%s", message);
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

		auto byOffsetAscending = []	(const UnrealStructElement* a, const UnrealStructElement* b) -> bool
		{
			return a->offset < b->offset;
		};

		std::sort(elements.begin(), elements.end(), byOffsetAscending);
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

stringmap<UnrealStructDef*> knownStructs;

IUnrealStruct* FindStruct(cstr name)
{
	auto i = knownStructs.find(name);
	if (i == knownStructs.end())
	{
		if (EndsWith(name, "^"))
		{
			char sansRef[256];
			CopyString(sansRef, sizeof sansRef, name);
			size_t len = strlen(sansRef);
			sansRef[len - 1] = 0;
			return FindStruct(sansRef);
		}
	}

	return i == knownStructs.end() ? nullptr : i->second;
}

void ParseStructDef(cr_sex sDef)
{
	UnrealStructDef* def = new UnrealStructDef(sDef);
	try
	{
		GenStructDef(*def, GetOutputDirectory());
		if (knownStructs.insert(def->TypeName(), def).second == false)
		{
			Throw(sDef, "Duplicate struct name: %s.%s", def->Package(), def->TypeName());
		}
	}
	catch (...)
	{
		printf("Error generating struct definition for %s\n", def->TypeName());
		throw;
	}
}

void GenDelegateDef(cstr typeName, int sizeInBytes, crwstr path);

void ParseDelegateDef(cstr typeName, int sizeInBytes)
{
	GenDelegateDef(typeName, sizeInBytes, GetOutputDirectory());
}

bool IsCPPKeyword(cstr token)
{
	static stringmap<int> cppKeywords;
	if (cppKeywords.empty())
	{
		cppKeywords.insert("namespace", 0);
		cppKeywords.insert("this", 0);
		cppKeywords.insert("class", 0);
		cppKeywords.insert("struct", 0);
		cppKeywords.insert("if", 0);
		cppKeywords.insert("else", 0);
		cppKeywords.insert("do", 0);
		cppKeywords.insert("for", 0);
		cppKeywords.insert("while", 0);
		cppKeywords.insert("try", 0);
		cppKeywords.insert("catch", 0);
	}

	return cppKeywords.find(token) != cppKeywords.end();
}

void AppendIdentifier(StringBuilder& sb, cstr rawName)
{
	char firstChar = *rawName;
	static cstr prefix = "l"; // for 'local'

	if (isupper(firstChar))
	{
		if (isupper(rawName[1]))
		{
			// We have two or more capital letters, this could be an acronym, so we would not want to create a name like this: hAL, instead we add a prefix,  e.g lHAL
			sb << prefix;
		}
		else
		{
			char c = (char)tolower(firstChar);

			char camelCaseToken[256];
			SecureFormat(camelCaseToken, sizeof camelCaseToken, "%c%s", c, rawName + 1);

			if (IsCPPKeyword(camelCaseToken))
			{
				sb << prefix;
				camelCaseToken[0] = (char)toupper(c);
			}

			sb << camelCaseToken;
			return;
		}
	}

	sb << rawName;
}

namespace Rococo::IO
{
	void PrintOverwriteReport();
}

fstring enumAsBytePrefix = "TEnumAsByte<"_fstring;
fstring softObjectPtrPrefix = "TSoftObjectPtr<"_fstring;

bool IsKnownElementType(cstr argType)
{
	cstr p = argType;

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

	auto* enumType = FindEnum(p);
	if (enumType)
	{
		return true;
	}

	if (StartsWith(p, enumAsBytePrefix))
	{
		cstr innerTypeStart = p + enumAsBytePrefix.length;
		cstr templateDelimeter = FindChar(innerTypeStart, '>');
		if (!templateDelimeter)
		{
			Throw(0, "Cannot find template delimeter '>' for %s", p);
		}

		size_t len = templateDelimeter - innerTypeStart;

		char innerType[256];
		CopyString(innerType, sizeof innerType, innerTypeStart, len);
		enumType = FindEnum(innerType);
		if (enumType)
		{
			return true;
		}
	}

	if (StartsWith(p, subclassOfPrefix))
	{
		return true;
	}

	if (StartsWith(p, softObjectPtrPrefix))
	{
		return true;
	}

	if (*p == 'F')
	{
		auto* structType = FindStruct(p + 1);
		if (structType)
		{
			return true;
		}
	}

	if (EndsWith(argType, "^"))
	{
		char sansRef[256];
		CopyString(sansRef, sizeof sansRef, argType);
		sansRef[strlen(argType) - 1] = 0;
		return IsKnownElementType(sansRef);
	}

	return false;
}

bool IsArgKeyword(cstr token)
{
	if (!IsLowerCase(*token))
	{
		return false;
	}

	static stringmap<int> keywords;
	if (keywords.empty())
	{
		keywords.insert("const", 0);
		keywords.insert("out", 0);
		keywords.insert("return", 0);
		keywords.insert("visible", 0);
		keywords.insert("read-only", 0);
	}

	return keywords.find(token) != keywords.end();
}

bool LooksLikeObjectPointer(cstr p)
{
	if (*p == 'U' && EndsWith(p, "*"))
	{
		return true;
	}

	if (*p == 'A' && EndsWith(p, "*"))
	{
		return true;
	}

	if (*p == 'U' && EndsWith(p, "^"))
	{
		return true;
	}

	if (*p == 'A' && EndsWith(p, "^"))
	{
		return true;
	}

	return false;
}

static stringmap<int> g_UnresolvedArgType;

struct UnrealFunctionArg : IUnrealArg
{
	cr_sex sFnArgDef;
	HString argName;
	HString argType;
	HString elementType;
	HString keyType;
	bool isConst = false;
	bool isRef = false;
	bool isContainer = false;

	UnrealFunctionArg(cr_sex sFunctionArgDef): sFnArgDef(sFunctionArgDef)
	{
		for (int i = 0; i < sFunctionArgDef.NumberOfElements(); i++)
		{
			cr_sex s = sFunctionArgDef[i];
			if (Eq(s.c_str(), "const"))
			{
				isConst = true;
				continue;
			}
		}

		for (int i = 0; i < sFunctionArgDef.NumberOfElements(); i++)
		{
			cr_sex s = sFunctionArgDef[i];

			cstr p = s.c_str();

			if (IsArgKeyword(p))
			{
				continue;
			}
			
			if (Eq(p, "TArray") || Eq(p, "TSet"))
			{
				argType = p;
				elementType = sFunctionArgDef[i + 1].c_str();
				argName = sFunctionArgDef[i + 2].c_str();
				isContainer = true;
				break;
			}

			if (Eq(p, "TMap"))
			{
				argType = "TMap";
				keyType = sFunctionArgDef[i + 1].c_str();
				elementType = sFunctionArgDef[i + 2].c_str();
				argName = sFunctionArgDef[i + 3].c_str();
				isContainer = true;
				break;
			}

			if (Eq(p, "Delegate"))
			{
				argType = "TDelegate";
				elementType = sFunctionArgDef[i + 1].c_str();
				argName = sFunctionArgDef[i + 2].c_str();
				isContainer = true;

				cr_sex sLastArg = sFunctionArgDef[sFunctionArgDef.NumberOfElements() - 1];
				int delegateSize = atoi(sLastArg.c_str());

				AddDelegate(elementType, delegateSize);
				
				break;
			}

			if (argType.length() == 0)
			{
				argType = p;
				argName = sFunctionArgDef[sFunctionArgDef.NumberOfElements() - 1].c_str();
				break;
			}
		}

		if (argType.length() == 0)
		{
			Throw(0, "Indeterminate arg type");
		}
	}

	bool HasSexyCounterpart() const override
	{
		if (IsKnownElementType(argType))
		{
			return true;
		}

		if (Eq(argType, "TArray") || Eq(argType, "TSet"))
		{
			return IsKnownElementType(elementType);
		}

		if (Eq(argType, "TMap"))
		{
			return IsKnownElementType(elementType) && IsKnownElementType(keyType);
		}

		if (Eq(argType, "TDelegate"))
		{
			return IsKnownElementType(elementType);
		}

		g_UnresolvedArgType.insert(argType, 0);
				
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

	cstr ArgType() const override
	{
		return argType.c_str();
	}

	cstr ElementType() const override
	{
		return elementType.c_str();
	}

	cstr KeyType() const override
	{
		return keyType.c_str();
	}

	bool GetObjectPointerType(char* buffer, size_t capacity) const override
	{
		if (LooksLikeObjectPointer(argType))
		{
			SecureFormat(buffer, capacity, "%s", argType.c_str());
			return true;
		}

		if (elementType.length() > 0)
		{
			if (LooksLikeObjectPointer(elementType))
			{
				SecureFormat(buffer, capacity, "%s", elementType.c_str());
				return true;
			}
		}

		return false;
	}

	bool IsPtr() const override
	{
		return EndsWith(argType.c_str(), "*");
	}

	bool IsRef() const override
	{
		return EndsWith(argType.c_str(), "^") || EndsWith(elementType, "^");
	}

	bool IsConst() const override
	{
		return IsRef() && isConst;
	}

	bool IsContainer() const override
	{
		return isContainer;
	}

	bool IsCPPOutput() const override
	{
		cr_sex argDef = sFnArgDef;
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
	HString name;

	std::vector<UnrealFunctionArg*> args;

	UnrealFunctionDef(cr_sex f): fDef(f), name(GetAtomicArg(fDef, 2).c_str())
	{
		// Example: (' Method0 DivideFloats (visible read-only double A) (visible read-only double B) (out double NewParam))
		for (int i = 3; i < fDef.NumberOfElements(); i++)
		{
			try
			{
				args.push_back(new UnrealFunctionArg(fDef[i]));
			}
			catch (ParseException& pex)
			{
				Throw(*pex.Source(), "Error parsing method %s: %s", name.c_str(), pex.Message());
			}
			catch (IException& ex)
			{
				Throw(fDef[i], "Error parsing method %s: %s", name.c_str(), ex.Message());
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
		sb << name;

		if (Eq(name, "Construct") || Eq(name, "Destruct"))
		{
			// Reserved method names in Sexy. We add a suffix unlikely to conflict with other method names
			sb << "QQQ";
		}
	}

	cstr Name() const override
	{
		return name;
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

		try
		{
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
		catch (ParseException& pex)
		{
			Throw(*pex.Source(), "Error parsing class %s\n%s", name.c_str(), pex.Message());
		}
		catch (IException& ex)
		{
			Throw(sDef, "Error parsing class %s\n%s", name.c_str(), ex.Message());
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

void GenClassDef(IUnrealClass& classDef, crwstr nativeDirectory);

void ParseClassDef(cr_sex sDef, IClassSystem& classSystem)
{
	UnrealClassDef* def = new UnrealClassDef(sDef);
	classSystem.AddClass(*def);
	GenClassDef(*def, GetOutputDirectory());

	allClasses.push_back(def);

	g_nClassesParsed++;
	g_nMethodsParsed += (int) def->MethodCount();

	for (int i = 0; i < def->MethodCount(); i++)
	{
		auto& method = def->GetFunction(i);
		if (!method.HasSexyCounterpart())
		{
			g_nMethodsNotMarshaled++;
		}
	}
}

WideFilePath g_outDir;

crwstr GetOutputDirectory()
{
	return g_outDir;
}

int mainProtected(int argc, char* argv[])
{
	AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(32768, 0, "main");
	Auto<ISParser> sParser = CreateSexParser_2_0(*allocator, SEXY_STANDARD_MAX_ATOMIC_STRING_LENGTH);

	fstring sexml = "-SEXML:"_fstring;
	fstring output = "-OUTDIR:"_fstring;

	cstr sexmlFile = nullptr;
	cstr outDir = nullptr;

	for (int i = 0; i < argc; i++)
	{
		cstr arg = argv[i];
		if (StartsWith(arg, sexml))
		{
			sexmlFile = arg + sexml.length;
		}

		if (StartsWith(arg, output))
		{
			outDir = arg + output.length;
		}
	}

	if (sexmlFile == nullptr || outDir == nullptr)
	{
		printf("Usage: unreal-sexy-gen.exe -SEXML:<sexml-file-path-and-filename> -OUTDIR:<output-directory>\n");
		return -1;
	}

	Format(g_outDir, L"%hs%hs", outDir, EndsWith(outDir, "\\") ? "" : "\\");

	if (!Rococo::IO::IsDirectory(g_outDir.buf))
	{
		printf("%ws does not appear to be a directory", g_outDir.buf);
		return -1;
	}

	WideFilePath wPath;
	Format(wPath, L"%hs", sexmlFile);
	ParseClassFile(wPath, *sParser);

	PrintUnknownsAscending();

	printf("\nNumber of classes: %d\n", g_nClassesParsed);
	printf("Number of methods in API: %d\n", g_nMethodsParsed);
	printf("Number of methods that could not be marshaled: %d\n", g_nMethodsNotMarshaled);

	if (!g_UnresolvedArgType.empty())
	{
		printf("Unknown arg types:\n");

		for (auto& p : g_UnresolvedArgType)
		{
			printf("%s\n", (cstr)p.first);
		}

		printf("\n");
	}

	double failureRate = 100.0 * (double)g_nMethodsNotMarshaled / (double)g_nMethodsParsed;
	printf("API coverage: %2.2f%%\n", 100.0 - failureRate);

	IO::PrintOverwriteReport();

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
		return ex.ErrorCode() != 0 ? ex.ErrorCode() : -1;
	}
}