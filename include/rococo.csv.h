#pragma once

#include <rococo.types.h>

namespace Rococo::Sexy
{
	// interface to set member variables of a derivative structure. A derivative structure can be recursive, containing a hierarchy of substructures of arbitrary type
	ROCOCOAPI IMemberBuilder
	{
		typedef cstr OBJECT_NAME;

		// Add a boolean32 primitive
		virtual void AddBooleanMember(cstr name, bool value) = 0;

		// Add a double primitive
		virtual void AddDoubleMember(cstr name, double value) = 0;

		// Add a float primitive
		virtual void AddFloatMember(cstr name, float value) = 0;

		// Add an int32 primitive
		virtual void AddInt32Member(cstr name, int32 value) = 0;

		// Add an int64 primitive
		virtual void AddInt64Member(cstr name, int64 value) = 0;

		// Add an IString ref to a ConstantBuffer@Sys.Type.Strings.sxy
		virtual void AddStringConstant(cstr name, cstr text, int32 stringLength) = 0;

		// Descend into derivative sub-member 
		virtual void AddDerivativeMember(cstr type, cstr name, cstr sourceFile) = 0;

		// After AddDerivativeMember, this resumes building the parent member
		virtual void ReturnToParent() = 0;

		// Add an interface pointer and point it to an object. If object name is "0" then the instance is the universal null object of the specified instance type
		virtual void AddInterfaceMember(cstr name, cstr interfaceType, cstr interfaceSource, cstr instanceType, cstr instanceSource, OBJECT_NAME objectName) = 0;

		virtual void AddFastStringBuilder(cstr name, fstring text, int32 capacity, cstr objectRefName) = 0;

		// We are done adding members to the object and we need to build another object
		virtual void AddNewObject(cstr name, cstr type, cstr sourceFile) = 0;

		// Create a new array object, and allow references to it by name
		// SetArrayBuildIndex will make other build operations write to the array until a new container definition is specified
		virtual void AddArrayDefinition(cstr refName, cstr elementType, cstr elementTypeSource, int32 length, int32 capacity) = 0;

		// Tells the builder which array item is to be overwritten. Called following AddArrayDefinition
		virtual void SetArrayWriteIndex(int32 index) = 0;

		// Add a member variable to an object, which is a reference to an array.
		virtual void AddArrayRefMember(cstr name, cstr arrayRefName) = 0;

		virtual void AddContainerItemF32(int elementMemberIndex, int32 memberDepth, cstr memberName) = 0;
		virtual void AddContainerItemF64(int elementMemberIndex, int32 memberDepth, cstr memberName) = 0;
		virtual void AddContainerItemI32(int elementMemberIndex, int32 memberDepth, cstr memberName) = 0;
		virtual void AddContainerItemI64(int elementMemberIndex, int32 memberDepth, cstr memberName) = 0;
		virtual void AddContainerItemBool(int elementMemberIndex, int32 memberDepth, cstr memberName) = 0;

		virtual void AddF32ItemValue(int itemIndex, float value) = 0;
		virtual void AddF64ItemValue(int itemIndex, double value) = 0;
		virtual void AddI32ItemValue(int itemIndex, int32 value) = 0;
		virtual void AddI64ItemValue(int itemIndex, int64 value) = 0;
		virtual void AddBoolItemValue(int itemIndex, bool value) = 0;
		virtual void AddObjectRefValue(int itemIndex, cstr objectName) = 0;

		virtual void AddContainerItemDerivative(int32 memberDepth, cstr name, cstr type, cstr typeSource) = 0;

		virtual void EnterDerivedContainerItem() = 0;

		virtual void LeaveDerivedContainerItem() = 0;
	};

	ROCOCOAPI ISexyObjectBuilder
	{
		virtual IMemberBuilder& MemberBuilder() = 0;
		virtual void SelectTarget(const Rococo::Compiler::IStructure& type, void* pObject, Rococo::Script::IPublicScriptSystem& ss) = 0;
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
