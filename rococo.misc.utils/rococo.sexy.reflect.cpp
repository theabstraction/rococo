#include <rococo.api.h>

#include <sexy.types.h>
#include <sexy.vm.cpu.h>
#include <sexy.vm.h>
#include <sexy.script.h>

#include <rococo.strings.h>

#include <rococo.asset.generator.h>
#include <rococo.csv.h>

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Sex;
using namespace Rococo::Sexy;
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

using namespace Rococo::Script;

struct Asset
{
	IPublicScriptSystem & ss;
	int objectCount = 0;

	const IStructure* structStringConstant = nullptr;

	Asset(IPublicScriptSystem& _ss): ss(_ss)
	{
		structStringConstant = &GetType("StringConstant", "Sys.Type.Strings.sxy");
	}

	const IStructure& GetType(cstr localName, cstr source)
	{
		auto* type = FindType(localName, source);
		if (type == nullptr)
		{
			Throw(0, "Cannot find type %s from %s", localName, source);
		}
		return *type;
	}

	const IStructure* FindType(cstr localName, cstr source)
	{
		auto& ppo = ss.PublicProgramObject();

		for (int i = 0; i < ppo.ModuleCount(); ++i)
		{
			auto& module = ppo.GetModule(i);
			if (Eq(module.Name(), source))
			{
				auto* pStruct = module.FindStructure(localName);
				return pStruct;
			}
		}

		return nullptr;
	}

	template<class T>
	const uint8* AppendPrimitiveAndReturnAdvancedPointer(const uint8* pField, IAssetBuilder& builder, cstr fieldName)
	{
		builder.AppendValue(fieldName, *(T*)pField);
		return pField + sizeof(T);
	}

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

	void SaveInterfaceRefAndObject(cr_sex s, IAssetBuilder& builder, cstr name, const IStructure& assetType, InterfacePointer pInterface)
	{
		auto& interfaceType = assetType.GetInterface(0);

		ObjectStub* stub = InterfaceToInstance(pInterface);
		auto& objectType = *stub->Desc->TypeInfo;

		if (&objectType == structStringConstant)
		{
			auto* sc = reinterpret_cast<CStringConstant*>(stub);
			builder.AppendStringConstant(name, sc->pointer, sc->length);
			return;
		}

		builder.AppendInterfaceType(interfaceType.Name(), name, assetType.Module().Name());

		char objectName[64];

		if (StartsWith(objectType.Name(), "_Null"))
		{
			SafeFormat(objectName, "0");
		}
		else
		{
			SafeFormat(objectName, "Object%d-%s", objectCount, objectType.Name());
		}

		builder.AppendObjectRef(objectType.Name(), objectType.Module().Name(), objectName);
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
			if (assetType.InterfaceCount() > 0)
			{
				// We have an interface reference
				InterfacePointer pInterface = *(InterfacePointer*)pField;
				pField += sizeof(InterfacePointer);
				SaveInterfaceRefAndObject(s, builder, name, assetType, pInterface);
			}
			else
			{
				builder.EnterMembers(name, assetType.Name(), assetType.Module().Name());
				pField = SaveAssetFields_Recursive(s, builder, name, assetType, assetData);
				builder.LeaveMembers();
			}
			break;
		default:
			Throw(s, "Only derivative and primitive value types are currently handled. type %s cannot be saved", assetType.Name());
		}
		return pField;
	}
};

// s has format (reflect SaveAsset <SexyAssetFile-variable> <object-type>)
static void SaveAssetWithSexyGenerator(IAssetGenerator* generator, Rococo::Script::ReflectionArguments& args)
{
	Validate_Type_Is_SexyAssetFile(args.s[2], args.lhsType);

	auto* assetFile = reinterpret_cast<SexyAssetFile*>(args.lhsData);
	auto filename = GetString(assetFile->ipStringFilename);

	if (!EndsWith(filename, ".sxya"))
	{
		Throw(args.s[2], "Expecting filename inside the SexyAssetFile to end with .sxya");
	}

	AutoFree<IAssetBuilder> builder = generator->CreateAssetBuilder(filename);
	builder->AppendHeader(args.rhsName, args.rhsType.Name(), args.rhsType.Module().Name());

	Asset asset(args.ss);

	if (IsPrimitiveType(args.rhsType.VarType()))
	{
		asset.SaveAssetField_Recursive(args.s, *builder, args.rhsName, args.rhsType, (const uint8*)args.rhsData);
	}
	else
	{
		asset.SaveAssetFields_Recursive(args.s, *builder, args.rhsName, args.rhsType, (const uint8*)args.rhsData);
	}
}

static void LoadAssetWithSexyParser(IAssetLoader* loader, Rococo::Script::ReflectionArguments& args)
{
	Validate_Type_Is_SexyAssetFile(args.s[2], args.lhsType);

	auto* assetFile = reinterpret_cast<SexyAssetFile*>(args.lhsData);
	auto filename = GetString(assetFile->ipStringFilename);

	if (!EndsWith(filename, ".sxya"))
	{
		Throw(args.s[2], "Expecting filename inside the SexyAssetFile to end with .sxya");
	}

	loader->LoadAndParse(filename, args.rhsType, args.rhsData, args.ss);
}

namespace Rococo::Assets
{
	void LinkAssetGenerator(IAssetGenerator& generator, Rococo::Script::IPublicScriptSystem& ss)
	{
		ss.AddNativeReflectionCall("SaveAsset", ::SaveAssetWithSexyGenerator, &generator);
	}

	void LinkAssetLoader(IAssetLoader& loader, Rococo::Script::IPublicScriptSystem& ss)
	{
		ss.AddNativeReflectionCall("LoadAsset", ::LoadAssetWithSexyParser, &loader);
	}
}