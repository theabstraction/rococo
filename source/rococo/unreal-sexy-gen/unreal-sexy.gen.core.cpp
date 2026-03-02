#include "unreal-sexy-gen.h"
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.hashtable.h>
#include <algorithm>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Unreal;

extern fstring enumAsBytePrefix;
extern fstring softObjectPtrPrefix;
extern fstring subclassOfPrefix;
extern fstring scriptInterfacePrefix;
extern fstring objectPtrPrefix;

extern bool g_unityBuild;

fstring delegatePrefix = "TDelegate<"_fstring;

void BuildSexyNativesCPP(IUnrealClass& classDef, StringBuilder& sb, IEnums& enums, IStructs& structs, IDelegates& delegates);
void BuildSexyNativesHPP(IUnrealClass& classDef, StringBuilder& sb);
void BuildSexyFiles(IUnrealClass& classDef, StringBuilder& sb, IEnums& enums, IStructs& structs, IDelegates& delegates);

const bool UsePackageForFolders = false;

namespace Rococo::Unreal
{
	HString FormatCPPNamespaceFromPath(cstr path);
}

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

std::vector<HString> allCppFileNames;

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

void AppendContractedName(StringBuilder& sb, cstr p)
{
	while (*p != 0)
	{
		char c = *p++;
		switch (c)
		{
		case '-':
		case ' ':
		case '_':
			break;
		default:
			sb.AppendChar(c);
			break;
		}
	}
}

void BuildCPPInputsAndOutputs(std::vector<const IUnrealArg*>& inputs, std::vector<const IUnrealArg*>& outputs, IUnrealFunction& method)
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

void BuildSexyInputsAndOutputs(std::vector<const IUnrealArg*>& inputs, std::vector<const IUnrealArg*>& outputs, const IUnrealFunction& method)
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

void MarshalNameTypeAsHandle(StringBuilder& sb, cstr nameType, bool makeSexyVariableType, bool isForElement = false)
{
	if (makeSexyVariableType && !isForElement)
	{
		sb << "UE.Handles.H";
	}
	
	if (makeSexyVariableType)
	{
		AppendContractedName(sb, nameType);
	}
	else
	{
		sb << nameType;
	}

	if (makeSexyVariableType)
	{
		sb.Undo(-1);
	}
}

void AppendTypeSansRef(StringBuilder& sb, cstr argType)
{
	cstr p = argType;
	while (*p != 0 && *p != '^')
	{
		sb.AppendChar(*p++);
	}
}

bool TryGetEnumAsByte(char* innerType, size_t capacity, cstr argType, IEnums& enums)
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
		auto* enumType = enums.FindEnum(innerType);
		if (enumType)
		{
			return true;
		}
	}

	return false;
}

void AppendNonContainerType_SXY_Private(StringBuilder& sb, cstr argType, IEnums& enums, IStructs& structs, IDelegates& delegates, bool isForElement = false)
{
	if (EndsWith(argType, "^"))
	{
		char valueType[256];
		CopyString(valueType, sizeof valueType, argType, strlen(argType) - 1);
		AppendNonContainerType_SXY_Private(sb, valueType, enums, structs, delegates);
		return;
	}

	if (*argType == 'U' && EndsWith(argType, "*"))
	{
		MarshalNameTypeAsHandle(sb, argType, true, isForElement);
		return;
	}

	if (*argType == 'A' && EndsWith(argType, "*"))
	{
		MarshalNameTypeAsHandle(sb, argType, true, isForElement);
		return;
	}

	auto* primitive = structs.FindPrimitiveType(argType);
	if (primitive)
	{
		sb << primitive->SXYName();
		return;
	}

	auto* enumType = enums.FindEnum(argType);
	if (enumType)
	{
		if (!EndsWith(*sb, "Of"))  // Hack, ArrayOfX does not use the namespace for X
		{
			sb << "UE.Enums.";
		}
		sb << enumType->Name();
		return;
	}

	size_t delegateSize = delegates.FindDelegateSize(argType);
	if (delegateSize)
	{
		sb << argType;
		return;
	}

	char innerType[256];
	if (TryGetEnumAsByte(innerType, sizeof innerType, argType, enums))
	{
		auto* enumRef = enums.FindEnum(innerType);
		if (!enumRef)
		{
			Throw(0, "Could not find inner type %s", innerType);
		}
		if (!EndsWith(*sb, "Of")) // Hack, ArrayOfX does not use the namespace for X
		{
			sb << "UE.Enums.";
		}
		sb << enumRef->Name();
		return;
	}

	if (*argType == 'F')
	{
		auto* structType = structs.FindStruct(argType + 1);
		if (structType)
		{
			sb << argType;
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
		// The handle does not retain the innter type, as subclass template TSubclassOf, wraps a UClass pointer.
		// This leaves the script language agnostic towards the class type
		sb << "HUClass";
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
		sb << "HUObject";
		return;
	}

	structs.MarkUnknown(argType);

	sb << "UnknownType /*";
	AppendTypeSansRef(sb, argType);
	sb << "*/";
}

void AppendNonContainerType_CPP_Private(StringBuilder& sb, fstring argType, IEnums& enums, IStructs& structs, IDelegates& delegates)
{
	if (EndsWith(argType, "^"))
	{
		char valueType[256];
		CopyString(valueType, sizeof valueType, argType, strlen(argType) - 1);
		AppendNonContainerType_CPP_Private(sb, to_fstring(valueType), enums, structs, delegates);
		return;
	}

	if (*argType == 'U' && EndsWith(argType, "*"))
	{
		MarshalNameTypeAsHandle(sb, argType, false);
		return;
	}

	if (*argType == 'A' && EndsWith(argType, "*"))
	{
		MarshalNameTypeAsHandle(sb, argType, false);
		return;
	}

	auto* primitive = structs.FindPrimitiveType(argType);
	if (primitive)
	{
		sb << primitive->CPPName();
		return;
	}

	auto* enumType = enums.FindEnum(argType);
	if (enumType)
	{
		sb << "R_" << enumType->Name();
		return;
	}

	size_t delegateSize = delegates.FindDelegateSize(argType);
	if (delegateSize)
	{
		sb << argType;
		return;
	}

	char innerType[256];
	if (TryGetEnumAsByte(innerType, sizeof innerType, argType, enums))
	{
		auto* enumRef = enums.FindEnum(innerType);
		if (!enumRef)
		{
			Throw(0, "Could not find inner type %s", innerType);
		}
		sb.AppendFormat("R_TEnumAsByte<R_%s>", enumRef->Name());
		return;
	}

	if (*argType == 'F')
	{
		auto* structType = structs.FindStruct(argType + 1);
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

	structs.MarkUnknown(argType);

	sb << "UnknownType /*";
	AppendTypeSansRef(sb, argType);
	sb << "*/";
}

void AppendNonContainerType_Private(StringBuilder& sb, fstring argType, bool makeSexyVariableType, IEnums& enums, IStructs& structs, IDelegates& delegates, bool isForElement = false)
{
	if (makeSexyVariableType)
	{
		AppendNonContainerType_SXY_Private(sb, argType, enums, structs, delegates, isForElement);
	}
	else
	{
		AppendNonContainerType_CPP_Private(sb, argType, enums, structs, delegates);
	}
}

void AppendType(StringBuilder& sb, const IUnrealArg& arg, bool makeSexyVariableType, IEnums& enums, IStructs& structs, IDelegates& delegates)
{
	fstring argType = arg.ArgType();

	if (arg.IsContainer())
	{
		if (makeSexyVariableType)
		{
			sb << "UE.Handles.";
			sb << argType;
			sb << "Of";

			if (*arg.KeyType() != 0)
			{
				AppendNonContainerType_Private(sb, arg.KeyType(), true, enums, structs, delegates, true);
			}

			if (*arg.ElementType() != 0)
			{
				AppendNonContainerType_Private(sb, arg.ElementType(), true, enums, structs, delegates, true);
			}
		}
		else
		{
			sb << "R_";
			sb << (cstr)arg.ArgType();
			sb << "<";
			if (*arg.KeyType() != 0)
			{
				AppendNonContainerType_Private(sb, arg.KeyType(), makeSexyVariableType, enums, structs, delegates);
				sb << ",";
			}

			if (Eq(argType, "TDelegate"))
			{
				sb << "R_";
			}

			AppendNonContainerType_Private(sb, arg.ElementType(), makeSexyVariableType, enums, structs, delegates);
			sb << ">";
		}
	}
	else
	{
		AppendNonContainerType_Private(sb, argType, makeSexyVariableType, enums, structs, delegates);
	}
}


void BuildMethod(IUnrealClass& classDef, IUnrealFunction& method, StringBuilder& sb, IEnums& enums, IStructs& structs, IDelegates& delegates)
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

		if (arg->IsCPPOutput())
		{
			sb << "OUT ";
		}

		AppendType(sb, *arg, false, enums, structs, delegates);

		// Add a prefix to the name, in case the variable name conflicts with a C++ keyword
		sb << " m_";

		arg->AppendName(sb);

		sb << ";\n";
	}

	sb << "\t\t} ";

	sb << "args;\n\n";

	std::vector<const IUnrealArg*> inputs;
	std::vector<const IUnrealArg*> outputs;
	BuildSexyInputsAndOutputs(REF inputs, REF outputs, method);

	for (auto i = outputs.rbegin(); i != outputs.rend(); i++)
	{
		const IUnrealArg* output = *i;
		if (EndsWith(output->ArgType(), "^"))
		{
			if (!output->IsMarshalledByRef() && !output->IsConst())
			{
				// This means we have a primitive type passed by ref, e.g Int32&, which serves as both input and output
				// Since Sexy does not marshal primitives by ref, we break the argument into two, an initial primitive value, and the final value

				sb << "\t\toffset += sizeof(args.m_";
				
				output->AppendName(sb, false);

				sb << ");\n";

				sb << "\t\tReadInput(args.m_";
				output->AppendName(sb, false);
				sb << ", sf, -offset);\n\n";
			}
		}
	}
	
	for (auto i = inputs.rbegin(); i != inputs.rend(); i++)
	{
		const IUnrealArg* input = *i;

		sb << "\t\t";

		AppendType(sb, *input, false, enums, structs, delegates);

		if (input->IsMarshalledByRef())
		{
			sb << "*";
		}

		sb << " in_";
		input->AppendName(sb, true);
		sb << ";\n";

		sb << "\t\toffset += sizeof(in_";
		input->AppendName(sb, true);
		sb << ");\n";

		sb << "\t\tReadInput(in_";
		input->AppendName(sb, true);
		sb << ", sf, -offset);\n";

		if (!input->IsCPPOutput())
		{
			sb << "\t\targs.m_";
			input->AppendName(sb, false);
			sb << " = ";

			if (input->IsMarshalledByRef())
			{
				sb << "*";
			}

			sb << "in_";
			input->AppendName(sb, true);
			sb << ";\n";
		}
		
		sb << "\n";
	}

	sb << "\t\tint64 objectHandle;\n";
	sb << "\t\toffset += sizeof(int64);\n";
	sb << "\t\tReadInput(objectHandle, sf, -offset);\n\n";

	sb << "\t\tUObject* object = GetNCEUObject(nce, objectHandle);\n";
	sb << "\t\tUFunction* methodRef = GetNCEUMethod(nce);\n";
	sb << "\t\tValidateArgs(methodRef, &args, sizeof(args));\n";
	sb << "\t\tProcessEvent(object, methodRef, &args);\n";

	for (auto i = inputs.rbegin(); i != inputs.rend(); i++)
	{
		const IUnrealArg* input = *i;

		if (input->IsCPPOutput())
		{
			if (input->IsReturnValue() || EndsWith(input->ArgType(), "*"))
			{
				sb << "\n\t\t*in_";
				input->AppendName(sb, true);
				sb << " = args.m_";
				input->AppendName(sb, false);
				sb << ";\n";
			}
			else
			{
				sb << "\n\t\tCloneToOutputFromArg(*in_";
				input->AppendName(sb, true);

				sb << ", args.m_";
				input->AppendName(sb, false);

				sb << ");\n";
			}
		}
	}

	for (auto i = outputs.rbegin(); i != outputs.rend(); i++)
	{
		const IUnrealArg* output = *i;
		sb << "\n\t\t";

		AppendType(sb, *output, false, enums, structs, delegates);

		sb << " out_";
		output->AppendName(sb, true);
		sb << " = ";
			
		sb << "args.m_";
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

void AppendNameAsSxyType(StringBuilder& sb, cstr className, cstr packageName)
{
	char curtailedClassName[256];

	// Often we have a class definition in which the package and name look like this: /Fruit/Apple Apple_C
	// We could create a namespace Fruit.Apple with class AppleC, but it looks more user-friendly to have namespace Fruit with class Apple
	// So if the trailing subpspace of the namespace matches the class sans _C, convert to this user-friendly representation
	if (EndsWith(className, "_C"))
	{
		Substring viewClassName = Substring::ToSubstring(className);
		viewClassName.finish -= 2;

		Substring viewPackage = Substring::ToSubstring(packageName);

		cstr finalSlash = ReverseFind('/', viewPackage);
		if (finalSlash)
		{
			Substring packageTrailer{ finalSlash + 1, viewPackage.finish };
			if (Eq(viewClassName, packageTrailer.start))
			{
				viewClassName.CopyWithTruncate(curtailedClassName, sizeof curtailedClassName);
				className = curtailedClassName;
			}
		}
	}

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
		Throw(0, "Cannot transform class name to Sexy type name. Bad first character: %s %s", packageName, className);
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
			Throw(0, "Cannot transform class name to Sexy type name. None alphanumeric character in short name: %s %s", packageName, className);
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

void AppendNameAsSxyType(StringBuilder& sb, const IUnrealClass& classRef)
{
	AppendNameAsSxyType(sb, classRef.ShortName(), classRef.PackageName());
}

void GetClassNameAsSxyType(char* buffer, size_t capacity, const IUnrealClass& classRef)
{
	StackStringBuilder sb(buffer, capacity);
	AppendNameAsSxyType(sb, classRef);
}

void AppendPackageAsSexyNamespace(StringBuilder& sb, IUnrealStruct& structRef)
{
	// Example /Engine/AI/
	cstr packageName = structRef.Package();

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
		Throw(0, "Cannot transform package name to Sexy namespace. Bad first character: %s %s", structRef.Package(), structRef.TypeName());
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

void AppendPackageAsSexyNamespace(StringBuilder& sb, cstr shortName, cstr packageName)
{
	// Example /Engine/AI/

	char curtailedPackageName[256];
	// Often we have a class definition in which the package and name look like this: /Fruit/Apple Apple_C
	// We could create a namespace Fruit.Apple with class AppleC, but it looks more user-friendly to have namespace Fruit with class Apple
	// So if the trailing subpspace of the namespace matches the class sans _C, convert to this user-friendly representation
	if (EndsWith(shortName, "_C"))
	{
		Substring viewClassName = Substring::ToSubstring(shortName);
		viewClassName.finish -= 2;

		Substring viewPackage = Substring::ToSubstring(packageName);

		cstr finalSlash = ReverseFind('/', viewPackage);
		if (finalSlash)
		{
			Substring packageTrailer{ finalSlash + 1, viewPackage.finish };
			if (Eq(viewClassName, packageTrailer.start))
			{
				Substring packageRoot{ packageName, finalSlash };
				packageRoot.CopyWithTruncate(curtailedPackageName, sizeof curtailedPackageName);
				packageName = curtailedPackageName;
			}
		}
	}

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
		Throw(0, "Cannot transform package name to Sexy namespace. Bad first character: %s %s", packageName, shortName);
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


void AppendPackageAsSexyNamespace(StringBuilder& sb, IUnrealClass& classRef)
{
	AppendPackageAsSexyNamespace(sb, classRef.ShortName(), classRef.PackageName());
}

void GetPackageNameAsSxyType(char* buffer, size_t capacity, cstr shortName, cstr packageName)
{
	StackStringBuilder sb(buffer, capacity);
	AppendPackageAsSexyNamespace(sb, shortName, packageName);
}

void GetPackageNameAsSxyType(char* buffer, size_t capacity, IUnrealClass& classRef)
{
	StackStringBuilder sb(buffer, capacity);
	AppendPackageAsSexyNamespace(sb, classRef);
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

			lambda(method, *arg);
		}
	}
}

void GetTypeWithoutRef(char* buffer, size_t capacity, fstring type)
{
	CopyString(buffer, capacity, type);
	if (EndsWith(buffer, "^"))
	{
		buffer[strlen(buffer) - 1] = 0;
	}
}

void AppendHeaders(stringmap<int>& requiredStructs, StringBuilder& sb, const IUnrealArg& arg, cstr argType, IEnums& enums, IStructs& structs)
{
	if (*argType == 'F')
	{
		auto* structType = structs.FindStruct(argType + 1);
		if (structType && structType->IsGenerated())
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
		if (TryGetEnumAsByte(innerType, sizeof innerType, argType, enums))
		{
			if (requiredStructs.insert(innerType, 0).second)
			{
				auto* enumDef = enums.FindEnum(innerType);
				sb.AppendFormat("#include \"Enum/%s.hpp\"\n", enumDef->Name());
				return;
			}
		}
	}

	auto* enumDef = enums.FindEnum(argType);
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

void AppendHeaders(stringmap<int>& requiredStructs, StringBuilder& sb, const IUnrealArg& arg, IEnums& enums, IStructs& structs)
{
	if (arg.IsContainer())
	{
		if (*arg.KeyType())
		{
			AppendHeaders(requiredStructs, sb, arg, arg.KeyType(), enums, structs);
		}
		AppendHeaders(requiredStructs, sb, arg, arg.ElementType(), enums, structs);
	}

	AppendHeaders(requiredStructs, sb, arg, arg.ArgType(), enums, structs);
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

void BuildSexyNativesCPP(IUnrealClass& classDef, StringBuilder& sb, IEnums& enums, IStructs& structs, IDelegates& delegates)
{
	sb <<

		R"(// Code generated by unreal-sexy.gen.core.cpp
#include "../unreal-sexy-marshalling.h"
)";

	stringmap<int> requiredStructs;

	ForEachArgumentOfEachMethod(classDef,
		[&requiredStructs, &sb, &enums, &structs](IUnrealFunction&, const IUnrealArg& arg)
		{
			AppendHeaders(REF requiredStructs, sb, arg, enums, structs);
		}
	);

	sb << "\n";

	stringmap<int> knownObjects;
	knownObjects.insert("UClass", 0);
	knownObjects.insert("UObject", 0);
	knownObjects.insert("UMethod", 0);
	knownObjects.insert("UnknownType", 0);

	stringmap<const IUnrealEnumDef*> knownEnums;

	// Declare classes and build list of knownObjects
	ForEachArgumentOfEachMethod(classDef,
		[&knownEnums,&knownObjects,&sb,&enums,&structs,&delegates](IUnrealFunction& method, const IUnrealArg& arg)
		{
			UNUSED(method);
			char objectPointerType[128];

			auto* e = enums.FindEnum(arg.ArgType());
			if (e)
			{
				knownEnums.insert(e->Name(), e);
				return;
			}

			char innerType[256];
			if (TryGetInnerType(innerType, sizeof innerType, arg.ArgType(), enumAsBytePrefix))
			{
				e = enums.FindEnum(innerType);
				if (e)
				{
					knownEnums.insert(e->Name(), e);
					return;
				}
			}

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

				if (Eq(arg.ArgType(), "TArray") || Eq(arg.ArgType(), "TSet"))
				{
					char containerType[MAX_FQ_NAME_LEN];

					cstr lastDot = Strings::ReverseFind('.', Substring::ToSubstring(objectPointerType));
					SecureFormat(containerType, "%sOf%s", (cstr) arg.ArgType(), lastDot ? lastDot + 1 : objectPointerType);
					knownObjects.insert(containerType, 0);
				}

				return;
			}

			fstring uobjectContainerPrefices[] = { softObjectPtrPrefix, subclassOfPrefix, scriptInterfacePrefix, objectPtrPrefix, ""_fstring };

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
				char containerType[MAX_FQ_NAME_LEN];
				StackStringBuilder cb(containerType, sizeof containerType);

				cb << arg.ArgType();
				cb << "Of";
				AppendNonContainerType_SXY_Private(cb, arg.ElementType(), enums, structs, delegates, true);
				
				if (knownObjects.find(containerType) == knownObjects.end())
				{
					// New object
					knownObjects.insert(containerType, 0);
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
using namespace Rococo::UE::Marshal;

#pragma pack(push, 1)

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
	sb << "\t}\n";

	for (size_t i = 0; i < classDef.MethodCount(); i++)
	{
		auto& method = classDef.GetFunction(i);
		BuildMethod(classDef, method, sb, enums, structs, delegates);
	}

	sb << "}\n\n"; // End of anonymous namespace

	sb << "#pragma pack(pop)\n\n";

	sb << "namespace Rococo::UE::Native\n{\n";
	sb.AppendFormat("\tvoid AddSexyNatives_Unreal_%s(IPublicScriptSystem& ss)\n", classDef.ShortName());
	sb << "\t{\n";
	sb.AppendFormat("\t\tUClass& classRef = GetStaticClassRef(TEXT(\"%s.%s\"));\n\n", classDef.PackageName(), classDef.ShortName());

	sb << "\t\tconst INamespace& nsHandles = ss.AddNativeNamespace(\"UE.Handles\");\n";
	for (auto& known : knownObjects)
	{
		cstr type = known.first;
		cstr prefix = StartsWith(type, "TArrayOf") || StartsWith(type, "TSetOf") || StartsWith(type, "TMapOf") ? "" : "H";
		sb.AppendFormat("\t\tss.CreateHandleType(nsHandles, __FILE__, __LINE__, \"%s", prefix);
		
		AppendContractedName(sb, type);

		sb << "\");\n";
	}

	if (!knownEnums.empty())
	{
		sb << "\n\t\tconst INamespace& nsEnums = ss.AddNativeNamespace(\"UE.Enums\");\n";
		for (auto& ePair : knownEnums)
		{
			auto* e = ePair.second;

			sb << "\t\tss.CreateEnumType(nsEnums, __FILE__, __LINE__, \"" << e->Name() << "\");\n";
		}
	}

	sb << "\n";

	sb << "\t\tconst INamespace& ns = ss.AddNativeNamespace(\"";
	
	sb << "UE.Native.";

	AppendPackageAsSexyNamespace(sb, classDef);

	sb << ".NS";
	sb << classDef.ClassIndex();
			
	sb << "\");\n";

	sb << "\t\tRococo::Script::AddNativeCallSecurity(ss, \"";
	
	sb << "UE.Native.";

	AppendPackageAsSexyNamespace(sb, classDef);

	sb << ".NS";
	sb << classDef.ClassIndex();

	sb << "\"";

	sb << ", \"!scripts/native/classes/";
	
	sb << classDef.ShortName() << ".sxy\");\n";

	std::vector<const IUnrealArg*> inputs;
	std::vector<const IUnrealArg*> outputs;

	sb << "\t\tss.AddNativeCall(ns, Construct_";
		
	AppendCompactName(sb, classDef.ShortName());
		
	sb <<", &classRef, \"Construct";
	AppendContractedName(sb, classDef.ShortName());
	sb << " -> (Int64 objectHandle)\", __FILE__, __LINE__);\n";

	sb << "\n";

	for (size_t i = 0; i < classDef.MethodCount(); i++)
	{
		auto& method = classDef.GetFunction(i);

		if (!method.HasSexyCounterpart())
		{
			sb << "/* ";
		}

		/* E.g:
			ScriptUFunction(ss, ns, __FILE__, __LINE__, classRef, BP_TopDownCharacter_C_GetActorScale3D, TEXT("GetActorScale3D"),
				"GetActorScale3D (Int64 objectHandle) (out FVector value) -> "); L"ActorHasTag", "ActorHasTag (int64 objectHandle) (R_FName tag) -> (bool returnValue)");
		*/

		sb << "\t\tScriptUFunction(ss, ns, __FILE__, __LINE__, classRef, ";
		
		AppendCompactName(sb, classDef.ShortName());
		sb << "_";
		method.AppendFunctionName(sb);

		sb << ", ";
		
		sb << "TEXT(\"";

		method.AppendFunctionName(sb);
		
		sb << "\"),\n\t\t\t\"";
		method.AppendFunctionName(sb, true);

		BuildSexyInputsAndOutputs(REF inputs, REF outputs, method);

		sb << " (Int64 objectHandle)";

		if (!inputs.empty())
		{
			sb << " ";
		}

		for (auto* input : inputs)
		{
			sb << "(";

			if (input->IsConst())
			{
				sb << "const ";
			}
			else if (input->IsCPPOutput())
			{
				sb << "out ";
			}

			AppendType(sb, *input, true, enums, structs, delegates);
			sb << " ";
			input->AppendName(sb, true);
			sb << ")";
		}

		for (auto* output : outputs)
		{
			if (!output->IsMarshalledByRef() && EndsWith(output->ArgType(), "^"))
			{
				// We have an argument by ref, but it is primitive, which sexy cannot marshal by ref, so instead we add an initial value and emit a finalized value
				sb << "(";

				AppendType(sb, *output, true, enums, structs, delegates);
				sb << " initial";

				char name[MAX_FQ_NAME_LEN];
				StackStringBuilder nameBuilder(name, sizeof name);
				output->AppendName(nameBuilder, true);

				name[0] = (char) toupper(name[0]);

				sb << name;

				sb << ")";
			}
		}

		sb << " -> ";

		for (auto* output : outputs)
		{
			sb << "(";

			AppendType(sb, *output, true, enums, structs, delegates);
			sb << " ";
			output->AppendName(sb, true);
			sb << ")";
		}

		sb << "\");\n";

		if (!method.HasSexyCounterpart())
		{
			sb << "*/";
		}

		sb << "\n";
	}

	sb << "\t}\n";
	sb << "}\n";

	sb << "namespace Rococo::UE::Native::" << FormatCPPNamespaceFromPath(classDef.PackageName()) << "\n";
	sb << "{\n";
	sb << "\tvoid RegisterNatives_" << classDef.ShortName() << "(ISexyNativeRegistry& registry)\n";
	sb << "\t{\n";
	sb << "\t\tregistry.AddNativeAPI(\"" << classDef.PackageName() << "\", \"" << classDef.ShortName() << "\", ";
	sb.AppendFormat("AddSexyNatives_Unreal_%s);\n", classDef.ShortName());
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

void InnerUnrealTypeToMarshalledType(char* buffer, size_t capacity, cstr unrealType, IEnums& enums)
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

	auto* enumDef = enums.FindEnum(unrealType);
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

void AppendHeaderForType(StringBuilder& sb, cstr typeName, IEnums& enums)
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
		AppendHeaderForType(sb, innerType, enums);
		return;
	}

	fstring prefices[] = { softObjectPtrPrefix, subclassOfPrefix, ""_fstring };
	if (TryGetInnerType(innerType, sizeof innerType, typeName, prefices))
	{
		AppendHeaderForType(sb, innerType, enums);
		return;
	}

	auto* enumDef = enums.FindEnum(typeName);
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

void BuildSexyNativeStructsSXY(IUnrealStruct& structDef, StringBuilder& sb)
{
	char sxyStructName[128];
	SafeFormat(sxyStructName, "%s", structDef.TypeName());

	char sxyNS[128];
	SecureFormat(sxyNS, "%s", structDef.Package());

	int len = sb.Length();

	sb << "(rock UEF" << sxyStructName;

	while (sb.Length() - len < 64)
	{
		sb << " ";
	}
	
	len = sb.Length();
	sb << " " << structDef.SizeOf();
	
	while (sb.Length() - len < 4)
	{
		sb << " ";
	}
	
	sb << " UE.";
	
	AppendPackageAsSexyNamespace(sb, structDef);
	
	sb << "." << sxyStructName << ")\n";
}

void BuildSexyNativeStructsHPP(IUnrealStruct& structDef, StringBuilder& sb, IEnums& enums, IDelegates& delegates)
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
			AppendHeaderForType(sb, e.InnerValueType(), enums);
			continue;
		}
		else if (Eq(e.TypeName(), "TMap"))
		{
			AppendHeaderForType(sb, e.InnerKeyType(), enums);
			AppendHeaderForType(sb, e.InnerValueType(), enums);
			continue;
		}
		else if (StartsWith(e.TypeName(), "TDelegate<"))
		{
			char delegateType[256];
			if (TryGetInnerType(delegateType, sizeof delegateType, e.TypeName(), "TDelegate<"_fstring))
			{
				sb.AppendFormat("#include \"Delegate/%s.hpp\"\n", delegateType);
				delegates.AddDelegate(delegateType, e.SizeOf());
			}
			continue;
		}
		else
		{
			AppendHeaderForType(sb, e.TypeName(), enums);
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
			InnerUnrealTypeToMarshalledType(innerType, sizeof innerType, e.InnerValueType(), enums);
			sb.AppendFormat("\t\tR_TArray<%s> ", innerType);
		}
		else if (Eq(e.TypeName(), "TSet"))
		{
			InnerUnrealTypeToMarshalledType(innerType, sizeof innerType, e.InnerValueType(), enums);
			sb.AppendFormat("\t\tR_TSet<%s> ", innerType);
		}
		else if (Eq(e.TypeName(), "TMap"))
		{
			InnerUnrealTypeToMarshalledType(innerType, sizeof innerType, e.InnerValueType(), enums);

			char keyType[256];
			InnerUnrealTypeToMarshalledType(keyType, sizeof keyType, e.InnerKeyType(), enums);
			sb.AppendFormat("\t\tR_TMap<%s,%s> ", keyType, innerType);
		}
		else if (TryGetInnerType(innerType, sizeof innerType, e.TypeName(), enumAsBytePrefix))
		{
			auto* enumRef = enums.FindEnum(innerType);
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
				auto* enumRef = enums.FindEnum(e.TypeName());
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

void AppendUnrealArgAsSexyPair(StringBuilder& sb, const IUnrealArg& arg, bool addMutability, IEnums& enums, IStructs& structs, IDelegates& delegates)
{
	sb << "(";

	if (addMutability)
	{
		if (arg.IsConst())
		{
			sb << "const ";
		}
		else if (arg.IsCPPOutput())
		{
			sb << "out ";
		}
		else
		{
			cstr p = arg.ArgType();
			if (*p == 'F')
			{
				auto* structType = structs.FindStruct(p + 1);
				if (structType)
				{
					// The arg has been passed by value, but requires marshalling by const reference in Sexy
					// Sexy structs always are passed by reference, but we must add the const by hand
					sb << "const ";
				}
			}
		}
	}

	AppendType(sb, arg, true, enums, structs, delegates);
	sb << " ";
	arg.AppendName(sb, true);
	sb << ")";
}

void BuildSexyFiles(IUnrealClass& classRef, StringBuilder& sb, IEnums& enums, IStructs& structs, IDelegates& delegates)
{
	char sxyName[256];
	GetClassNameAsSxyType(sxyName, sizeof sxyName, classRef);

	char sexyNs[256];
	GetPackageNameAsSxyType(sexyNs, sizeof sexyNs, classRef);

	sb << "(namespace UE." << sexyNs;
	sb << ")\n\n";

	sb << "(using UE." << sexyNs;
	sb << ")\n\n";

	sb << "(class " << sxyName;
	sb << " (defines UE." << sexyNs;
	sb << ".I" << sxyName;
	sb << ")\n";
	sb << "\t(Int64 objectHandle)\n";
	sb << ")\n\n";

	sb << "(factory UE." << sexyNs;

	sb << ".New" << sxyName;

	sb << " UE." << sexyNs;

	sb << ".I" << sxyName;

	sb << " :\n";
	sb << "\t(construct " << sxyName;
	sb << ")\n";
	sb << ")\n";

	std::vector<const IUnrealArg*> inputs;
	std::vector<const IUnrealArg*> outputs;

	sb << "\n(method ";
	sb << sxyName;
	sb << ".Construct :\n";

	sb << "\t(UE.Native.";
	AppendPackageAsSexyNamespace(sb, classRef);
	sb << ".NS" << classRef.ClassIndex() << ".Construct";

	AppendContractedName(sb, classRef.ShortName());
	sb << " -> this.objectHandle)\n";
	sb << ")\n";

	for (size_t i = 0; i < classRef.MethodCount(); i++)
	{
		auto& method = classRef.GetFunction(i);

		sb << "\n(method ";
		sb << sxyName;
		sb << ".";

		method.AppendFunctionName(sb, true);

		BuildSexyInputsAndOutputs(REF inputs, REF outputs, method);

		for (auto* output : outputs)
		{
			if (EndsWith(output->ArgType(), "^") && !output->IsMarshalledByRef() && !output->IsConst())
			{
				sb << " (";
				AppendType(sb, *output, true, enums, structs, delegates);
				sb << " initial";
				output->AppendName(sb, true);
				sb << ")";
			}			
		}

		if (!inputs.empty())
		{
			sb << " ";
		}

		for (auto* input : inputs)
		{
			AppendUnrealArgAsSexyPair(sb, *input, true, enums, structs, delegates);
		}

		sb << " -> ";

		for (auto* output : outputs)
		{
			AppendUnrealArgAsSexyPair(sb, *output, false, enums, structs, delegates);
		}

		sb << ":\n";

		// Here we invoke the native function
		sb << "\t(UE.Native.";
		AppendPackageAsSexyNamespace(sb, classRef);
		sb << ".NS" << classRef.ClassIndex() << ".";

		method.AppendFunctionName(sb, true);

		sb << " this.objectHandle";

		bool isFirst = true;

		for (auto* output : outputs)
		{
			if (!isFirst)
			{
				sb << " ";
			}

			isFirst = false;

			if (EndsWith(output->ArgType(), "^") && !output->IsMarshalledByRef() && !output->IsConst())
			{
				sb << " initial";
				output->AppendName(sb, true);
			}
		}

		if (!inputs.empty())
		{
			sb << " ";
		}

		for (auto* input : inputs)
		{
			if (!isFirst)
			{
				sb << " ";
			}

			isFirst = false;

			input->AppendName(sb, true);
		}

		isFirst = true;

		if (!outputs.empty())
		{
			sb << " -> ";

			for (auto* output : outputs)
			{
				if (!isFirst)
				{
					sb << " ";
				}

				isFirst = false;

				output->AppendName(sb, true);
			}
		}

		sb << ")\n";

		sb << ")\n";
	}
}

namespace Rococo::Unreal
{
	struct APIGenerator : public IAPIGenerator
	{
		AutoFree<IDynamicStringBuilder> dsbAllStructsSXY = CreateDynamicStringBuilder(4_megabytes);

		APIGenerator()
		{
			
		}

		void Commit(crwstr sxyOutputDirectory) override
		{
			WideFilePath sexyScriptsDirectory;
			Format(sexyScriptsDirectory, L"%s", sxyOutputDirectory);

			WideFilePath wTargetSXYFile;
			Format(wTargetSXYFile, L"%lsnative-structs.sxy", sexyScriptsDirectory.buf);
			IO::ToSysPath(wTargetSXYFile.buf);

			IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetSXYFile, *dsbAllStructsSXY->Builder());
		}

		void GenStructDef(IUnrealStruct& structDef, crwstr outputDirectory, IEnums& enums, IDelegates& delegates) override
		{
			WideFilePath nativeDirectory;
			Format(nativeDirectory, L"%snatives\\", outputDirectory);

			cstr structName = structDef.TypeName();
			cstr packageName = UsePackageForFolders ? structDef.Package() : "";

			if (*packageName == '/')
			{
				packageName++;
			}

			AutoFree<IDynamicStringBuilder> dsbHPP = CreateDynamicStringBuilder(16_kilobytes);

			auto& sbHPP = dsbHPP->Builder();

			WideFilePath wTargetHPPFile;
			Format(wTargetHPPFile, L"%lsStruct/%hs/%hs.hpp", nativeDirectory, packageName, structName);
			IO::ToSysPath(wTargetHPPFile.buf);

			BuildSexyNativeStructsHPP(structDef, sbHPP, enums, delegates);
			BuildSexyNativeStructsSXY(structDef, dsbAllStructsSXY->Builder());

			IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
		}

		void Free() override
		{
			delete this;
		}

		void GenDelegateDef(cstr rawTypeName, int sizeInBytes, crwstr path) override
		{
			cstr typeName = rawTypeName;

			char mutedTypeName[256];

			if (EndsWith(rawTypeName, "^"))
			{
				CopyString(mutedTypeName, sizeof mutedTypeName, rawTypeName);
				mutedTypeName[strlen(mutedTypeName) - 1] = 0;
				typeName = mutedTypeName;
			}

			char delegateBody[256];
			StackStringBuilder sb(delegateBody, sizeof delegateBody);

			WideFilePath wTargetHPPFile;
			Format(wTargetHPPFile, L"%lsnatives\\Delegate\\%hs.hpp", path, typeName);
			IO::ToSysPath(wTargetHPPFile.buf);

			sb << R"(#pragma once

namespace Rococo::UE::Native::Delegate
{
)"

<< "\tclass R_" << typeName << "\n"
<< "\t{\n"
<< "\t\t";

			sb.AppendFormat("char _opaque_data[%d];\n", sizeInBytes);

			sb << "\t};\n}\n";

			IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetHPPFile, *sb);
		}

		void GenRock(StringBuilder& sb, IUnrealStruct& structure, cstr compactNS)
		{
			int sizeofStruct = structure.SizeOf();

			char fqName[MAX_FQ_NAME_LEN];
			StackStringBuilder fqb(fqName, sizeof fqName);

			fqb << "UE.";
			AppendPackageAsSexyNamespace(fqb, structure);
			fqb << ".F";
			fqb << structure.SXYTypeName();

			sb << "\t\t{\n";
			sb << "\t\t\tss.CreateRockType(" << compactNS << ", __FILE__, __LINE__, \"F" << structure.SXYTypeName() << "\", " << sizeofStruct << ");\n";
			sb << "\t\t\tIRockFactory& rf = factories.BindRockFactory(TEXT(\"" << structure.Package() << "/" << structure.TypeName() << "\"));\n";
			sb << "\t\t\tss.AddNativeCall(" << compactNS << ", ANON::ConstructUERock, &rf, \"++F" << structure.SXYTypeName() << "(out " << fqName << " item)->\", __FILE__, __LINE__, false, 0);\n";
			sb << "\t\t\tss.AddNativeCall(" << compactNS << ", ANON::DestructUERock, &rf, \"--F" << structure.SXYTypeName() << "(out " << fqName << " item)->\", __FILE__, __LINE__, false, 0);\n";
			sb << "\t\t}\n";
		}

		void GenRocks(IStructs& structs, crwstr outputDirectory) override
		{
			int fnLineNumber = __LINE__ - 2;
			AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(64_kilobytes);

			WideFilePath wTargetCPPFile;
			Format(wTargetCPPFile, L"%lsrocks.cpp", outputDirectory);
			IO::ToSysPath(wTargetCPPFile.buf);

			auto& sb = dsb->Builder();

			sb << "// Generated by " << __FILE__ << " line " << fnLineNumber << " (" << __FUNCTION__ << ")\n";
			sb << "#include <rococo.types.h>\n";
			sb << "#include <sexy.script.h>\n";
			sb << "#include \"rock.marshalling.h\"\n\n";
			
			sb << "using namespace Rococo::Script;\n\n";
			sb << "namespace Rococo::UE::Rocks\n";
			sb << "{\n";

			sb << "\tnamespace ANON\n";
			sb << "\t{\n";

			sb << "\t\tvoid ConstructUERock(NativeCallEnvironment& e)\n";
			sb << "\t\t{\n";
			sb << "\t\t\tvoid* rock;\n";
			sb << "\t\t\tReadInput(0, OUT rock, e);\n";
			sb << "\t\t\tauto* rf = reinterpret_cast<IRockFactory*>(e.context);\n";
			sb << "\t\t\trf->Construct(rock);\n";
			sb << "\t\t}\n";

			sb << "\t\tvoid DestructUERock(NativeCallEnvironment& e)\n";
			sb << "\t\t{\n";
			sb << "\t\t\tvoid* rock;\n";
			sb << "\t\t\tReadInput(0, OUT rock, e);\n";
			sb << "\t\t\tauto* rf = reinterpret_cast<IRockFactory*>(e.context);\n";
			sb << "\t\t\trf->Destruct(rock);\n";
			sb << "\t\t}\n";
		
			sb << "\t}\n";
			sb << "\n";

			sb << "\tSEXY_MARSHALLING_API void RegisterRocks(Rococo::Script::IPublicScriptSystem& ss, IRockFactories& factories)\n";
			sb << "\t{\n";

			stringmap<int> setOfNativeNamespaces;

			structs.EnumerateAll(
				[this, &sb, &setOfNativeNamespaces]
				(IUnrealStruct& structure) 
				{
					if (!structure.IsGenerated())
					{
						return;
					}

					char compactNS[MAX_FQ_NAME_LEN] = "ns";
					structure.CompactPackageName(compactNS + 2, sizeof compactNS - 2);

					if (setOfNativeNamespaces.find(structure.Package()) == setOfNativeNamespaces.end())
					{
						sb << "\t\tauto& ";
						sb << compactNS;
						sb << " = ss.AddNativeNamespace(\"UE.";
						AppendPackageAsSexyNamespace(sb, structure);
						sb << "\");\n";
						setOfNativeNamespaces.insert(structure.Package(), 0);
					}
					GenRock(sb, structure, compactNS);
				}
			);
			sb << "\t}\n";
			sb << "}\n";

			IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetCPPFile, *sb);
		}

		void AddCppFileName(crwstr filename)
		{
			HString newFilename;
			Format(OUT newFilename, "%ls", filename);
			allCppFileNames.push_back(newFilename);
		}

		void GenClassDef(IUnrealClass& classDef, crwstr outputDirectory, crwstr sxyOutputDirectory, IEnums& enums, IStructs& structs, IDelegates& delegates) override
		{
			WideFilePath nativeDirectory;
			Format(nativeDirectory, L"%snatives\\", outputDirectory);

			WideFilePath sexyScriptsDirectory;
			Format(sexyScriptsDirectory, L"%s", sxyOutputDirectory);

			cstr shortName = classDef.ShortName();
			cstr packageName = UsePackageForFolders ? classDef.PackageName() : "";
			cstr slash = "";

			if (*packageName == '/')
			{
				slash = "/";
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
			Format(wTargetCPPFile, L"%ls%hs%hs%hs.cpp", nativeDirectory.buf, packageName, slash, shortName);

			AddCppFileName(wTargetCPPFile);

			IO::ToSysPath(wTargetCPPFile.buf);

			WideFilePath wTargetHPPFile;
			Format(wTargetHPPFile, L"%ls%hs%hs%hs.hpp", nativeDirectory.buf, packageName, slash, shortName);
			IO::ToSysPath(wTargetHPPFile.buf);

			WideFilePath wTargetSXYFile;
			Format(wTargetSXYFile, L"%lsclasses\\%hs%hs%hs.sxy", sexyScriptsDirectory.buf, packageName, slash, shortName);
			IO::ToSysPath(wTargetSXYFile.buf);

			BuildSexyNativesCPP(classDef, sbCPP, enums, structs, delegates);
			BuildSexyNativesHPP(classDef, sbHPP);
			BuildSexyFiles(classDef, sbSXY, enums, structs, delegates);

			IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetCPPFile, *sbCPP);
			IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
			IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetSXYFile, *sbSXY);
		}
	};

	IAPIGenerator* CreateAPIGenerator()
	{
		return new APIGenerator();
	}
}

void BuildSexyNativeEnumHPP(IUnrealEnumDef& structDef, StringBuilder& sb);

void GenEnumDef(IUnrealEnumDef& enumDef, crwstr outputDirectory)
{
	WideFilePath nativeDirectory;
	Format(nativeDirectory, L"%snatives\\", outputDirectory);

	cstr structName = enumDef.Name();
	cstr packageName = UsePackageForFolders ? enumDef.Package() : "";

	if (*packageName == '/')
	{
		packageName++;
	}

	AutoFree<IDynamicStringBuilder> dsbHPP = CreateDynamicStringBuilder(16_kilobytes);

	// .HPP builder
	auto& sbHPP = dsbHPP->Builder();

	AutoFree<IDynamicStringBuilder> dsbSXY = CreateDynamicStringBuilder(64_kilobytes);

	WideFilePath wTargetHPPFile;
	Format(wTargetHPPFile, L"%lsEnum\\%hs\\%hs.hpp", nativeDirectory, packageName, structName);
	IO::ToSysPath(wTargetHPPFile.buf);

	BuildSexyNativeEnumHPP(enumDef, sbHPP);

	IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetHPPFile, *sbHPP);
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

namespace Rococo::Unreal
{
	HString FormatCPPNamespaceFromPath(cstr path)
	{
		if (!path || *path != '/')
		{
			Throw(0, "Unexpected, path did not begin with a forward slash");
		}

		char ns[256];
		StackStringBuilder sb(ns, sizeof ns);
		
		cstr start = path + 1;

		while (*start != 0)
		{
			cstr next = FindChar(start, '/');
			if (next)
			{
				Substring token{ start, next };

				char buffer[256];
				token.CopyWithTruncate(buffer, sizeof buffer);

				sb << buffer;
				sb << "::";

				start = next + 1;
			}
			else
			{
				sb << start;
				break;
			}
		}

		return ns;
	}

	void AppendMethodArchetype(StringBuilder& sb, const IUnrealFunction& method, IEnums& enums, IStructs& structs, IDelegates& delegates)
	{
		sb << "\n\t(";

		method.AppendFunctionName(sb, true);

		std::vector<const IUnrealArg*> inputs;
		std::vector<const IUnrealArg*> outputs;

		BuildSexyInputsAndOutputs(REF inputs, REF outputs, method);

		if (!inputs.empty())
		{
			sb << " ";
		}

		for (auto* input : inputs)
		{
			sb << "(";

			if (input->IsConst())
			{
				sb << "const ";
			}
			else if (input->IsCPPOutput())
			{
				sb << "out ";
			}

			AppendType(sb, *input, true, enums, structs, delegates);
			sb << " ";
			input->AppendName(sb, true);
			sb << ")";
		}

		sb << " -> ";

		for (auto* output : outputs)
		{
			sb << "(";

			AppendType(sb, *output, true, enums, structs, delegates);
			sb << " ";
			output->AppendName(sb, true);
			sb << ")";
		}

		sb << ")";
	}

	struct ClassSystem: public IClassSystem
	{		
		WideFilePath wNativeDirectory;
		WideFilePath wSxyDirectory;

		struct ClassRep
		{
			HString package;
			HString cppNS;
			HString className;
		};

		std::vector<ClassRep> classes;

		IEnums& enums;
		IStructs& structs;
		IDelegates& delegates;

		ClassSystem(crwstr outputDirectory, crwstr sxyOutputDirectory, IEnums& _enums, IStructs& _structs, IDelegates& _delegates):
			enums(_enums), structs(_structs), delegates(_delegates)
		{
			Format(wNativeDirectory, L"%snatives\\", outputDirectory);
			Format(wSxyDirectory, L"%sclasses\\", sxyOutputDirectory);
		}

		void CommitHeader()
		{
			AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(16_kilobytes);
			auto& sb = dsb->Builder();
			sb << "#include \"../unreal-sexy-marshalling.h\"\n\n";

			HString previousNamespace = "";

			int nsCount = 0;

			for (auto& rep : classes)
			{
				if (previousNamespace != rep.cppNS)
				{
					if (nsCount > 0)
					{
						sb << "}\n\n";
					}
					nsCount++;

					sb << "namespace Rococo::UE::Native::" << rep.cppNS << "\n";					
					sb << "{\n";
				}
				
				sb << "\tvoid RegisterNatives_" << rep.className << "(ISexyNativeRegistry &registry);\n";

				previousNamespace = rep.cppNS;
			}

			if (nsCount > 0)
			{
				sb << "}\n\n";
			}


			WideFilePath wTargetHPPFile;
			Format(wTargetHPPFile, L"%ls/sexy-register.h", wNativeDirectory.buf);
			IO::ToSysPath(wTargetHPPFile.buf);

			Rococo::IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetHPPFile, *sb);
		}

		void CommitSource()
		{
			AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(16_kilobytes);
			auto& sb = dsb->Builder();

			sb << "#include \"sexy-register.h\"\n\n";
			sb << "namespace Rococo::UE::Native\n";
			sb << "{\n";
			sb << "\tvoid RegisterNatives(ISexyNativeRegistry& registry)\n";
			sb << "\t{\n";

			for (auto& rep : classes)
			{
				sb << "\t\t" << rep.cppNS << "::RegisterNatives_" << rep.className << "(registry);\n";
			}

			sb << "\t}\n";
			sb << "}\n";

			WideFilePath wTargetCPPFile;
			Format(wTargetCPPFile, L"%ls/sexy-register.cpp", wNativeDirectory.buf);
			IO::ToSysPath(wTargetCPPFile.buf);

			Rococo::IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetCPPFile, *sb);
		}

		void CommitUnityBuild()
		{
			WideFilePath wTargetCPPFile;

			AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(16_kilobytes);
			auto& sb = dsb->Builder();

			int index = 0;

			std::sort(allCppFileNames.begin(), allCppFileNames.end(),
				[](const HString& a, const HString& b)
				{
					return _stricmp(a, b) < 0;
				}
			);

			for (auto& filename : allCppFileNames)
			{
				if ((index % 100) == 0)
				{
					if (sb.Length() > 0)
					{
						Rococo::IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetCPPFile, *sb);
					}

					Format(wTargetCPPFile, L"%ls../all-files-%02.2d.cpp", wNativeDirectory.buf, 1 + (index / 100));
					IO::ToSysPath(wTargetCPPFile.buf);
					sb.Clear();
				}

				index++;

				sb << "#include \"" << filename.c_str() << "\"\n";
			}

			if (sb.Length() > 0)
			{
				Rococo::IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetCPPFile, *sb);
			}
		}

		void CommitSuggestions()
		{
			AutoFree<IDynamicStringBuilder> dsbSuggestions = CreateDynamicStringBuilder(64_kilobytes);

			// .sxy-hints builder
			auto& sb = dsbSuggestions->Builder();

			WideFilePath wTargetHintFile;
			Format(wTargetHintFile, L"%ls..\\hints\\interfaces.sxy", wSxyDirectory.buf);
			IO::ToSysPath(wTargetHintFile.buf);

			stringmap<int> namespaceCount;

			for (auto& c : classes)
			{
				char sexyNs[256];
				GetPackageNameAsSxyType(sexyNs, sizeof sexyNs, c.className, c.package);

				auto i = namespaceCount.insert(sexyNs, 0).first;
				int count = ++i->second;

				if (count == 1)
				{
					sb << "(namespace UE." << sexyNs << ")\n";
				}
			}

			sb << "\n\n\n\n";

			for (auto* fullDef : fullClassDefs)
			{
				char sexyNs[256];
				GetPackageNameAsSxyType(sexyNs, sizeof sexyNs, fullDef->ShortName(), fullDef->PackageName());

				sb << "(interface UE." << sexyNs << ".I";

				AppendNameAsSxyType(sb, *fullDef);

				for (size_t i = 0; i < fullDef->MethodCount(); i++)
				{
					auto& method = fullDef->GetFunction(i);
					AppendMethodArchetype(sb, method, enums, structs, delegates);
				}

				if (fullDef->MethodCount() > 0)
				{
					sb << "\n";
				}

				sb << ")\n";
			}

			if (sb.Length() > 0)
			{
				Rococo::IO::SaveAsciiTextFileIfDifferentAndLog(IO::TargetDirectory_Root, wTargetHintFile, *sb);
			}
		}

		void Commit() override
		{
			std::sort(classes.begin(), classes.end(),
				[](const ClassRep& a, const ClassRep& b)
				{
					int diff = _stricmp(a.cppNS, b.cppNS);
					if (diff < 0)
					{
						return true;
					}

					if (diff == 0)
					{
						return _stricmp(a.className, b.className) < 0;
					}

					return false;
				}
			);

			CommitHeader();
			CommitSource();
			CommitSuggestions();

			if (g_unityBuild)
			{
				CommitUnityBuild();
			}
		}

		virtual ~ClassSystem()
		{
			
		}

		std::vector<const IUnrealClass*> fullClassDefs;

		void AddClass(const IUnrealClass* classRef) override
		{
			classes.push_back({ classRef->PackageName(), FormatCPPNamespaceFromPath(classRef->PackageName()), classRef->ShortName() });
			fullClassDefs.push_back(classRef);
		}

		void Free() override
		{
			delete this;
		}
	};

	IClassSystem* CreateClassSystem(crwstr outputDirectory, crwstr sxyOutputDirectory, IEnums& enums, IStructs& structs, IDelegates& delegates)
	{
		return new ClassSystem(outputDirectory, sxyOutputDirectory, enums, structs, delegates);
	}
}