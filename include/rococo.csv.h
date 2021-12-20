#pragma once

#include <rococo.types.h>

namespace Rococo::Sexy
{
	// interface to set member variables of a derivative structure. A derivative structure can be recursive, containing a hierarchy of substructures of arbitrary type
	ROCOCOAPI IMemberBuilder
	{
		typedef cstr OBJECT_NAME;

		// Add an IString ref to a ConstantBuffer@Sys.Type.Strings.sxy
		virtual void AddStringConstant(cstr stringNameRef, cstr text, int32 stringLength) = 0;

		// Descend into derivative sub-member 
		

		// After AddDerivativeMember, this resumes building the parent member
		virtual void ReturnToParent() = 0;

		virtual void AddFastStringBuilder(cstr opbjectNameRef, fstring text, int32 capacity) = 0;

		// Readies the system for the specified object. If name is #Object0, we are building the root object
		virtual void BuildObject(cstr name, cstr type, cstr sourceFile) = 0;

		// Create a new array object, and allow references to it by name
		// SetArrayBuildIndex will make other build operations write to the array until a new container definition is specified
		virtual void AddArrayDefinition(cstr refName, cstr elementType, cstr elementTypeSource, int32 length, int32 capacity) = 0;

		// Tells the builder which array item is to be overwritten. Called following AddArrayDefinition
		virtual void SetArrayWriteIndex(int32 index) = 0;

		virtual void AddTypeDerivative(int32 memberDepth, cstr type, cstr memberName, cstr sourceFile) = 0;
		virtual void AddTypeInterface(int32 memberDepth, cstr interfaceType, cstr memberName, cstr sourceFile) = 0;
		virtual void AddTypeF32(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeF64(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeI32(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeI64(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeBool(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeArrayRef(int32 memberDepth, cstr memberName) = 0;

		virtual void AddF32ItemValue(int itemIndex, float value) = 0;
		virtual void AddF64ItemValue(int itemIndex, double value) = 0;
		virtual void AddI32ItemValue(int itemIndex, int32 value) = 0;
		virtual void AddI64ItemValue(int itemIndex, int64 value) = 0;
		virtual void AddBoolItemValue(int itemIndex, bool value) = 0;
		virtual void AddArrayRefValue(int itemIndex, cstr arrayName) = 0;
		virtual void AddObjectRefValue(int itemIndex, cstr objectName) = 0;

		virtual void AddContainerItemDerivative(int32 memberDepth, cstr name, cstr type, cstr typeSource) = 0;

		virtual void EnterDerivedContainerItem() = 0;

		virtual void LeaveDerivedContainerItem() = 0;
	};

	ROCOCOAPI ISexyObjectBuilder
	{
		virtual IMemberBuilder& MemberBuilder() = 0;
		virtual void SelectScriptSystem(Rococo::Script::IPublicScriptSystem& ss) = 0;
		virtual void SelectTarget(const Rococo::Compiler::IStructure& type, void* pObject) = 0;
		virtual void SelectRootTarget(const Rococo::Compiler::IStructure& rootType, void* pRootObject) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IAssetLoader
	{
		virtual void Free() = 0;
	
		// Attempt to load some asset, identified with a sourceFilename URL, and overwrite the destination object with data.
		virtual void LoadAndParse(cstr sourceFilename, const Rococo::Compiler::IStructure& destinationType, void* destinationAssetData, Rococo::Script::IPublicScriptSystem& ss) = 0;
	};

	IAssetLoader* CreateAssetLoader(IInstallation& installation);
}

namespace Rococo::IO
{
	ROCOCOAPI ICSVTokenParser
	{
		virtual void OnBadChar(Vec2i cursorPosition, char value) = 0;
		virtual void OnBlankLine(Vec2i cursorPosition) = 0;
		virtual void OnToken(int row, int column, cstr token, int stringLengthPlusNul) = 0;
		virtual void Reset() = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI ITabbedCSVTokenizer
	{
		virtual void Tokenize(cstr csvString, ICSVTokenParser & tokenParser) = 0;
		virtual void Free() = 0;
	};

	ITabbedCSVTokenizer* CreateTabbedCSVTokenizer();

	ICSVTokenParser* CreateSXYAParser(Rococo::Sexy::IMemberBuilder& memberBuilder);
}
