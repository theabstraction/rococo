// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.types.h>

namespace Rococo::Script
{
	bool IsSerializable(const Rococo::Compiler::IStructure& type);
}

namespace Rococo::Sexy
{
	// interface to set member variables of a derivative structure. A derivative structure can be recursive, containing a hierarchy of substructures of arbitrary type
	ROCOCO_INTERFACE IMemberBuilder
	{
		typedef cstr OBJECT_NAME;

		// Add an IString ref to a ConstantBuffer@Sys.Type.Strings.sxy
		virtual void AddStringConstant(cstr stringNameRef, cstr text, int32 stringLength) = 0;

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

		// Create a new map object, and allow references to it by name
		// SetMapBuildKey will make other build operations write to the map value until a new container definition is specified
		virtual void AddMapDefinition(cstr refName, cstr keyType, cstr keyTypeSource, cstr valueType, cstr valueTypeSource, int32 length) = 0;

		// Tell the builder to parse and set a new map node to the selected key
		virtual void SetMapKey(const fstring& keyValue) = 0;

		// Tell the builder to prepare for a list definition
		virtual void AddListDefinition(cstr refName, cstr valueType, cstr valueTypeSource, int32 length) = 0;

		// Tells the builder to write a new node. Index = 0 => head. [Index (N + 1) => (node N->Next)] The call will be repeated with the index incremented each time.
		virtual void AppendNewListNode(int32 index) = 0;

		virtual void AddTypeDerivative(int32 memberDepth, cstr type, cstr memberName, cstr sourceFile) = 0;
		virtual void AddTypeInterface(int32 memberDepth, cstr interfaceType, cstr memberName, cstr sourceFile) = 0;
		virtual void AddTypeF32(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeF64(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeI32(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeI64(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeBool(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeArrayRef(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeListRef(int32 memberDepth, cstr memberName) = 0;
		virtual void AddTypeMapRef(int32 memberDepth, cstr memberName) = 0;

		virtual void AddF32ItemValue(int itemIndex, float value) = 0;
		virtual void AddF64ItemValue(int itemIndex, double value) = 0;
		virtual void AddI32ItemValue(int itemIndex, int32 value) = 0;
		virtual void AddI64ItemValue(int itemIndex, int64 value) = 0;
		virtual void AddBoolItemValue(int itemIndex, bool value) = 0;
		virtual void AddArrayRefValue(int itemIndex, cstr arrayName) = 0;
		virtual void AddObjectRefValue(int itemIndex, cstr objectName) = 0;
		virtual void AddMapRefValue(int itemIndex, cstr mapName) = 0;
		virtual void AddListRefValue(int itemIndex, cstr mapName) = 0;

		virtual void AddNullObject(cstr objectNameRef, cstr nullType, cstr nullTypeModule) = 0;

		virtual void AddContainerItemDerivative(int32 memberDepth, cstr name, cstr type, cstr typeSource) = 0;
	};

	ROCOCO_INTERFACE ISexyObjectBuilder
	{
		virtual IMemberBuilder& MemberBuilder() = 0;
		virtual void SelectScriptSystem(Rococo::Script::IPublicScriptSystem& ss) = 0;
		virtual void SelectTarget(const Rococo::Compiler::IStructure& type, void* pObject) = 0;
		virtual void SelectRootTarget(const Rococo::Compiler::IStructure& rootType, void* pRootObject) = 0;
		virtual void Free() = 0;
	};
}

namespace Rococo::Sex::Assets
{
	// Parse the CSV string and invoke member builder functions to build the object hierarchy.
	// The implementation is guaranteed to take responsibility for minimizing dynamic memory allocation.
	void ParseTabbedCSV_AssetFile(cstr csvString, Rococo::Sexy::IMemberBuilder& builder);

	// Parse a CSV string and build up the targeted Sexy asset. This function is used by LoadAndParseSexyObjectTree to do the hard work
	void ParseSexyObjectTree(cstr treeAsCSVString, const Rococo::Compiler::IStructure& assetType, void* assetData, Rococo::Script::IPublicScriptSystem& ss);

	// Given a sexy object of given type, overwrites its fields with the object tree at the specified ping path with the CSV data specified by the ping path. 
	void LoadAndParseSexyObjectTree(IO::IInstallation& installation, cstr pingPath, const Rococo::Compiler::IStructure& assetType, void* assetData, Rococo::Script::IPublicScriptSystem& ss);
}
