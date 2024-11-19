#pragma once

#include <rococo.types.h>

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

	void AlignedMemcpy(void* __restrict dest, const void* __restrict source, size_t nBytes);
	void DestroyElements(ArrayImage& a, IScriptSystem& ss);
	void DestroyObject(const Compiler::IStructure& type, uint8* item, IScriptSystem& ss);
	void IncrementRef(MapImage* mapImage);
	bool IsGetAccessor(const Compiler::IArchetype& callee);
	void ListRelease(ListImage* l, IScriptSystem& ss);
}