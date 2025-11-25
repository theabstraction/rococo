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

void BuildInputsAndOutputs(std::vector<IUnrealArg*>& inputs, std::vector<IUnrealArg*>& outputs, IUnrealFunction& method)
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

		if (arg->IsOutput())
		{
			outputs.push_back(arg);
		}
		else
		{
			inputs.push_back(arg);
		}
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

		arg->AppendType(sb);

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
	BuildInputsAndOutputs(REF inputs, REF outputs, method);

	sb << "\t\tint64 objectHandle;\n";
	sb << "\t\toffset += sizeof(int64);\n";
	sb << "\t\tReadInput(objectHandle, sf, -offset);\n\n";
	
	for (auto* input : inputs)
	{
		sb << "\t\t";

		input->AppendType(sb);
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

		output->AppendType(sb);

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
#include <rococo.types.h>
#include <sexy.script.h>

typedef int UnknownType;
class UClass;
class UObject;
class UMethod;
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
namespace Rococo::UE5::Marshal
{
	int64 ConstructUObject(Rococo::Script::NativeCallEnvironment& e);
	UMethod* GetNCEUMethod(Rococo::Script::NativeCallEnvironment& e);
	UMethod& GetMethod(UClass& classRef, crwstr methodName);
	UObject* GetNCEUObject(Rococo::Script::NativeCallEnvironment& e, int64 objectHandle);
	void ValidateArgs(UMethod* methodRef, void* args, size_t argSize);
	void ProcessEvent(UObject* object, UMethod* methodRef, void* args);
}
)";

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

		sb << "\t\tss.AddNativeCall(ns, ";
		AppendCompactName(sb, classDef.ShortName());
		sb << "_";
		method.AppendFunctionName(sb);
		sb << ", &GetMethod(*classRef, L\"";

		method.AppendFunctionName(sb);

		sb << "\"), \"";
		method.AppendFunctionName(sb);

		BuildInputsAndOutputs(REF inputs, REF outputs, method);

		sb << " (int64 objectHandle)";

		if (!inputs.empty())
		{
			sb << " ";
		}

		for (auto* input : inputs)
		{
			sb << "(";
			input->AppendType(sb);
			sb << " ";
			input->AppendName(sb, true);
			sb << ")";
		}

		sb << " -> ";

		for (auto* output : outputs)
		{
			sb << "(";
			output->AppendType(sb);
			sb << " ";
			output->AppendName(sb, true);
			sb << ")";
		}

		sb << "\", __FILE__, __LINE__);\n";
	}

	sb << "\t}\n";
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

		BuildInputsAndOutputs(REF inputs, REF outputs, method);

		if (!inputs.empty())
		{
			sb << " ";
		}

		for (auto* input : inputs)
		{
			sb << "(";
			input->AppendType(sb);
			sb << " ";
			input->AppendName(sb, true);
			sb << ")";
		}

		sb << " -> ";

		for (auto* output : outputs)
		{
			sb << "(";
			output->AppendType(sb);
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