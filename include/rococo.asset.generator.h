#pragma once

#include <rococo.api.h>

namespace Rococo::Script
{
	struct IPublicScriptSystem;
}

namespace Rococo::Sexy 
{
	struct IAssetLoader;
}

namespace Rococo::Assets
{
	ROCOCOAPI IAssetBuilder
	{
		virtual void AppendIndents() = 0;
		virtual void AppendHeader(cstr name, cstr typename, cstr moduleName) = 0;
		virtual void AppendInterfaceType(cstr interfaceType, cstr name, cstr moduleName) = 0;
		virtual void AppendObjectDesc(cstr type, cstr moduleName) = 0;
		virtual void AppendSimpleMemberDef(cstr name, cstr simpleType) = 0;
		virtual void AppendStringConstant(cstr name, cstr buffer, int32 length) = 0;
		virtual void AppendSimpleString(cstr text) = 0;
		virtual void AppendValue(cstr fieldName, int32 value) = 0;
		virtual void AppendValue(cstr fieldName, int64 value) = 0;
		virtual void AppendValue(cstr fieldName, float value) = 0;
		virtual void AppendValue(cstr fieldName, double value) = 0;
		virtual void AppendValue(cstr fieldName, bool value) = 0;
		virtual void AppendArrayMeta(cstr fieldName, cstr elementType, cstr elementSource, int32 numberOfElements, int32 elementCapacity) = 0;
		virtual void AppendFString(const fstring& text) = 0;
		virtual void AppendInt32(int32 value) = 0;
		virtual void AppendInt64(int64 value) = 0;
		virtual void AppendFloat32(float32 value) = 0;
		virtual void AppendFloat64(float64 value) = 0;
		virtual void AppendBool(bool value) = 0;
		virtual void ArrayItemStart(int32 index) = 0;
		virtual void ArrayItemEnd() = 0;
		virtual void NextLine() = 0;

		virtual void EnterArray() = 0;
		virtual void EnterMemberFormat(cstr typename, cstr moduleName) = 0;
		virtual void EnterMembers(cstr name, cstr typename, cstr moduleName) = 0;
		virtual void LeaveMembers() = 0;

		virtual void Free() = 0;
	};

	ROCOCOAPI IAssetGenerator
	{
		virtual IAssetBuilder* CreateAssetBuilder(const fstring& pingPath) = 0;
		virtual void Free() = 0;
	};

	// Enables Sexy semantic:
	// (reflect SaveAsset <SexyFileAsset-variable> <target-object>) 
	// to serialize a target-object to the supplied asset generator
	void LinkAssetGenerator(IAssetGenerator& generator, Rococo::Script::IPublicScriptSystem& ss);

	// Enables Sexy semantic:
	// (reflect LoadAsset <SexyFileAsset-variable> <target-object>) 
	// to serialize a target-object from the supplied asset loader
	void LinkAssetLoader(Rococo::Sexy::IAssetLoader& loader, Rococo::Script::IPublicScriptSystem& ss);

	// Create a CSV asset generator. These allow creation of files that save objects in CSV format.
	IAssetGenerator* CreateAssetGenerator_CSV(IInstallation& installation, bool addHumanReadableReferences);
}
