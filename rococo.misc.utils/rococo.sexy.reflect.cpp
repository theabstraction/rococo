#include <rococo.api.h>

#include <sexy.types.h>
#include <sexy.vm.cpu.h>
#include <sexy.vm.h>
#include <sexy.script.h>

#include <rococo.strings.h>

#include <rococo.asset.generator.h>

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Assets;
using namespace Rococo::Compiler;

struct SexyAssetFile
{
	InterfacePointer ipStringFilename;
};

void Validate_Type_Is_SexyAssetFile(cr_sex s, const IStructure& type)
{
	if (!Eq(type.Name(), "SexyAssetFile"))
	{
		Throw(s, "Expecting type to be a SexyAssetFile");
	}

	if (type.VarType() != VARTYPE_Derivative)
	{
		Throw(s, "Expecting type SexyAssetFile to be a struct");
	}

	if (type.MemberCount() != 1)
	{
		Throw(s, "Expecting type SexyAssetFile to be a struct with 1 member");
	}

	auto& member0 = type.GetMember(0);
	if (!member0.IsInterfaceVariable())
	{
		Throw(s, "Expecting an interface variable in position 0 of SexyAssetType");
	}

	auto* member0Type = member0.UnderlyingType();
	if (!Eq(member0Type->Name(), "_Null_Sys_Type_IString"))
	{
		Throw(s, "Expecting a Sys.Type.IString in position 0 of SexyAssetType");
	}
}

inline ObjectStub* InterfaceToInstance(InterfacePointer i)
{
	auto* p = ((uint8*)i) + (*i)->OffsetToInstance;
	auto* obj = (ObjectStub*)p;
	return obj;
}

// All concrete IString objects, even the Null IString has two members (Int32 length) and (cstr pointer) that immediately follow the object stub, thus:
#pragma pack(push,1)
struct StringBase
{
	ObjectStub header;
	int32 length;
	cstr pointer;
};
#pragma pack(pop)

fstring GetString(InterfacePointer ip)
{
	auto* stub = InterfaceToInstance(ip);
	auto* stringBase = reinterpret_cast<StringBase*>(stub);
	return fstring{ stringBase->pointer, stringBase->length };
}

template<class T>
const uint8* AppendPrimitiveAndReturnAdvancedPointer(const uint8* pField, IAssetBuilder& builder, cstr fieldName)
{
	builder.AppendValue(fieldName, *(T*)pField);
	return pField + sizeof(T);
}

const uint8* SaveAssetField_Recursive(cr_sex s, IAssetBuilder& builder, cstr name, const IStructure& assetType, const uint8* assetData);

const uint8* SaveAssetFields_Recursive(cr_sex s, IAssetBuilder& builder, cstr name, const IStructure& assetType, const uint8* assetData)
{
	const uint8* pMember = assetData;
	for (int i = 0; i < assetType.MemberCount(); ++i)
	{
		auto& member = assetType.GetMember(i);
		pMember = SaveAssetField_Recursive(s, builder, member.Name(), *member.UnderlyingType(), pMember);
	}

	return pMember;
}

const uint8* SaveAssetField_Recursive(cr_sex s, IAssetBuilder& builder, cstr name, const IStructure& assetType, const uint8* assetData)
{
	const uint8* pField = assetData;
	switch (assetType.VarType())
	{
	case VARTYPE_Int32:
		pField = AppendPrimitiveAndReturnAdvancedPointer<int32>(pField, builder, name);
		break;
	case VARTYPE_Float32:
		pField = AppendPrimitiveAndReturnAdvancedPointer<float>(pField, builder, name);
		break;
	case VARTYPE_Int64:
		pField = AppendPrimitiveAndReturnAdvancedPointer<int64>(pField, builder, name);
		break;
	case VARTYPE_Float64:
		pField = AppendPrimitiveAndReturnAdvancedPointer<double>(pField, builder, name);
		break;
	case VARTYPE_Bool:
	{
		auto* pBoolean32 = (boolean32*)pField;
		boolean32 value = *pBoolean32;
		pField += sizeof(boolean32);
		builder.AppendValue(name, value == 1 ? true : false);
	}
		break;
	case VARTYPE_Pointer: // Pointers are not serialized
		pField += sizeof(void*);
		break;
	case VARTYPE_Derivative:
		builder.EnterMembers(name, assetType.Name(), assetType.Module().Name());
		pField = SaveAssetFields_Recursive(s, builder, name, assetType, assetData);
		builder.LeaveMembers();
		break;
	default:
		Throw(s, "Only derivative and primitive value types are currently handled. type %s cannot be saved", assetType.Name());
	}
	return pField;
}

// s has format (reflect SaveAsset <SexyAssetFile-variable> <object-type>)
static void SaveAssetWithSexyGenerator(IAssetGenerator* generator, cr_sex s, const IStructure& lhsType, void* lhsData, cstr rhsName, const IStructure& assetType, void* assetData)
{
	Validate_Type_Is_SexyAssetFile(s[2], lhsType);

	auto* assetFile = reinterpret_cast<SexyAssetFile*>(lhsData);
	auto filename = GetString(assetFile->ipStringFilename);

	if (!EndsWith(filename, ".sxya"))
	{
		Throw(s[2], "Expecting filename inside the SexyAssetFile to end with .sxya");
	}

	AutoFree<IAssetBuilder> builder = generator->CreateAssetBuilder(filename);
	builder->AppendHeader(rhsName, assetType.Name(), assetType.Module().Name());

	if (IsPrimitiveType(assetType.VarType()))
	{
		SaveAssetField_Recursive(s, *builder, rhsName, assetType, (const uint8*)assetData);
	}
	else
	{
		SaveAssetFields_Recursive(s, *builder, rhsName, assetType, (const uint8*)assetData);
	}
}

namespace Rococo::Assets
{
	void LinkAssetGenerator(IAssetGenerator& generator, Rococo::Script::IPublicScriptSystem& ss)
	{
		ss.AddNativeReflectionCall("SaveAsset", ::SaveAssetWithSexyGenerator, &generator);
	}
}