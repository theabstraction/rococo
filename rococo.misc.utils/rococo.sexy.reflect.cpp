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
	IPublicScriptSystem& ss;

	mutable const IStructure* structStringConstant = nullptr;
	mutable const IStructure* structfastStringBuilder = nullptr;

	std::unordered_map<ObjectStub*, ObjectDef> refToObject;
	std::vector<ObjectDef> exportQueue;
	std::unordered_map<ArrayImage*, ArrayDef> refToArray;
	std::vector<ArrayDef> arrayExportQueue;

	uint32 nextIndex = 1;

	IAssetBuilder& builder;

	cr_sex s;

	Asset(IPublicScriptSystem& _ss, IAssetBuilder& _builder, cr_sex _s) : ss(_ss), builder(_builder), s(_s)
	{

	}

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

	void SaveInterfaceRefAndObject(cstr name, const IStructure& assetType, InterfacePointer pInterface)
	{
		auto& interfaceType = assetType.GetInterface(0);

		ObjectStub* stub = InterfaceToInstance(pInterface);
		auto& objectType = *stub->Desc->TypeInfo;

		bool newDefinition = false;

		auto i = refToObject.find(stub);
		if (i == refToObject.end())
		{
			char objectName[Rococo::NAMESPACE_MAX_LENGTH];
			SecureFormat(objectName, "#%s%d", objectType.GetInterface(0).Name(), nextIndex);
			
			ObjectDef def = ObjectDef(objectName, stub, nextIndex++);
			i = refToObject.insert(std::make_pair(stub, def)).first;

			newDefinition = true;

			exportQueue.push_back(def);
		}

		builder.AppendSimpleString(i->second.objectName);
		builder.NextLine();
	}

	void SaveMembers(const IStructure& type, const uint8* pVariable)
	{
		const uint8* readPtr = pVariable;

		for (int j = 0; j < type.MemberCount(); ++j)
		{
			auto& member = type.GetMember(j);
			auto& memberType = *member.UnderlyingType();

			if (member.Name()[0] != '_')
			{
				SaveValues(memberType, member.Name(), readPtr);
			}

			readPtr += member.SizeOfMember();
		}

		size_t finalOffset = readPtr - pVariable;

		if (finalOffset != type.SizeOfStruct())
		{
			Throw(0, "%s: Bad maths", __FUNCTION__);
		}
	}

	void SaveValues(const IStructure& type, cstr name, const uint8* pVariable)
	{
		switch (type.VarType())
		{
		case VARTYPE_Int32:
			builder.AppendIndents();
			builder.AppendInt32(*(const int32*)pVariable);
			builder.NextLine();
			break;
		case VARTYPE_Int64:
			builder.AppendIndents();
			builder.AppendInt64(*(const int64*)pVariable);
			builder.NextLine();
			break;
		case VARTYPE_Float32:
			builder.AppendIndents();
			builder.AppendFloat32(*(const float32*)pVariable);
			builder.NextLine();
			break;
		case VARTYPE_Float64:
			builder.AppendIndents();
			builder.AppendFloat64(*(const float64*)pVariable);
			builder.NextLine();
			break;
		case VARTYPE_Bool:
			builder.AppendIndents();
			builder.AppendBool((*(const boolean32*)pVariable) == 1 ? true : false);
			builder.NextLine();
			break;
		case VARTYPE_Array:
			SaveArray(s, builder, name, type, *(ArrayImage**)pVariable);
			break;
		case VARTYPE_Derivative:
			if (IsNullType(type))
			{
				auto** ip = (InterfacePointer*) pVariable;
				SaveInterfaceRefAndObject(name, type, *ip);
			}
			else
			{
				SaveMembers(type, pVariable);
			}
			break;
		default:
			Throw(0, "Unhandled type");
			break;
		}
	}

	void SaveTypeAndMemberFormat(const IStructure& type)
	{
		builder.AppendObjectDesc(type.Name(), type.Module().Name());
		SaveMembers(type);
	}

	void SaveMembers(const IStructure& type)
	{
		int startIndex = 0;

		if (type.Prototype().IsClass)
		{
			startIndex = 2 + type.InterfaceCount();
		}

		for (int j = startIndex; j < type.MemberCount(); ++j)
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
					SaveMembers(memberType);
					builder.LeaveMembers();
				}
				
				break;
			case VARTYPE_Pointer:
				builder.AppendSimpleMemberDef(member.Name(), "*");
				break;
			case VARTYPE_Array:
				builder.AppendArrayMemberDef(member.Name(), member.UnderlyingGenericArg1Type()->Name(), member.UnderlyingGenericArg1Type()->Module().Name());
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
			builder.AppendSimpleString("<null>");
			builder.NextLine();
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

		builder.AppendArrayRef(arrayName);
	}

	void SaveNonContainerObject(const IStructure& type, cstr name, const uint8* rawObjectData)
	{
		if (IsContainerType(type.VarType()))
		{
			Throw(0, "To save an array, list or map, embed it in a struct or a class and save the containing object.");
		}

		builder.AppendSimpleString(name);
		builder.NextLine();

		SaveTypeAndMemberFormat(type);
		builder.AppendSimpleString("#");
		builder.NextLine();
		SaveValues(type, name, rawObjectData);
	}

	void SaveSexyObject(const IStructure& type, cstr name, ObjectStub* stub)
	{
		builder.AppendSimpleString(name);
		builder.NextLine();

		if (IsFastStringBuilderType(type))
		{
			auto& fsb = *(FastStringBuilder*)stub;
			builder.AppendSimpleString("_SB\t");
			builder.AppendInt32(fsb.length);
			builder.AppendSimpleString("\t");
			builder.AppendInt32(fsb.capacity);
			builder.AppendSimpleString("\t");
			builder.AppendFString(fstring{ fsb.buffer, fsb.length });
			builder.NextLine();
		}
		else if (IsStringConstantType(type))
		{
			auto& sc = *(CStringConstant*)stub;
			builder.AppendSimpleString("_SC\t");
			builder.AppendInt32(sc.length);
			builder.AppendSimpleString("\t");
			builder.AppendFString(fstring{ sc.pointer, sc.length });
			builder.NextLine();
		}
		else if (IsNullType(type))
		{
			builder.AppendSimpleString(type.Name());
			builder.AppendSimpleString("\t");
			builder.AppendSimpleString(type.Module().Name());
			builder.NextLine();
		}
		else
		{
			SaveTypeAndMemberFormat(type);
			builder.AppendSimpleString("#");
			builder.NextLine();
			SaveValues(type, name, (const uint8*) stub);
		}
	}

	void SaveArray(ArrayDef& def)
	{
		auto& arrayData = *def.image;

		auto& elementType = *arrayData.ElementType;

		builder.AppendArrayMeta(def.arrayName, elementType.Name(), elementType.Module().Name(), arrayData.NumberOfElements, arrayData.ElementCapacity);

		if (StartsWith(elementType.Name(), "_Null_") || IsPrimitiveType(elementType.VarType()))
		{
			// The type is stated in the meta data.
		}
		else
		{
			SaveTypeAndMemberFormat(elementType);
		}

		builder.EnterArray();

		auto* start = (const uint8*)arrayData.Start;

		auto* readPtr = start;

		for (int i = 0; i < arrayData.NumberOfElements; ++i)
		{
			try
			{
				builder.ArrayItemStart(i);
				SaveValues(elementType, def.arrayName, readPtr);
				builder.ArrayItemEnd();
				readPtr += arrayData.ElementLength;
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "Error saving element %d of array<%s of %s> %s:\n%s", i, def.image->ElementType->Name(), def.image->ElementType->Module().Name(), def.arrayName.c_str(), ex.Message());
			}
		}

		builder.LeaveMembers();
	}
	
	void SaveQueuedObjects()
	{
		while (!exportQueue.empty() || !arrayExportQueue.empty())
		{
			while (!exportQueue.empty())
			{
				builder.NextLine();

				ObjectDef nextObject = exportQueue.back();
				exportQueue.pop_back();

				SaveSexyObject(*nextObject.stub->Desc->TypeInfo, nextObject.objectName, nextObject.stub);
			}

			while (!arrayExportQueue.empty())
			{
				builder.NextLine();

				ArrayDef def = arrayExportQueue.back();
				arrayExportQueue.pop_back();

				SaveArray(def);
			}
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
	
	Asset asset(args.ss, *builder, args.s);

	auto& type = args.rhsType;

	try
	{
		asset.SaveNonContainerObject(type, "#Object0", (const uint8*)args.rhsData);
	}
	catch (IException& ex)
	{
		Throw(args.s, "Error saving %s of %s:\n%s\n", type.Name(), type.Module().Name(), ex.Message());
	}

	asset.SaveQueuedObjects();
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