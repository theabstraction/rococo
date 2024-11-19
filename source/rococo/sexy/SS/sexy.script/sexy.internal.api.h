#pragma once

#include <rococo.types.h>

#ifdef _WIN32
# define VM_CALLBACK_CONVENTION _cdecl
#else
# define VM_CALLBACK_CONVENTION
#endif
#define VM_CALLBACK(x) void VM_CALLBACK_CONVENTION OnInvoke##x(VariantValue* registers, void* context)

namespace Rococo
{
	class NamespaceSplitter;
}

namespace Rococo::Compiler
{
	DECLARE_ROCOCO_INTERFACE IStructure;
	DECLARE_ROCOCO_INTERFACE IArchetype;
	DECLARE_ROCOCO_INTERFACE IProgramObject;
	DECLARE_ROCOCO_INTERFACE INamespaceBuilder;
}

namespace Rococo::Sex
{
	DECLARE_ROCOCO_INTERFACE ISExpression;

	typedef const ISExpression& cr_sex;

	void AssertMacroShortName(cr_sex s, cstr shortName);
	void AssertPascalCaseNameValid(cstr text, int length, int maxLength, cstr desc);
	void AssertSplitTail(NamespaceSplitter& splitter, cr_sex s, OUT cstr& body, OUT cstr& tail);
	Compiler::INamespaceBuilder& AssertGetSubspace(Compiler::IProgramObject& object, cr_sex s, cstr name);
	void ThrowTokenAlreadyDefined(cr_sex s, cstr name, cstr repository, cstr type);
	void AssertLocalVariableOrMember(cr_sex e);
	void AssertValidNamespaceDef(cr_sex e);
	void AssertValidArchetypeName(cr_sex src, cstr name);
	void AssertValidInterfaceName(cr_sex src, cstr name);
	void AssertValidStructureName(cr_sex e);
	void AssertValidFunctionName(cr_sex e);
	void AssertKeyword(cr_sex e, int arg, cstr name);
	void AssertCompoundOrNull(cr_sex e);
	void ValidateNumberOfInputArgs(cr_sex s, const Compiler::IArchetype& callee, int numberOfSuppliedInputArgs);
}

namespace Rococo::Script
{
	struct ArrayImage;
	struct MapImage;
	struct ListImage;

	DECLARE_ROCOCO_INTERFACE IScriptSystem;

	void AddArchiveRegister(CCompileEnvironment& ce, int saveTempDepth, int restoreTempDepth, BITCOUNT bits);
	void AlignedMemcpy(void* __restrict dest, const void* __restrict source, size_t nBytes);
	int AllocFunctionOutput(CCompileEnvironment& ce, const Compiler::IArchetype& callee, Sex::cr_sex sExpr);
	void DestroyElements(ArrayImage& a, IScriptSystem& ss);
	void DestroyObject(const Compiler::IStructure& type, uint8* item, IScriptSystem& ss);
	void IncrementRef(MapImage* mapImage);
	bool IsGetAccessor(const Compiler::IArchetype& callee);
	void ListRelease(ListImage* l, IScriptSystem& ss);
	bool TryCompileStringLiteralInputToTemp(CCompileEnvironment& ce, Sex::cr_sex s, int tempDepth, const Compiler::IStructure& inputType);
	void ValidateReturnType(Sex::cr_sex s, VARTYPE returnType, VARTYPE type);
	bool TryCompileMethodCallWithoutInputAndReturnValue(CCompileEnvironment& ce, Sex::cr_sex s, cstr instance, cstr methodName, VARTYPE returnType, const Compiler::IStructure* returnTypeStruct, const Compiler::IArchetype* returnArchetype);

	class CCompileEnvironment;
	int CompileInstancePointerArgFromTemp(CCompileEnvironment& ce, int tempDepth);

	void CompileGetStructRef(CCompileEnvironment& ce, Sex::cr_sex s, const Compiler::IStructure& inputType, cstr name);
	VARTYPE CompileMethodCallWithoutInputAndReturnNumericValue(CCompileEnvironment& ce, Sex::cr_sex s, cstr instance, cstr methodName);

	void ReturnOutput(CCompileEnvironment& ce, int outputOffset, VARTYPE returnType);
}