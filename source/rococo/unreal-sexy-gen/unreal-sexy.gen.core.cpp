#include "unreal-sexy-gen.h"
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.hashtable.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Unreal;

void BuildSexyNativesCPP(IUnrealClass& classDef, StringBuilder& sb);
void BuildSexyNativesHPP(IUnrealClass& classDef, StringBuilder& sb);
void BuildSexyFiles(IUnrealClass& classDef, StringBuilder& sb);
IUnrealStruct* FindStruct(cstr name);
IUnrealEnumDef* FindEnum(cstr name);

const bool UsePackageForFolders = false;

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

	IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetCPPFile, *sbCPP);
	IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
	IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetSXYFile, *sbSXY);
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

void AppendType(StringBuilder& sb, cstr argType, bool makeSexyVariableType)
{
	cstr p = argType;

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
	AppendTypeSansRef(sb, argType);
	sb << "*";
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

		cstr argType = arg->ArgType();	
		AppendType(sb, argType, false);

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

		cstr argType = input->ArgType();
		AppendType(sb, argType, false);

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

		cstr argType = output->ArgType();
		AppendType(sb, argType, false);

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

void BuildSexyNativesCPP(IUnrealClass& classDef, StringBuilder& sb)
{
	sb <<

		R"(// Code generated by unreal-sexy.gen.core.cpp
#include "../unreal-sexy-marshalling.h"
)";


	stringmap<int> knownObjects;
	knownObjects.insert("UClass", 0);
	knownObjects.insert("UObject", 0);
	knownObjects.insert("UMethod", 0);
	knownObjects.insert("UnknownType", 0);

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

			char objectPointerType[128];
			if (arg->GetObjectPointerType(objectPointerType, sizeof objectPointerType))
			{
				// Zap the trailing *
				objectPointerType[strlen(objectPointerType) - 1] = 0;

				if (knownObjects.find(objectPointerType) == knownObjects.end())
				{
					// New object
					knownObjects.insert(objectPointerType, 0);

					sb << "class " << objectPointerType << ";\n";
				}
			}

			
		}
	}

	sb << R"(
using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
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

	sb << "namespace Rococo::Unreal\n{\n";
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
			AppendType(sb, input->ArgType(), true);
			sb << " ";
			input->AppendName(sb, true);
			sb << ")";
		}

		sb << " -> ";

		for (auto* output : outputs)
		{
			sb << "(";
			AppendType(sb, output->ArgType(), true);
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

cstr STRUCT_PREFIX = "RF_";

void BuildHardCodedTypes()
{
	if (!hardcodedTypes.empty())
	{
		return;
	}

	hardcodedTypes.insert("uint8", { "uint8", nullptr });
	hardcodedTypes.insert("float", { "float", nullptr });
	hardcodedTypes.insert("bool",  { "bool", nullptr });
	hardcodedTypes.insert("int32", { "int32", nullptr });
	hardcodedTypes.insert("FName", { "RF_Name", nullptr });
	hardcodedTypes.insert("FVector", { "RF_VectorD", nullptr });
	hardcodedTypes.insert("FGuid", { "RF_Guid", nullptr });
}

auto objectPtrPrefix = "TObjectPtr<"_fstring;
auto subclassOfPrefix = "TSubclassOf<"_fstring;
auto enumAsBytePrefix = "TEnumAsByte<"_fstring;

void InnerUnrealTypeToMarshalledType(char* buffer, size_t capacity, cstr unrealType)
{
	if (Eq(unrealType, "TArray"))
	{
		Throw(0, "Cannot handle TArray as an inner type");
	}
	else if (StartsWith(unrealType, enumAsBytePrefix))
	{
		char innerType[256];
		cstr endToken = FindChar(unrealType, '>');
		cstr startToken = unrealType + enumAsBytePrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		SafeFormat(buffer, capacity, "uint8");
	}
	else if (StartsWith(unrealType, subclassOfPrefix))
	{
		char innerType[256];
		cstr endToken = FindChar(unrealType, '>');
		cstr startToken = unrealType + subclassOfPrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		SafeFormat(buffer, capacity, "RF_SubclassOf");
	}
	else if (StartsWith(unrealType, objectPtrPrefix))
	{
		char innerType[256];
		cstr endToken = FindChar(unrealType, '>');
		cstr startToken = unrealType + objectPtrPrefix.length;
		CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
		SafeFormat(buffer, capacity, "tRF_HObject");
	}
	else
	{
		auto h = hardcodedTypes.find(unrealType);
		if (h != hardcodedTypes.end())
		{
			SafeFormat(buffer, capacity, "%s", h->second.typeName);
		}
		else
		{
			SafeFormat(buffer, capacity, "%s%s", STRUCT_PREFIX, unrealType);
		}
	}
}

void BuildSexyNativeStructsHPP(IUnrealStruct& structDef, StringBuilder& sb)
{
	BuildHardCodedTypes();

	size_t nElements = structDef.ElementCount();

	sb <<
		R"(#pragma once
// Code generated by unreal-sexy.gen.core.cpp
#include <rococo.types.h>
)";

	dependencies.clear();

	for (size_t i = 0; i < nElements; i++)
	{
		auto& e = structDef[i];

		if (Eq(e.TypeName(), "Array"))
		{
			continue;
		}

		if (StartsWith(e.TypeName(), "TObjectPtr<"))
		{
			continue;
		}

		if (StartsWith(e.TypeName(), "TSubclassOf<"))
		{
			continue;
		}

		if (StartsWith(e.TypeName(), "TEnumAsByte<"))
		{
			continue;
		}

		auto* enumDef = FindEnum(e.TypeName());
		if (enumDef)
		{
			sb.AppendFormat("#include <%s.hpp>\n", enumDef->Name());
			continue;
		}

		if (StartsWith(e.TypeName(), "E"))
		{
			Throw(0, "Expecting Enumeration name %s, but it was not a known enumeration type", e.TypeName());
		}

		auto h = hardcodedTypes.find(e.TypeName());
		if (h != hardcodedTypes.end())
		{
			if (h->second.header != nullptr)
			{
				sb.AppendFormat("#include <%s.hpp>\n", h->second.header);
			}
		}
		else
		{
			auto d = dependencies.insert(e.TypeName(), 0);
			if (d.second)
			{
				sb.AppendFormat("#include <%s.hpp>\n", e.TypeName());
			}
		}
	}

	sb << R"(
namespace Rococo::UE::Structs::Gen
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

		if (Eq(e.TypeName(), "TArray"))
		{
			char innerType[256];
			InnerUnrealTypeToMarshalledType(innerType, sizeof innerType, e.InnerValueType());
			sb.AppendFormat("\t\tRT_Array<%s> ", innerType);
		}
		else if (StartsWith(e.TypeName(), enumAsBytePrefix))
		{
			char innerType[256];
			cstr endToken = FindChar(e.TypeName(), '>');
			cstr startToken = e.TypeName() + enumAsBytePrefix.length;
			CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
			sb.AppendFormat("\t\tuint8 /* %s */ ", innerType);
		}
		else if (StartsWith(e.TypeName(), subclassOfPrefix))
		{
			char innerType[256];
			cstr endToken = FindChar(e.TypeName(), '>');
			cstr startToken = e.TypeName() + subclassOfPrefix.length;
			CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
			sb.AppendFormat("\t\tRF_SubclassOf /* %s */ ", innerType);
		}
		else if (StartsWith(e.TypeName(), objectPtrPrefix))
		{
			char innerType[256];
			cstr endToken = FindChar(e.TypeName(), '>');
			cstr startToken = e.TypeName() + objectPtrPrefix.length;
			CopyString(innerType, sizeof innerType, startToken, endToken - startToken);
			sb.AppendFormat("\t\tRF_HObject /* %s */ ", innerType);
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
				sb.AppendFormat("\t\t%s%s ", STRUCT_PREFIX, e.TypeName());
			}
		}

		AppendIdentifier(sb, e.FieldName());
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

namespace Rococo::Unreal
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
			AppendType(sb, input->ArgType(), true);
			sb << " ";
			input->AppendName(sb, true);
			sb << ")";
		}

		sb << " -> ";

		for (auto* output : outputs)
		{
			sb << "(";
			AppendType(sb, output->ArgType(), true);
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
	Format(wTargetHPPFile, L"%lsStructs\\%hs\\%hs.hpp", nativeDirectory, packageName, structName);
	IO::ToSysPath(wTargetHPPFile.buf);

	//WideFilePath wTargetSXYFile;
	//Format(wTargetSXYFile, L"%ls%hs\\%hs.sxy", sexyDirectory, packageName, shortName);
	//IO::ToSysPath(wTargetSXYFile.buf);

	//BuildSexyNativesCPP(classDef, sbCPP);
	BuildSexyNativeStructsHPP(structDef, sbHPP);
	//BuildSexyFiles(classDef, sbSXY);

	//IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetCPPFile, *sbCPP);
	IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
	//IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetSXYFile, *sbSXY);
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
	Format(wTargetHPPFile, L"%lsStructs\\%hs\\%hs.hpp", nativeDirectory, packageName, structName);
	IO::ToSysPath(wTargetHPPFile.buf);

	//WideFilePath wTargetSXYFile;
	//Format(wTargetSXYFile, L"%ls%hs\\%hs.sxy", sexyDirectory, packageName, shortName);
	//IO::ToSysPath(wTargetSXYFile.buf);

	//BuildSexyNativesCPP(classDef, sbCPP);
	BuildSexyNativeEnumHPP(enumDef, sbHPP);
	//BuildSexyFiles(classDef, sbSXY);

	//IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetCPPFile, *sbCPP);
	IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
	//IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetSXYFile, *sbSXY);
}

void BuildSexyNativeEnumHPP(IUnrealEnumDef& enumDef, StringBuilder& sb)
{
	sb <<

R"(#pragma once
// Code generated by unreal-sexy.gen.core.cpp
#include <rococo.types.h>

namespace Rococo::Unreal
{
)";

	sb << "\tenum class ";
	sb << enumDef.Name();
	sb << ": Rococo::";

	switch (enumDef.GetUnderlyingSize())
	{
	case 1:
		sb << "uint8";
		break;
	case 2:
		sb << "uint16";
		break;
	case 4:
		sb << "uint32";
		break;
	case 8:
		sb << "uint64";
		break;
	}

	sb << "\n";
	sb << "\t{\n";

	for (int32 i = 0; i < enumDef.NumberOfKeys(); i++)
	{
		cstr key = enumDef.GetKey(i);
		int64 value = enumDef.GetValue(i);

		sb.AppendFormat("\t\t%s = %lld\n", key, value);
	}

	sb << "\t};\n";

	sb << "}\n";
}