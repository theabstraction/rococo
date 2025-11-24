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

const bool UsePackageForFolders = false;

void GenClassDef(IUnrealClass& classDef, crwstr rootDirectory)
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

	WideFilePath wTargetCPPFile;
	Format(wTargetCPPFile, L"%ls%hs\\%hs.cpp", rootDirectory, packageName, shortName);
	IO::ToSysPath(wTargetCPPFile.buf);

	WideFilePath wTargetHPPFile;
	Format(wTargetHPPFile, L"%ls%hs\\%hs.hpp", rootDirectory, packageName, shortName);
	IO::ToSysPath(wTargetHPPFile.buf);

	BuildSexyNativesCPP(classDef, sbCPP);
	BuildSexyNativesHPP(classDef, sbHPP);

	IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetCPPFile, *sbCPP);
	IO::SaveAsciiTextFile(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
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
	sb << "(NativeCallEnvironment & _nce)\n";
	sb << "\t{\n";

	sb << "\t\tuint8* _sf = _nce.cpu.SF();\n";
	sb << "\t\tptrdiff_t _offset = 2 * sizeof(size_t);\n";

	sb << "\t\tUMethod* methodRef = GetNCEUMethod(_nce);\n";
	sb << "\t\tUObject* object = GetNCEUObject(_nce);\n\n";
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

		if (arg->IsRef())
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

	for (auto* input : inputs)
	{
		sb << "\t\t";

		input->AppendType(sb);
		sb << " ";
		input->AppendName(sb, true);
		sb << ";\n";

		sb << "\t\t_offset += sizeof(";
		input->AppendName(sb, true);
		sb << ");\n";

		sb << "\t\tReadInput(";
		input->AppendName(sb, true);
		sb << ", _sf, -_offset);\n";

		sb << "\t\targs.m_";
		input->AppendName(sb, false);
		sb << " = ";

		if (input->IsRef())
		{
			sb << "&";
		}

		input->AppendName(sb, true);
		sb << ";\n\n";
	}

	sb << "\t\tValidateArgs(methodRef, &args, sizeof(args));\n";
	sb << "\t\tProcessEvent(object, methodRef, &args);\n";

	for (auto* output : outputs)
	{
		sb << "\n\t\t";

		output->AppendType(sb);
		sb << " ";
		output->AppendName(sb, true);
		sb << " = args.m_";
		output->AppendName(sb, false);
		sb << ";\n";

		sb << "\t\t_offset += sizeof(";
		output->AppendName(sb, true);
		sb << ");\n";

		sb << "\t\tWriteOutput(";
		output->AppendName(sb, true);
		sb << ", _sf, -_offset);\n";
	}

	sb << "\t}\n";
}

#include <ctype.h>

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
		Throw(0, "Cannot transform package name to Sexy namespace. Bad first character: %s %s", classRef.PackageName(), classRef.PackageName());
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
			sb.AppendChar(c);
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
	UMethod* GetNCEUMethod(Rococo::Script::NativeCallEnvironment& e);
	UObject* GetNCEUObject(Rococo::Script::NativeCallEnvironment& e);
	void ValidateArgs(UMethod* methodRef, void* args, size_t argSize);
	void ProcessEvent(UObject* object, UMethod* methodRef, void* args);
}
)";

	if (classDef.MethodCount() > 0)
	{
		sb << R"(
namespace 
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;
	using namespace Rococo::UE5::Marshal;
)";

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
	
		sb << "UE.";

		AppendPackageAsSexyNamespace(sb, classDef);
			
		sb << "\");\n";

		std::vector<IUnrealArg*> inputs;
		std::vector<IUnrealArg*> outputs;

		for (size_t i = 0; i < classDef.MethodCount(); i++)
		{
			auto& method = classDef.GetFunction(i);

			sb << "\t\tss.AddNativeCall(ns, ";
			AppendCompactName(sb, classDef.ShortName());
			sb << "_";
			method.AppendFunctionName(sb);
			sb << ", classRef, \"";
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

			sb << "\", __FILE__, __LINE__);\n";
		}

		sb << "\t}\n";
		sb << "}\n";
	}
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