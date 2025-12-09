#include "unreal-sexy-gen.h"
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.hashtable.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Unreal;

extern fstring enumAsBytePrefix;
extern fstring softObjectPtrPrefix;
extern fstring subclassOfPrefix;
extern fstring scriptInterfacePrefix;
extern fstring objectPtrPrefix;

fstring delegatePrefix = "TDelegate<"_fstring;

void BuildSexyNativesCPP(IUnrealClass& classDef, StringBuilder& sb);
void BuildSexyNativesHPP(IUnrealClass& classDef, StringBuilder& sb);
void BuildSexyFiles(IUnrealClass& classDef, StringBuilder& sb);
IUnrealStruct* FindStruct(cstr name);
IUnrealEnumDef* FindEnum(cstr name);

const bool UsePackageForFolders = false;

namespace Rococo::IO
{
	int g_redundantWrites = 0;
	int g_overwrites = 0;

	void SaveAsciiTextFileIfDifferentAndLog(TargetDirectory target, crwstr filename, const fstring& text)
	{
		bool isMatch = IO::SaveAsciiTextFileIfDifferent(target, filename, text);
		if (isMatch)
		{
			g_redundantWrites++;
		}
		else
		{
			g_overwrites++;
		}
	}

	void PrintOverwriteReport()
	{
		printf("Files written: %d. Files unchanged: %d\n", g_overwrites, g_redundantWrites);
	}
}

void GenClassDef(IUnrealClass& classDef, crwstr nativeDirectory, crwstr sexyDirectory)
{
	cstr shortName = classDef.ShortName();
	cstr packageName = UsePackageForFolders ? classDef.PackageName() : "";

	if (*packageName == '/')
	{
		packageName++;
	}

	AutoFree<IDynamicStringBuilder> dsbCPP = CreateDynamicStringBuilder(64_kilobytes);

	// .cpp builder
	auto& sbCPP = dsbCPP->Builder();

	AutoFree<IDynamicStringBuilder> dsbHPP = CreateDynamicStringBuilder(16_kilobytes);

	// .HPP builder
	auto& sbHPP = dsbHPP->Builder();

	AutoFree<IDynamicStringBuilder> dsbSXY = CreateDynamicStringBuilder(64_kilobytes);

	// .sxy builder
	auto& sbSXY = dsbSXY->Builder();

	WideFilePath wTargetCPPFile;
	Format(wTargetCPPFile, L"%ls%hs\\%hs.cpp", nativeDirectory, packageName, shortName);
	IO::ToSysPath(wTargetCPPFile.buf);

	WideFilePath wTargetHPPFile;
	Format(wTargetHPPFile, L"%ls%hs\\%hs.hpp", nativeDirectory, packageName, shortName);
	IO::ToSysPath(wTargetHPPFile.buf);

	WideFilePath wTargetSXYFile;
	Format(wTargetSXYFile, L"%ls%hs\\%hs.sxy", sexyDirectory, packageName, shortName);
	IO::ToSysPath(wTargetSXYFile.buf);

	BuildSexyNativesCPP(classDef, sbCPP);
	BuildSexyNativesHPP(classDef, sbHPP);
	BuildSexyFiles(classDef, sbSXY);

	IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetCPPFile, *sbCPP);
	IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
	IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetSXYFile, *sbSXY);
}

void AppendCompactName(StringBuilder& sb, cstr p)
{
	while (*p != 0)
	{
		char c = *p++;
		switch (c)
		{
		case '-':
		case ' ':
			sb.AppendChar('_');
			break;
		default:
			sb.AppendChar(c);
			break;
		}
	}
}

void BuildCPPInputsAndOutputs(std::vector<IUnrealArg*>& inputs, std::vector<IUnrealArg*>& outputs, IUnrealFunction& method)
{
	inputs.clear();
	outputs.clear();

	size_t j = 0;
	for (;;)
	{
		auto* arg = method.GetArg(j++);
		if (!arg)
		{
			break;
		}

		if (arg->IsCPPOutput())
		{
			outputs.push_back(arg);
		}
		else
		{
			inputs.push_back(arg);
		}
	}
}

void BuildSexyInputsAndOutputs(std::vector<IUnrealArg*>& inputs, std::vector<IUnrealArg*>& outputs, IUnrealFunction& method)
{
	inputs.clear();
	outputs.clear();

	size_t j = 0;
	for (;;)
	{
		auto* arg = method.GetArg(j++);
		if (!arg)
		{
			break;
		}

		if (arg->IsSexyOutput())
		{
			outputs.push_back(arg);
		}
		else
		{
			inputs.push_back(arg);
		}
	}
}

void MarshalNameTypeAsHandle(StringBuilder& sb, cstr nameType, bool makeSexyVariableType)
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

const IMarshalType* FindPrimitiveType(cstr argType);
void MarkUnknown(cstr type);

void AppendTypeSansRef(StringBuilder& sb, cstr argType)
{
	cstr p = argType;
	while (*p != 0 && *p != '^')
	{
		sb.AppendChar(*p++);
	}
}

bool TryGetEnumAsByte(char* innerType, size_t capacity, cstr argType)
{
	if (StartsWith(argType, enumAsBytePrefix))
	{
		cstr innerTypeStart = argType + enumAsBytePrefix.length;
		cstr templateDelimeter = FindChar(innerTypeStart, '>');
		if (!templateDelimeter)
		{
			Throw(0, "Cannot find template delimeter '>' for %s", argType);
		}

		size_t len = templateDelimeter - innerTypeStart;

		CopyString(innerType, capacity, innerTypeStart, len);
		auto* enumType = FindEnum(innerType);
		if (enumType)
		{
			return true;
		}
	}

	return false;
}

int FindDelegateSize(cstr name);

void AppendNonContainerType_Private(StringBuilder& sb, cstr argType, bool makeSexyVariableType)
{
	if (EndsWith(argType, "^"))
	{
		char valueType[256];
		CopyString(valueType, sizeof valueType, argType, strlen(argType) - 1);
		AppendNonContainerType_Private(sb, valueType, makeSexyVariableType);
		return;
	}

	if (*argType == 'U' && EndsWith(argType, "*"))
	{
		MarshalNameTypeAsHandle(sb, argType, makeSexyVariableType);
		return;
	}

	if (*argType == 'A' && EndsWith(argType, "*"))
	{
		MarshalNameTypeAsHandle(sb, argType, makeSexyVariableType);
		return;
	}

	auto* primitive = FindPrimitiveType(argType);
	if (primitive)
	{
		sb << primitive->CPPName();
		return;
	}

	auto* enumType = FindEnum(argType);
	if (enumType)
	{
		sb << "R_" << enumType->Name();
		return;
	}

	int delegateSize = FindDelegateSize(argType);
	if (delegateSize)
	{
		sb << argType;
		return;
	}

	char innerType[256];
	if (TryGetEnumAsByte(innerType, sizeof innerType, argType))
	{
		auto* enumRef = FindEnum(innerType);
		if (!enumRef)
		{
			Throw(0, "Could not find inner type %s", innerType);
		}
		sb.AppendFormat("R_TEnumAsByte<R_%s>", enumRef->Name());
		return;
	}

	if (*argType == 'F')
	{
		auto* structType = FindStruct(argType + 1);
		if (structType)
		{
			sb << "R_" << argType;
			return;
		}
	}

	if (StartsWith(argType, softObjectPtrPrefix))
	{
		cstr endToken = FindChar(argType, '>');
		cstr startToken = argType + softObjectPtrPrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		sb.AppendFormat("R_TSoftObjectPtr<%s>", innerType);
		return;
	}

	if (StartsWith(argType, subclassOfPrefix))
	{
		cstr endToken = FindChar(argType, '>');
		cstr startToken = argType + subclassOfPrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		sb.AppendFormat("R_TSubclassOf<%s>", innerType);
		return;
	}

	if (StartsWith(argType, scriptInterfacePrefix))
	{
		cstr endToken = FindChar(argType, '>');
		cstr startToken = argType + scriptInterfacePrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		sb.AppendFormat("R_TScriptInterface<%s>", innerType);
		return;
	}

	if (StartsWith(argType, objectPtrPrefix))
	{
		cstr endToken = FindChar(argType, '>');
		cstr startToken = argType + objectPtrPrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		sb.AppendFormat("R_TObjectPtr<%s>", innerType);
		return;
	}

	MarkUnknown(argType);

	sb << "UnknownType /*";
	AppendTypeSansRef(sb, argType);
	sb << "*/";
}

void AppendType(StringBuilder& sb, IUnrealArg& arg, bool makeSexyVariableType)
{
	cstr argType = arg.ArgType();

	if (arg.IsContainer())
	{
		sb << "R_";
		sb << arg.ArgType();
		sb << "<";
		if (*arg.KeyType() != 0)
		{
			AppendNonContainerType_Private(sb, arg.KeyType(), makeSexyVariableType);
			sb << ",";
		}

		if (Eq(argType, "TDelegate"))
		{
			sb << "R_";
		}

		AppendNonContainerType_Private(sb, arg.ElementType(), makeSexyVariableType);
		sb << ">";
	}
	else
	{
		AppendNonContainerType_Private(sb, argType, makeSexyVariableType);
	}
}


void BuildMethod(IUnrealClass& classDef, IUnrealFunction& method, StringBuilder& sb)
{
	sb << "\n\tvoid ";
	AppendCompactName(sb, classDef.ShortName());
	sb << "_";
	method.AppendFunctionName(sb);
	sb << "(NativeCallEnvironment& nce)\n";
	sb << "\t{\n";

	sb << "\t\tuint8* sf = nce.cpu.SF();\n";
	sb << "\t\tptrdiff_t offset = 2 * sizeof(size_t);\n\n";

	sb << "\t\tstruct SexyArgs_";
	method.AppendFunctionName(sb);
	sb << "\n\t\t{\n";

	size_t j = 0;
	for (;;)
	{
		auto* arg = method.GetArg(j++);
		if (arg == nullptr)
		{
			break;
		}

		sb << "\t\t\t";

		AppendType(sb, *arg, false);

		if (arg->IsRef() && !arg->IsPtr())
		{
			sb << "*";
		}

		// Add a prefix to the name, in case the variable name conflicts with a C++ keyword
		sb << " m_";

		arg->AppendName(sb);

		sb << ";\n";
	}

	sb << "\t\t} ";

	sb << "args;\n\n";

	std::vector<IUnrealArg*> inputs;
	std::vector<IUnrealArg*> outputs;
	BuildCPPInputsAndOutputs(REF inputs, REF outputs, method);

	sb << "\t\tint64 objectHandle;\n";
	sb << "\t\toffset += sizeof(int64);\n";
	sb << "\t\tReadInput(objectHandle, sf, -offset);\n\n";
	
	for (auto* input : inputs)
	{
		sb << "\t\t";

		AppendType(sb, *input, false);

		sb << " in_";
		input->AppendName(sb, true);
		sb << ";\n";

		sb << "\t\toffset += sizeof(in_";
		input->AppendName(sb, true);
		sb << ");\n";

		sb << "\t\tReadInput(in_";
		input->AppendName(sb, true);
		sb << ", sf, -offset);\n";

		sb << "\t\targs.m_";
		input->AppendName(sb, false);
		sb << " = ";

		if (input->IsRef())
		{
			sb << "&";
		}

		sb << "in_";
		input->AppendName(sb, true);
		sb << ";\n\n";
	}

	sb << "\t\tUObject* object = GetNCEUObject(nce, objectHandle);\n";
	sb << "\t\tUMethod* methodRef = GetNCEUMethod(nce);\n";
	sb << "\t\tValidateArgs(methodRef, &args, sizeof(args));\n";
	sb << "\t\tProcessEvent(object, methodRef, &args);\n";

	for (auto* output : outputs)
	{
		sb << "\n\t\t";

		AppendType(sb, *output, false);

		if (output->IsRef() && !output->IsPtr())
		{
			sb << "*";
		}

		sb << " out_";
		output->AppendName(sb, true);
		sb << " = args.m_";
		output->AppendName(sb, false);
		sb << ";\n";

		sb << "\t\toffset += sizeof(out_";
		output->AppendName(sb, true);
		sb << ");\n";

		sb << "\t\tWriteOutput(out_";
		output->AppendName(sb, true);
		sb << ", sf, -offset);\n";
	}

	sb << "\t}\n";
}

#include <ctype.h>

void AppendNameAsSxyType(StringBuilder& sb, IUnrealClass& classRef)
{
	cstr className = classRef.ShortName();
	
	char firstChar = *className++;

	if (IsLowerCase(firstChar))
	{
		// Lower case - but Sexy namespaces must be pascal case, so convert to pascal case
		sb.AppendChar((char)toupper(firstChar));
	}
	else if (isupper(firstChar))
	{
		sb.AppendChar(firstChar);
	}
	else if (IsNumeric(firstChar))
	{
		// Illegal first character, so add a prefix
		sb << "C";
		sb.AppendChar(firstChar);
	}
	else
	{
		Throw(0, "Cannot transform class name to Sexy type name. Bad first character: %s %s", classRef.PackageName(), classRef.ShortName());
	}

	bool pascalize = false;

	for (cstr p = className; *p != 0; p++)
	{
		char c = *p;
		if (c == '_')
		{
			pascalize = true;
			continue; // Skip underscores
		}
		else if (!isalnum(c))
		{
			Throw(0, "Cannot transform class name to Sexy type name. None alphanumeric character in short name: %s %s", classRef.PackageName(), classRef.ShortName());
		}

		if (pascalize)
		{
			pascalize = false;
			if (islower(c))
			{
				sb.AppendChar((char)toupper(c));
				continue;
			}
		}

		sb.AppendChar(c);		
	}
}

void AppendPackageAsSexyNamespace(StringBuilder& sb, IUnrealClass& classRef)
{
	// Example /Engine/AI/
	cstr packageName = classRef.PackageName();
	if (*packageName == '/')
	{
		packageName++;
	}

	char firstChar = *packageName++;

	if (IsLowerCase(firstChar))
	{
		// Lower case - but Sexy namespaces must be pascal case, so convert to pascal case
		sb.AppendChar((char)toupper(firstChar));
	}
	else if (isupper(firstChar))
	{
		sb.AppendChar(firstChar);
	}
	else if (IsNumeric(firstChar))
	{
		// Illegal first character, so add a prefix
		sb << "NS";
		sb.AppendChar(firstChar);
	}
	else
	{
		Throw(0, "Cannot transform package name to Sexy namespace. Bad first character: %s %s", classRef.PackageName(), classRef.ShortName());
	}

	int subspaceCharCount = 1;

	for (cstr p = packageName; *p != 0; p++)
	{
		char c = *p;

		if (c == '/')
		{
			if (subspaceCharCount > 0)
			{
				sb.AppendChar('.');
				subspaceCharCount = 0;
			}
		}
		else if (IsAlphaNumeric(c))
		{
			if (subspaceCharCount == 0)
			{
				// We must begin with a capital letter
				if (islower(c))
				{
					sb.AppendChar((char)toupper(c));
				}
				else if (isupper(c))
				{
					sb.AppendChar(c);
				}
				else
				{
					sb << "N"; // For want of a better prefix
					sb.AppendChar(c);
				}
			}
			else
			{
				sb.AppendChar(c);
			}
			subspaceCharCount++;
		}
		else
		{
			// Skip character, assume it to be blankspace
		}
	}

	if (subspaceCharCount == 0)
	{
		// We finished on a trailing dot, which is not permitted
		sb.Undo(-1);
	}
}

template<class LAMBDA>
void ForEachArgumentOfEachMethod(IUnrealClass& classDef, LAMBDA lambda)
{
	for (size_t i = 0; i < classDef.MethodCount(); i++)
	{
		auto& method = classDef.GetFunction(i);
		size_t j = 0;
		for (;;)
		{
			auto* arg = method.GetArg(j++);
			if (arg == nullptr)
			{
				break;
			}

			lambda(*arg);
		}
	}
}

void GetTypeWithoutRef(char* buffer, size_t capacity, cstr type)
{
	CopyString(buffer, capacity, type);
	if (EndsWith(buffer, "^"))
	{
		buffer[strlen(buffer) - 1] = 0;
	}
}

void AppendHeaders(stringmap<int>& requiredStructs, StringBuilder& sb, IUnrealArg& arg, cstr argType)
{
	if (*argType == 'F')
	{
		auto* structType = FindStruct(argType + 1);
		if (structType)
		{
			if (requiredStructs.insert(structType->TypeName(), 0).second)
			{
				sb.AppendFormat("#include \"Struct/%s.hpp\"\n", structType->TypeName());
				return;
			}
		}
	}

	else if (*argType == 'T')
	{
		char innerType[256];
		if (TryGetEnumAsByte(innerType, sizeof innerType, argType))
		{
			if (requiredStructs.insert(innerType, 0).second)
			{
				auto* enumDef = FindEnum(innerType);
				sb.AppendFormat("#include \"Enum/%s.hpp\"\n", enumDef->Name());
				return;
			}
		}
	}

	auto* enumDef = FindEnum(argType);
	if (enumDef)
	{
		if (requiredStructs.insert(enumDef->Name(), 0).second)
		{
			sb.AppendFormat("#include \"Enum/%s.hpp\"\n", enumDef->Name());
			return;
		}
	}

	if (Eq(argType, "TDelegate"))
	{
		char valueWithoutRef[256];
		GetTypeWithoutRef(valueWithoutRef, sizeof valueWithoutRef, arg.ElementType());
		if (requiredStructs.insert(valueWithoutRef, 0).second)
		{
			sb.AppendFormat("#include \"Delegate/%s.hpp\"\n", valueWithoutRef);
			return;
		}
	}
}

void AppendHeaders(stringmap<int>& requiredStructs, StringBuilder& sb, IUnrealArg& arg)
{
	if (arg.IsContainer())
	{
		if (*arg.KeyType())
		{
			AppendHeaders(requiredStructs, sb, arg, arg.KeyType());
		}
		AppendHeaders(requiredStructs, sb, arg, arg.ElementType());
	}

	AppendHeaders(requiredStructs, sb, arg, arg.ArgType());
}

bool TryGetInnerType(char* innerType, size_t capacity, cstr argType, fstring prefix)
{
	if (StartsWith(argType, prefix))
	{
		cstr endToken = FindChar(argType, '>');
		cstr startToken = argType + prefix.length;
		CopyString(innerType, capacity, startToken, endToken - startToken);
		return true;
	}

	return false;
}

bool TryGetInnerType(char* innerType, size_t capacity, cstr argType, fstring prefices[])
{
	for (fstring* p = prefices; p->length != 0; p++)
	{
		if (TryGetInnerType(innerType, capacity, argType, *p))
		{
			return true;
		}
	}

	return false;
}

void BuildSexyNativesCPP(IUnrealClass& classDef, StringBuilder& sb)
{
	sb <<

		R"(// Code generated by unreal-sexy.gen.core.cpp
#include "../unreal-sexy-marshalling.h"
)";

	stringmap<int> requiredStructs;

	ForEachArgumentOfEachMethod(classDef,
		[&requiredStructs, &sb](IUnrealArg& arg)
		{
			AppendHeaders(REF requiredStructs, sb, arg);
		}
	);

	sb << "\n";

	stringmap<int> knownObjects;
	knownObjects.insert("UClass", 0);
	knownObjects.insert("UObject", 0);
	knownObjects.insert("UMethod", 0);
	knownObjects.insert("UnknownType", 0);

	ForEachArgumentOfEachMethod(classDef,
		[&knownObjects,&sb](IUnrealArg& arg)
		{
			char objectPointerType[128];
			if (arg.GetObjectPointerType(objectPointerType, sizeof objectPointerType))
			{
				// Zap the trailing *
				char* finalBit = objectPointerType + strlen(objectPointerType) - 1;
				for (;; finalBit--)
				{
					switch (*finalBit)
					{
					case '*':
					case '^':
						*finalBit = 0;
						break;
					default:
						goto next;
					}
				}

				next:

				if (knownObjects.find(objectPointerType) == knownObjects.end())
				{
					// New object
					knownObjects.insert(objectPointerType, 0);

					sb << "class " << objectPointerType << ";\n";
				}

				return;
			}

			fstring uobjectContainerPrefices[] = { softObjectPtrPrefix, subclassOfPrefix, scriptInterfacePrefix, objectPtrPrefix, ""_fstring };

			char innerType[256];
			if (TryGetInnerType(innerType, sizeof innerType, arg.ArgType(), uobjectContainerPrefices))
			{
				if (knownObjects.find(innerType) == knownObjects.end())
				{
					// New object
					knownObjects.insert(innerType, 0);
					sb << "class " << innerType << ";\n";
				}
			}

			if (Eq(arg.ArgType(), "TArray") || Eq(arg.ArgType(), "TSet"))
			{
				if (TryGetInnerType(innerType, sizeof innerType, arg.ElementType(), uobjectContainerPrefices))
				{
					if (knownObjects.find(innerType) == knownObjects.end())
					{
						// New object
						knownObjects.insert(innerType, 0);
						sb << "class " << innerType << ";\n";
					}
				}
			}

			if (Eq(arg.ArgType(), "TMap"))
			{
				if (TryGetInnerType(innerType, sizeof innerType, arg.KeyType(), uobjectContainerPrefices))
				{
					if (knownObjects.find(innerType) == knownObjects.end())
					{
						// New object
						knownObjects.insert(innerType, 0);
						sb << "class " << innerType << ";\n";
					}
				}

				if (TryGetInnerType(innerType, sizeof innerType, arg.ElementType(), uobjectContainerPrefices))
				{
					if (knownObjects.find(innerType) == knownObjects.end())
					{
						// New object
						knownObjects.insert(innerType, 0);
						sb << "class " << innerType << ";\n";
					}
				}
			}
		}
	);

	sb << R"(
using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::UE::Native;
using namespace Rococo::UE::Native::Delegate;
using namespace Rococo::UE::Native::Enum;
using namespace Rococo::UE::Native::Struct;
using namespace Rococo::UE5::Marshal;

namespace 
{
)";

	sb << "\tvoid Construct_";
	AppendCompactName(sb, classDef.ShortName());
	sb << "(NativeCallEnvironment& nce)\n";
	sb << "\t{\n";

	sb << "\t\tuint8* sf = nce.cpu.SF();\n";
	sb << "\t\tptrdiff_t offset = 2 * sizeof(size_t);\n\n";
	sb << "\t\tint64 objectHandle = ConstructUObject(nce);\n";
	sb << "\t\toffset += sizeof(objectHandle);\n";
	sb << "\t\tWriteOutput(objectHandle, sf, -offset);\n";
	sb << "\t};\n";

	for (size_t i = 0; i < classDef.MethodCount(); i++)
	{
		auto& method = classDef.GetFunction(i);
		BuildMethod(classDef, method, sb);
	}

	sb << "}\n\n";

	sb << "namespace Rococo::UE::Native\n{\n";
	sb.AppendFormat("\tvoid AddSexyNatives_Unreal_%s(IPublicScriptSystem& ss, UClass* classRef)\n", classDef.ShortName());
	sb << "\t{\n";

	sb << "\t\tconst INamespace& nsHandles = ss.AddNativeNamespace(\"UE.Handles\");\n";
	for (auto& known : knownObjects)
	{
		cstr type = known.first;
		sb.AppendFormat("\t\tss.CreateHandleType(nsHandles, __FUNCTION__, __LINE__, \"H%s\");\n", type);
	}

	sb << "\n";

	sb << "\t\tconst INamespace& ns = ss.AddNativeNamespace(\"";
	
	sb << "UE.Native.";

	AppendPackageAsSexyNamespace(sb, classDef);
			
	sb << "\");\n";

	std::vector<IUnrealArg*> inputs;
	std::vector<IUnrealArg*> outputs;

	sb << "\t\tss.AddNativeCall(ns, Construct_";
		
	AppendCompactName(sb, classDef.ShortName());
		
	sb <<", classRef, \"Construct";
	AppendCompactName(sb, classDef.ShortName());
	sb << " -> (Int64 objectHandle)\", __FILE__, __LINE__);\n";

	for (size_t i = 0; i < classDef.MethodCount(); i++)
	{
		auto& method = classDef.GetFunction(i);

		if (!method.HasSexyCounterpart())
		{
			sb << "// ";
		}

		sb << "\t\tss.AddNativeCall(ns, ";
		AppendCompactName(sb, classDef.ShortName());
		sb << "_";
		method.AppendFunctionName(sb);
		sb << ", &GetMethod(*classRef, L\"";

		method.AppendFunctionName(sb);

		sb << "\"), \"";
		method.AppendFunctionName(sb);

		BuildSexyInputsAndOutputs(REF inputs, REF outputs, method);

		sb << " (int64 objectHandle)";

		if (!inputs.empty())
		{
			sb << " ";
		}

		for (auto* input : inputs)
		{
			sb << "(";
			AppendType(sb, *input, true);
			sb << " ";
			input->AppendName(sb, true);
			sb << ")";
		}

		sb << " -> ";

		for (auto* output : outputs)
		{
			sb << "(";
			AppendType(sb, *output, true);
			sb << " ";
			output->AppendName(sb, true);
			sb << ")";
		}

		sb << "\", __FILE__, __LINE__);\n";
	}

	sb << "\t}\n";
	sb << "}\n";
}

void AppendIdentifier(StringBuilder& sb, cstr rawName);

stringmap<int> dependencies;

struct HardCodedType
{
	cstr typeName;
	cstr header;
};

stringmap<HardCodedType> hardcodedTypes;

cstr STRUCT_PREFIX = "R_F";

void BuildHardCodedTypes()
{
	if (!hardcodedTypes.empty())
	{
		return;
	}

	hardcodedTypes.insert("uint8", { "uint8", nullptr });
	hardcodedTypes.insert("double", { "double", nullptr });
	hardcodedTypes.insert("float", { "float", nullptr });
	hardcodedTypes.insert("bool",  { "bool", nullptr });
	hardcodedTypes.insert("int32", { "int32", nullptr });
	hardcodedTypes.insert("int64", { "int64", nullptr });
	hardcodedTypes.insert("FName", { "R_FName", nullptr });
	hardcodedTypes.insert("FString", { "R_FString", nullptr });
	hardcodedTypes.insert("FText", { "R_FText", nullptr });
}

void InnerUnrealTypeToMarshalledType(char* buffer, size_t capacity, cstr unrealType)
{
	if (Eq(unrealType, "TArray"))
	{
		Throw(0, "Cannot handle TArray as an inner type");
		return;
	}

	if (StartsWith(unrealType, enumAsBytePrefix))
	{
		char innerType[256];
		cstr endToken = FindChar(unrealType, '>');
		cstr startToken = unrealType + enumAsBytePrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		SafeFormat(buffer, capacity, "uint8");
		return;
	}

	if (StartsWith(unrealType, subclassOfPrefix))
	{
		char innerType[256];
		cstr endToken = FindChar(unrealType, '>');
		cstr startToken = unrealType + subclassOfPrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		SafeFormat(buffer, capacity, "RF_SubclassOf");
		return;
	}

	if (StartsWith(unrealType, objectPtrPrefix))
	{
		char innerType[256];
		cstr endToken = FindChar(unrealType, '>');
		cstr startToken = unrealType + objectPtrPrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		SafeFormat(buffer, capacity, "R_TObjectPtr<%s>", innerType);
		return;
	}

	auto* enumDef = FindEnum(unrealType);
	if (enumDef)
	{
		SafeFormat(buffer, capacity, "Enum::R_%s", unrealType);
		return;
	}

	auto h = hardcodedTypes.find(unrealType);
	if (h != hardcodedTypes.end())
	{
		SafeFormat(buffer, capacity, "%s", h->second.typeName);
		return;
	}
	
	SafeFormat(buffer, capacity, "%s%s", STRUCT_PREFIX, unrealType + 1);	
}

void AppendHeaderForType(StringBuilder& sb, cstr typeName)
{
	if (StartsWith(typeName, softObjectPtrPrefix))
	{
		return;
	}

	if (StartsWith(typeName, objectPtrPrefix))
	{
		return;
	}

	if (StartsWith(typeName, subclassOfPrefix))
	{
		return;
	}

	char innerType[256];

	if (StartsWith(typeName, enumAsBytePrefix))
	{
		cstr endToken = FindChar(typeName, '>');
		cstr startToken = typeName + enumAsBytePrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		AppendHeaderForType(sb, innerType);
		return;
	}

	fstring prefices[] = { softObjectPtrPrefix, subclassOfPrefix, ""_fstring };
	if (TryGetInnerType(innerType, sizeof innerType, typeName, prefices))
	{
		AppendHeaderForType(sb, innerType);
		return;
	}

	auto* enumDef = FindEnum(typeName);
	if (enumDef)
	{
		sb.AppendFormat("#include \"Enum/%s.hpp\"\n", enumDef->Name());
		return;
	}

	if (StartsWith(typeName, "E"))
	{
		Throw(0, "Expecting Enumeration name %s, but it was not a known enumeration type", typeName);
	}

	auto h = hardcodedTypes.find(typeName);
	if (h != hardcodedTypes.end())
	{
		if (h->second.header != nullptr)
		{
			sb.AppendFormat("#include \"%s.hpp\"\n", h->second.header);
		}
	}
	else
	{
		auto d = dependencies.insert(typeName, 0);
		if (d.second)
		{
			sb.AppendFormat("#include \"%s.hpp\"\n", typeName + 1);
		}
	}
}

void AddDelegate(cstr elementType, int delegateSize);

void BuildSexyNativeStructsHPP(IUnrealStruct& structDef, StringBuilder& sb)
{
	BuildHardCodedTypes();

	size_t nElements = structDef.ElementCount();

	sb <<
		R"(#pragma once
// Code generated by unreal-sexy.gen.core.cpp
#include "../../unreal-sexy-marshalling.h"
)";

	dependencies.clear();

	for (size_t i = 0; i < nElements; i++)
	{
		auto& e = structDef[i];

		if (Eq(e.TypeName(), "TArray") || Eq(e.TypeName(), "TSet"))
		{
			AppendHeaderForType(sb, e.InnerValueType());
			continue;
		}
		else if (Eq(e.TypeName(), "TMap"))
		{
			AppendHeaderForType(sb, e.InnerKeyType());
			AppendHeaderForType(sb, e.InnerValueType());
			continue;
		}
		else if (StartsWith(e.TypeName(), "TDelegate<"))
		{
			char delegateType[256];
			if (TryGetInnerType(delegateType, sizeof delegateType, e.TypeName(), "TDelegate<"_fstring))
			{
				sb.AppendFormat("#include \"Delegate/%s.hpp\"\n", delegateType);
				AddDelegate(delegateType, e.SizeOf());
			}
			continue;
		}
		else
		{
			AppendHeaderForType(sb, e.TypeName());
		}
	}

	sb << "\n";

	for (size_t i = 0; i < nElements; i++)
	{
		auto& e = structDef[i];

		char innerType[256];

		if (*e.TypeName() == 'F' && EndsWith(e.TypeName(), "Delegate"))
		{
			sb << "class R_" << e.TypeName() << ";\n";
		}
		else if (TryGetInnerType(innerType, sizeof innerType, e.TypeName(), objectPtrPrefix))
		{
			sb << "class " << innerType << ";\n";
		}
		else if (Eq(e.TypeName(), "TArray") || Eq(e.TypeName(), "TSet"))
		{
			if (TryGetInnerType(innerType, sizeof innerType, e.InnerValueType(), objectPtrPrefix))
			{
				sb << "class " << innerType << ";\n";
			}
		}
		else if (Eq(e.TypeName(), "TMap"))
		{
			if (TryGetInnerType(innerType, sizeof innerType, e.InnerValueType(), objectPtrPrefix))
			{
				sb << "class " << innerType << ";\n";
			}

			if (TryGetInnerType(innerType, sizeof innerType, e.InnerKeyType(), objectPtrPrefix))
			{
				sb << "class " << innerType << ";\n";
			}
		}
		else if (TryGetInnerType(innerType, sizeof innerType, e.TypeName(), softObjectPtrPrefix))
		{
			sb << "class " << innerType << ";\n";
		}
		else if (TryGetInnerType(innerType, sizeof innerType, e.TypeName(), subclassOfPrefix))
		{
			sb << "class " << innerType << ";\n";
		}
	}

	sb << R"(
namespace Rococo::UE::Native::Struct
{
)";
	if (structDef.Alignment() > 0)
	{
		sb.AppendFormat("\tstruct alignas(%d) %s%s\n", structDef.Alignment(), STRUCT_PREFIX, structDef.TypeName());
	}
	else
	{
		sb.AppendFormat("\tstruct %s%s\n", STRUCT_PREFIX, structDef.TypeName());
	}
	sb << "\t{\n";

	int currentOffset = 0;

	int paddingIndex = 0;
	
	for (size_t i = 0; i < nElements; i++)
	{
		auto& e = structDef[i];
		if (e.Offset() > currentOffset)
		{
			sb.AppendFormat("\t\tchar _padding_%d[%d];\n", paddingIndex++, e.Offset() - currentOffset);
		}

		char innerType[256];

		if (Eq(e.TypeName(), "TArray"))
		{
			InnerUnrealTypeToMarshalledType(innerType, sizeof innerType, e.InnerValueType());
			sb.AppendFormat("\t\tR_TArray<%s> ", innerType);
		}
		else if (Eq(e.TypeName(), "TSet"))
		{
			InnerUnrealTypeToMarshalledType(innerType, sizeof innerType, e.InnerValueType());
			sb.AppendFormat("\t\tR_TSet<%s> ", innerType);
		}
		else if (Eq(e.TypeName(), "TMap"))
		{
			InnerUnrealTypeToMarshalledType(innerType, sizeof innerType, e.InnerValueType());

			char keyType[256];
			InnerUnrealTypeToMarshalledType(keyType, sizeof keyType, e.InnerKeyType());
			sb.AppendFormat("\t\tR_TMap<%s,%s> ", keyType, innerType);
		}
		else if (TryGetInnerType(innerType, sizeof innerType, e.TypeName(), enumAsBytePrefix))
		{
			auto* enumRef = FindEnum(innerType);
			sb.AppendFormat("\t\tEnum::R_TEnumAsByte<Enum::R_%s> ", enumRef ? enumRef->Name() : innerType);
		}
		else if (TryGetInnerType(innerType, sizeof innerType, e.TypeName(), subclassOfPrefix))
		{
			sb.AppendFormat("\t\tR_TSubclassOf<%s> ", innerType);
		}
		else if (TryGetInnerType(innerType, sizeof innerType, e.TypeName(), objectPtrPrefix))
		{
			sb.AppendFormat("\t\tR_TObjectPtr<%s> ", innerType);
		}
		else if (TryGetInnerType(innerType, sizeof innerType, e.TypeName(), softObjectPtrPrefix))
		{
			sb.AppendFormat("\t\tR_TSoftObjectPtr<%s> ", innerType);
		}
		else if (TryGetInnerType(innerType, sizeof innerType, e.TypeName(), delegatePrefix))
		{
			sb.AppendFormat("\t\tR_TDelegate<Delegate::R_%s> ", innerType);
		}
		else
		{
			auto h = hardcodedTypes.find(e.TypeName());
			if (h != hardcodedTypes.end())
			{
				sb.AppendFormat("\t\t%s ", h->second.typeName);
			}
			else
			{
				auto* enumRef = FindEnum(e.TypeName());
				if (enumRef)
				{
					cstr enumInnerType;
					switch (e.SizeOf())
					{
					case 1:						
						enumInnerType = "int8";
						break;
					case 2:
						enumInnerType = "int16";
						break;
					case 4:
						enumInnerType = "int32";
						break;
					default:
						Throw(0, "Unknown enum size: %d", e.SizeOf());
					}
					sb.AppendFormat("\t\tR_TEnum<Enum::R_%s,%s> ", e.TypeName(), enumInnerType);
				}
				else if (*e.TypeName() == 'F' && EndsWith(e.TypeName(), "Delegate"))
				{
					sb.AppendFormat("\t\tR_TDelegate<R_%s> ", e.TypeName());
				}
				else
				{
					sb.AppendFormat("\t\t%s%s ", STRUCT_PREFIX, e.TypeName() + 1);
				}
			}
		}

		AppendIdentifier(sb, e.FieldName());

		if (e.IsBitfield())
		{
			if (i > 0)
			{
				auto& predecessor = structDef[i - 1];
				if (predecessor.IsBitfield() && predecessor.Offset() == e.Offset())
				{
					sb << " : 1";
					goto next;
				}
			}

			if (i < structDef.ElementCount() - 1)
			{
				auto& successor = structDef[i + 1];
				if (successor.IsBitfield() && successor.Offset() == e.Offset())
				{
					sb << " : 1";
				}			
			}
		}

		next:

		sb << ";\n";
		currentOffset = e.Offset() + e.SizeOf();
	}

	if (currentOffset < structDef.SizeOf())
	{
		sb.AppendFormat("\t\tchar _padding_%d[%d];\n", paddingIndex++, structDef.SizeOf() - currentOffset);
	}

	sb << "\t};\n";

	sb.AppendFormat("\tstatic_assert(sizeof(%s%s) == %d);\n", STRUCT_PREFIX, structDef.TypeName(), structDef.SizeOf());

	sb << "}\n";
}

void BuildSexyNativesHPP(IUnrealClass& classDef, StringBuilder& sb)
{
	sb << 

R"(#pragma once
// Code generated by unreal-sexy.gen.core.cpp
#include <rococo.types.h>

class UClass;

namespace Rococo::Script
{
	DECLARE_ROCOCO_INTERFACE IPublicScriptSystem;
}

namespace Rococo::UE::Native
{
)";

	sb.AppendFormat("\tvoid AddSexyNatives_Unreal_%s(Rococo::Script::IPublicScriptSystem& ss, UClass* classRef);\n", classDef.ShortName());
	sb << "}\n";
}

void BuildSexyFiles(IUnrealClass& classRef, StringBuilder& sb)
{
	sb << "(using UE.Native.";
	AppendPackageAsSexyNamespace(sb, classRef);
	sb << ")\n\n";

	sb << "(class ";
	AppendNameAsSxyType(sb, classRef);
	sb << " (defines UE.";
	AppendPackageAsSexyNamespace(sb, classRef);
	sb << ".I";
	AppendNameAsSxyType(sb, classRef);
	sb << ")\n";
	sb << "\t(int64 objectHandle)\n";
	sb << ")\n\n";

	sb << "(factory UE.";

	AppendPackageAsSexyNamespace(sb, classRef);

	sb << ".New";

	AppendNameAsSxyType(sb, classRef);

	sb << " UE.";

	AppendPackageAsSexyNamespace(sb, classRef);

	sb << ".I";
	AppendNameAsSxyType(sb, classRef);

	sb << " ()\n";
	sb << "\t(construct ";
	AppendNameAsSxyType(sb, classRef);
	sb << ")\n";
	sb << ")\n";

	std::vector<IUnrealArg*> inputs;
	std::vector<IUnrealArg*> outputs;

	sb << "\n(method ";
	AppendNameAsSxyType(sb, classRef);
	sb << ".Construct :\n";
	sb << "\t(Construct";
	AppendNameAsSxyType(sb, classRef);
	sb << " -> this.ObjectHandle)\n";
	sb << ")\n";

	for (size_t i = 0; i < classRef.MethodCount(); i++)
	{
		auto& method = classRef.GetFunction(i);

		sb << "\n(method ";
		AppendNameAsSxyType(sb, classRef);

		sb << ".";

		method.AppendFunctionName(sb);

		BuildSexyInputsAndOutputs(REF inputs, REF outputs, method);

		if (!inputs.empty())
		{
			sb << " ";
		}

		for (auto* input : inputs)
		{
			sb << "(";
			AppendType(sb, *input, true);
			sb << " ";
			input->AppendName(sb, true);
			sb << ")";
		}

		sb << " -> ";

		for (auto* output : outputs)
		{
			sb << "(";
			AppendType(sb, *output, true);
			sb << " ";
			output->AppendName(sb, true);
			sb << ")";
		}

		sb << ":\n";

		sb << "\t(";

		method.AppendFunctionName(sb);

		sb << " this.objectHandle";

		if (!inputs.empty())
		{
			sb << " ";
		}

		for (auto* input : inputs)
		{
			input->AppendName(sb, true);
		}

		if (!outputs.empty())
		{
			sb << " -> ";

			for (auto* output : outputs)
			{
				output->AppendName(sb, true);
			}
		}

		sb << ")\n";

		sb << ")\n";
	}
}

void GenStructDef(IUnrealStruct& structDef, crwstr nativeDirectory)
{
	cstr structName = structDef.TypeName();
	cstr packageName = UsePackageForFolders ? structDef.Package() : "";

	if (*packageName == '/')
	{
		packageName++;
	}

	//AutoFree<IDynamicStringBuilder> dsbCPP = CreateDynamicStringBuilder(64_kilobytes);

	// .cpp builder
	//auto& sbCPP = dsbCPP->Builder();

	AutoFree<IDynamicStringBuilder> dsbHPP = CreateDynamicStringBuilder(16_kilobytes);

	// .HPP builder
	auto& sbHPP = dsbHPP->Builder();

	AutoFree<IDynamicStringBuilder> dsbSXY = CreateDynamicStringBuilder(64_kilobytes);

	// .sxy builder
	//auto& sbSXY = dsbSXY->Builder();

	//WideFilePath wTargetCPPFile;
	//Format(wTargetCPPFile, L"%ls%hs\\%hs.cpp", nativeDirectory, packageName, shortName);
	//IO::ToSysPath(wTargetCPPFile.buf);

	WideFilePath wTargetHPPFile;
	Format(wTargetHPPFile, L"%lsStruct/%hs/%hs.hpp", nativeDirectory, packageName, structName);
	IO::ToSysPath(wTargetHPPFile.buf);

	//WideFilePath wTargetSXYFile;
	//Format(wTargetSXYFile, L"%ls%hs\\%hs.sxy", sexyDirectory, packageName, shortName);
	//IO::ToSysPath(wTargetSXYFile.buf);

	//BuildSexyNativesCPP(classDef, sbCPP);
	BuildSexyNativeStructsHPP(structDef, sbHPP);
	//BuildSexyFiles(classDef, sbSXY);

	//IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetCPPFile, *sbCPP);
	IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
	//IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetSXYFile, *sbSXY);
}

void GenDelegateDef(cstr rawTypeName, int sizeInBytes, crwstr path)
{
	cstr typeName = rawTypeName;

	char mutedTypeName[256];

	if (EndsWith(rawTypeName, "^"))
	{
		CopyString(mutedTypeName, sizeof mutedTypeName, rawTypeName);
		mutedTypeName[strlen(mutedTypeName) - 1] = 0;
		typeName = mutedTypeName;
	}

	AutoFree<IDynamicStringBuilder> dsbHPP = CreateDynamicStringBuilder(16_kilobytes);
	auto& sbHPP = dsbHPP->Builder();

	WideFilePath wTargetHPPFile;
	Format(wTargetHPPFile, L"%lsDelegate/%hs.hpp", path, typeName);
	IO::ToSysPath(wTargetHPPFile.buf);

	sbHPP << R"(#pragma once

namespace Rococo::UE::Native::Delegate
{
)"

<< "\tclass R_" << typeName << "\n"
<< "\t{\n"
<< "\t\t";

	sbHPP.AppendFormat("char _opaque_data[%d];\n", sizeInBytes);

	sbHPP << "\t};\n}\n";

	IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
}

void BuildSexyNativeEnumHPP(IUnrealEnumDef& structDef, StringBuilder& sb);

void GenEnumDef(IUnrealEnumDef& enumDef, crwstr nativeDirectory)
{
	cstr structName = enumDef.Name();
	cstr packageName = UsePackageForFolders ? enumDef.Package() : "";

	if (*packageName == '/')
	{
		packageName++;
	}

	//AutoFree<IDynamicStringBuilder> dsbCPP = CreateDynamicStringBuilder(64_kilobytes);

	// .cpp builder
	//auto& sbCPP = dsbCPP->Builder();

	AutoFree<IDynamicStringBuilder> dsbHPP = CreateDynamicStringBuilder(16_kilobytes);

	// .HPP builder
	auto& sbHPP = dsbHPP->Builder();

	AutoFree<IDynamicStringBuilder> dsbSXY = CreateDynamicStringBuilder(64_kilobytes);

	// .sxy builder
	//auto& sbSXY = dsbSXY->Builder();

	//WideFilePath wTargetCPPFile;
	//Format(wTargetCPPFile, L"%ls%hs\\%hs.cpp", nativeDirectory, packageName, shortName);
	//IO::ToSysPath(wTargetCPPFile.buf);

	WideFilePath wTargetHPPFile;
	Format(wTargetHPPFile, L"%lsEnum\\%hs\\%hs.hpp", nativeDirectory, packageName, structName);
	IO::ToSysPath(wTargetHPPFile.buf);

	//WideFilePath wTargetSXYFile;
	//Format(wTargetSXYFile, L"%ls%hs\\%hs.sxy", sexyDirectory, packageName, shortName);
	//IO::ToSysPath(wTargetSXYFile.buf);

	//BuildSexyNativesCPP(classDef, sbCPP);
	BuildSexyNativeEnumHPP(enumDef, sbHPP);
	//BuildSexyFiles(classDef, sbSXY);

	//IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetCPPFile, *sbCPP);
	IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
	//IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetSXYFile, *sbSXY);
}

void BuildSexyNativeEnumHPP(IUnrealEnumDef& enumDef, StringBuilder& sb)
{
	sb <<

R"(#pragma once
// Code generated by unreal-sexy.gen.core.cpp
#include <rococo.types.h>

namespace Rococo::UE::Native::Enum
{
)";

	sb << "\tenum class R_";
	sb << enumDef.Name();
	sb << " : int32";	
	sb << "\n";
	sb << "\t{\n";

	for (int32 i = 0; i < enumDef.NumberOfKeys(); i++)
	{
		cstr key = enumDef.GetKey(i);
		int64 value = enumDef.GetValue(i);

		sb.AppendFormat("\t\t%s = %lld", key, value);

		if (i != enumDef.NumberOfKeys() - 1)
		{
			sb << ",";
		}

		sb << "\n";
	}

	sb << "\t};\n";

	sb << "}\n";
}