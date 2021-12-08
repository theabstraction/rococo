#include <rococo.api.h>

#include <sexy.types.h>
#include <sexy.vm.cpu.h>
#include <sexy.vm.h>
#include <sexy.script.h>

#include <rococo.strings.h>

#include <rococo.asset.generator.h>
#include <rococo.csv.h>

#include <unordered_map>
#include <vector>

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

struct ObjectDef
{
	ObjectDef()
	{

	}

	ObjectDef(cstr name, ObjectStub* _stub, int32 _index) : objectName(name), stub(_stub), index(_index)
	{

	}

	HString objectName;
	ObjectStub* stub = nullptr;
	int index = 0;
};

struct ArrayDef
{
	HString arrayName;
	ArrayImage* image;
	int index;
};

struct Asset
{
	IPublicScriptSystem & ss;

	mutable const IStructure* structStringConstant = nullptr;
	mutable const IStructure* structfastStringBuilder = nullptr;

	std::unordered_map<ObjectStub*, ObjectDef> refToObject;
	std::vector<ObjectDef> exportQueue;
	std::unordered_map<ArrayImage*, ArrayDef> refToArray;
	std::vector<ArrayDef> arrayExportQueue;

	uint32 nextIndex = 1;

	~Asset()
	{
	}

	bool IsFastStringBuilderType(const IStructure& type) const
	{
		if (!structfastStringBuilder)
		{
			structfastStringBuilder = &GetType("FastStringBuilder", "Sys.Type.Strings.sxy");
			if (!structfastStringBuilder)
			{
				Throw(0, "Cannot get FastStringBuilder type");
			}
		}

		return structfastStringBuilder == &type;
	}

	bool IsStringConstantType(const IStructure& type) const
	{
		if (!structStringConstant)
		{
			structStringConstant = &GetType("StringConstant", "Sys.Type.Strings.sxy");
			if (!structStringConstant)
			{
				Throw(0, "Cannot get StringConstant type");
			}
		}

		return structStringConstant == &type;
	}

	IAssetBuilder& builder;

	Asset(IPublicScriptSystem& _ss, IAssetBuilder& _builder): ss(_ss), builder(_builder)
	{
	}

	const IStructure& GetType(cstr localName, cstr source) const
	{
		auto* type = FindType(localName, source);
		if (type == nullptr)
		{
			Throw(0, "Cannot find type %s from %s", localName, source);
		}
		return *type;
	}

	const IStructure* FindType(cstr localName, cstr source) const
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
		if (!StartsWith(fieldName, "_"))
		{
			builder.AppendValue(fieldName, *(T*)pField);
		}

		return pField + sizeof(T);
	}

	const uint8* SaveDerivativeFields_Recursive(cr_sex s, IAssetBuilder& builder, cstr name, const IStructure& assetType, const uint8* assetData)
	{
		const uint8* pMember = assetData;
		for (int i = 0; i < assetType.MemberCount(); ++i)
		{
			auto& member = assetType.GetMember(i);
			pMember = SavePrimitiveField_Recursive(s, builder, member.Name(), *member.UnderlyingType(), pMember);
		}

		return pMember;
	}

	void SaveInterfaceRefAndObject(cr_sex s, IAssetBuilder& builder, cstr name, const IStructure& assetType, InterfacePointer pInterface)
	{
		auto& interfaceType = assetType.GetInterface(0);

		ObjectStub* stub = InterfaceToInstance(pInterface);
		auto& objectType = *stub->Desc->TypeInfo;

		if (IsStringConstantType(objectType))
		{
			auto* sc = reinterpret_cast<CStringConstant*>(stub);
			builder.AppendStringConstant(name, sc->pointer, sc->length);
			return;
		}

		builder.AppendInterfaceType(interfaceType.Name(), name, assetType.Module().Name());

		builder.AppendObjectDesc(objectType.Name(), objectType.Module().Name());

		bool isFastStringBuilder = IsFastStringBuilderType(objectType);

		ObjectDef def;
		bool newDefinition = false;

		auto i = refToObject.find(stub);
		if (i == refToObject.end())
		{
			char objectName[64];

			bool isNullObject = StartsWith(objectType.Name(), "_Null");

			if (isNullObject)
			{
				SafeFormat(objectName, "0");
			}
			else
			{
				SafeFormat(objectName, "#Object%d", nextIndex);
			}

			def = ObjectDef(objectName, stub, nextIndex++);
			refToObject.insert(std::make_pair(stub, def));

			newDefinition = true;

			if (!isFastStringBuilder && !isNullObject)
			{
				exportQueue.push_back(def);
			}
		}
		else
		{
			def = i->second;
		}
		
		builder.AppendSimpleString(def.objectName);

		if (isFastStringBuilder && newDefinition)
		{
			auto* fb = reinterpret_cast<FastStringBuilder*>(stub);
			builder.AppendFString(fstring{ fb->buffer, fb->length });
			builder.AppendInt32(fb->length);
			builder.AppendInt32(fb->capacity);
		}
		
		builder.NextLine();		
	}

	void SaveValues(IAssetBuilder& builder, const IStructure& type, const uint8* pVariable)
	{
 		const uint8* readPtr = pVariable;

		for (int j = 0; j < type.MemberCount(); ++j)
		{
			auto& member = type.GetMember(j);
			auto& memberType = *member.UnderlyingType();

			switch (memberType.VarType())
			{
			case VARTYPE_Int32:
				builder.AppendIndents();
				builder.AppendInt32(*(const int32*)readPtr);
				builder.NextLine();
				break;
			case VARTYPE_Int64:
				builder.AppendIndents();
				builder.AppendInt64(*(const int64*)readPtr);
				builder.NextLine();
				break;
			case VARTYPE_Float32:
				builder.AppendIndents();
				builder.AppendFloat32(*(const float32*)readPtr);
				builder.NextLine();
				break;
			case VARTYPE_Float64:
				builder.AppendIndents();
				builder.AppendFloat64(*(const float64*)readPtr);
				builder.NextLine();
				break;
			case VARTYPE_Bool:
				builder.AppendIndents();
				builder.AppendBool((*(const boolean32*)readPtr) == 1 ? true : false);
				builder.NextLine();
				break;
			case VARTYPE_Derivative:
				builder.AppendIndents();
				builder.AppendSimpleString("(");
				builder.NextLine();
				builder.EnterMemberValues();
				SaveValues(builder, memberType, readPtr);
				builder.LeaveMembers();
				builder.AppendIndents();
				builder.AppendSimpleString(")");
				builder.NextLine();
				break;
			case VARTYPE_Pointer:
				Throw(0, "Cannot save pointer values");
				break;
			}

			readPtr += member.SizeOfMember();
		}
	}

	void SaveMemberFormat(const IStructure& type)
	{
		for (int j = 0; j < type.MemberCount(); ++j)
		{
			auto& member = type.GetMember(j);
			auto& memberType = *member.UnderlyingType();

			switch (memberType.VarType())
			{
			case VARTYPE_Int32:
				builder.AppendSimpleMemberDef(member.Name(), "i");
				break;
			case VARTYPE_Int64:
				builder.AppendSimpleMemberDef(member.Name(), "l");
				break;
			case VARTYPE_Float32:
				builder.AppendSimpleMemberDef(member.Name(), "f");
				break;
			case VARTYPE_Float64:
				builder.AppendSimpleMemberDef(member.Name(), "d");
				break;
			case VARTYPE_Bool:
				builder.AppendSimpleMemberDef(member.Name(), "?");
				break;
			case VARTYPE_Derivative:
				if (StartsWith(memberType.Name(), "_Null"))
				{
					builder.AppendHeader(member.Name(), memberType.GetInterface(0).Name(), memberType.Module().Name());
				}
				else
				{
 					builder.EnterMembers(member.Name(), memberType.Name(), memberType.Module().Name());
					SaveMemberFormat(memberType);
					builder.LeaveMembers();
				}
				
				break;
			case VARTYPE_Pointer:
				builder.AppendSimpleMemberDef(member.Name(), "*");
				break;
			default:
				Throw(0, "Cannot save unhandled var type values");
				break;
			}
		}
	}

	int32 nextArrayIndex = 1;

	void SaveArray(cr_sex s, IAssetBuilder& builder, cstr name, const IStructure& assetType, ArrayImage* arrayData)
	{
		if (!arrayData)
		{
			builder.AppendArrayRef(name, "null");
			return;
		}

		char arrayName[32];
		SafeFormat(arrayName, "array%d", nextArrayIndex++);

		auto i = refToArray.find(arrayData);
		if (i == refToArray.end())
		{
			ArrayDef def;
			def.index = nextArrayIndex;
			def.arrayName = arrayName;
			def.image = arrayData;

			i = refToArray.insert(std::make_pair(arrayData, def)).first;

			arrayExportQueue.push_back(def);
		}

		builder.AppendArrayRef(name, arrayName);
	}

	void SaveArray(ArrayDef& def)
	{
		builder.NextLine();

		auto& arrayData = *def.image;

		auto& elementType = *arrayData.ElementType;

		builder.AppendArrayMeta(def.arrayName, elementType.Name(), elementType.Module().Name(), arrayData.NumberOfElements, arrayData.ElementCapacity);

		//builder.EnterMemberFormat(elementType.Name(), elementType.Module().Name());
		SaveMemberFormat(elementType);
		//builder.LeaveMembers();

		builder.EnterArray();

		auto* start = (const uint8*)arrayData.Start;

		auto* readPtr = start;

		for (int i = 0; i < arrayData.NumberOfElements; ++i)
		{
			builder.ArrayItemStart(i);
			SaveValues(builder, elementType, readPtr);
			builder.ArrayItemEnd();
			readPtr += arrayData.ElementLength;
		}

		builder.LeaveMembers();
	}

	const uint8* SavePrimitiveField_Recursive(cr_sex s, IAssetBuilder& builder, cstr name, const IStructure& assetType, const uint8* assetData)
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
			if (assetType.InterfaceCount() == 1)
			{
				// We have an interface reference
				InterfacePointer pInterface = *(InterfacePointer*)pField;
				pField += sizeof(InterfacePointer);
				SaveInterfaceRefAndObject(s, builder, name, assetType, pInterface);
			}
			else
			{
				builder.EnterMembers(name, assetType.Name(), assetType.Module().Name());
				pField = SaveDerivativeFields_Recursive(s, builder, name, assetType, assetData);
				builder.LeaveMembers();
			}
			break;
		case VARTYPE_Array:
			SaveArray(s, builder, name, assetType, *(ArrayImage**) assetData);
			pField += sizeof(ArrayImage*);
			break;
		default:
			Throw(s, "Only derivative and primitive value types are currently handled. type %s cannot be saved", assetType.Name());
		}
		return pField;
	}

	void SaveObject(ObjectDef& def, cr_sex sSrcExpression)
	{
		builder.NextLine();
		builder.AppendHeader(def.objectName, def.stub->Desc->TypeInfo->Name(), def.stub->Desc->TypeInfo->Module().Name());

		SaveDerivativeFields_Recursive(sSrcExpression, builder, def.objectName, *def.stub->Desc->TypeInfo, (const uint8*) def.stub);
	}

	void SaveQueuedObjects(cr_sex sourceExpression)
	{
		while (!exportQueue.empty())
		{
			ObjectDef nextObject = exportQueue.back();
			exportQueue.pop_back();

			SaveObject(nextObject, sourceExpression);
		}

		while (!arrayExportQueue.empty())
		{
			ArrayDef def = arrayExportQueue.back();
			arrayExportQueue.pop_back();

			SaveArray(def);
		}
	}
};

// args.s has format (reflect SaveAsset <SexyAssetFile-variable> <object-type>)
static void SaveAssetWithSexyGenerator(IAssetGenerator* generator, ReflectionArguments& args)
{
	Validate_Type_Is_SexyAssetFile(args.s[2], args.lhsType);

	auto* assetFile = reinterpret_cast<SexyAssetFile*>(args.lhsData);
	auto filename = GetString(assetFile->ipStringFilename);

	if (!EndsWith(filename, ".sxya"))
	{
		Throw(args.s[2], "Expecting filename inside the SexyAssetFile to end with .sxya");
	}

	AutoFree<IAssetBuilder> builder = generator->CreateAssetBuilder(filename);

	builder->AppendHeader("#Object0", args.rhsType.Name(), args.rhsType.Module().Name());

	Asset asset(args.ss, *builder);

	if (IsPrimitiveType(args.rhsType.VarType()))
	{
		asset.SavePrimitiveField_Recursive(args.s, *builder, args.rhsName, args.rhsType, (const uint8*)args.rhsData);
	}
	else
	{
		if (args.rhsType.InterfaceCount() == 1)
		{
			InterfacePointer pInterface = *(InterfacePointer*) args.rhsData;
			ObjectStub* rootObject = InterfaceToInstance(pInterface);
			asset.refToObject[rootObject] = ObjectDef("#Object0", rootObject, 0);
		}
		asset.SaveDerivativeFields_Recursive(args.s, *builder, args.rhsName, args.rhsType, (const uint8*)args.rhsData);
	}

	asset.SaveQueuedObjects(args.s);
}

static void LoadAssetWithSexyParser(IAssetLoader* loader, ReflectionArguments& args)
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
	void LinkAssetGenerator(IAssetGenerator& generator, IPublicScriptSystem& ss)
	{
		ss.AddNativeReflectionCall("SaveAsset", ::SaveAssetWithSexyGenerator, &generator);
	}

	void LinkAssetLoader(IAssetLoader& loader, IPublicScriptSystem& ss)
	{
		ss.AddNativeReflectionCall("LoadAsset", ::LoadAssetWithSexyParser, &loader);
	}
}