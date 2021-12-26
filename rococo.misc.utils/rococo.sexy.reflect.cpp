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
using namespace Rococo::Script;
using namespace Rococo::Sex;
using namespace Rococo::Sexy;
using namespace Rococo::Assets;
using namespace Rococo::Compiler;

namespace
{
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

	
	fstring GetString(InterfacePointer ip)
	{
		// All concrete IString objects, even the Null IString has two members (Int32 length) and (cstr pointer) that immediately follow the object stub, thus:
		#pragma pack(push,1)
		struct StringBase
		{
			ObjectStub header;
			int32 length;
			cstr pointer;
		};
		#pragma pack(pop)

		auto* stub = InterfaceToInstance(ip);
		auto* stringBase = reinterpret_cast<StringBase*>(stub);
		return fstring{ stringBase->pointer, stringBase->length };
	}

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

}

struct AssetBuilder
{
	IPublicScriptSystem& ss;

	mutable const IStructure* structStringConstant = nullptr;
	mutable const IStructure* structfastStringBuilder = nullptr;

	// N.B TODO - if the asset builder was used for network serialization then these dynamic allocation classes could prove a bottleneck.
	std::unordered_map<ObjectStub*, ObjectDef> refToObject;
	std::vector<ObjectDef> exportQueue;
	std::unordered_map<ArrayImage*, ArrayDef> refToArray;
	std::vector<ArrayDef> arrayExportQueue;

	uint32 nextIndex = 1;

	StringBuilder& sb;

	cr_sex s;

	AssetBuilder(IPublicScriptSystem& _ss, cr_sex _s, StringBuilder& _sb) : ss(_ss), s(_s), sb(_sb)
	{

	}

	~AssetBuilder()
	{
	}

	bool IsFastStringBuilderType(const IStructure& type) const
	{
		if (!structfastStringBuilder)
		{
			structfastStringBuilder = &ss.GetTypeForSource("FastStringBuilder", "Sys.Type.Strings.sxy");
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
			structStringConstant = &ss.GetTypeForSource("StringConstant", "Sys.Type.Strings.sxy");
			if (!structStringConstant)
			{
				Throw(0, "Cannot get StringConstant type");
			}
		}

		return structStringConstant == &type;
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

		sb.AppendFormat(i->second.objectName);
		sb.AppendChar('\n');
	}

	void SaveMemberValues(const IStructure& type, const uint8* pVariable)
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
			sb.AppendFormat("%d\n", *(const int32*)pVariable);
			break;
		case VARTYPE_Int64:
			sb.AppendFormat("%lld\n", *(const int64*)pVariable);
			break;
		case VARTYPE_Float32:
			sb.AppendFormat("%f\n", *(const float32*)pVariable);
			break;
		case VARTYPE_Float64:
			sb.AppendFormat("%lf\n", *(const float64*)pVariable);
			break;
		case VARTYPE_Bool:
			sb.AppendFormat("%s\n", (*(const boolean32*)pVariable) == 1 ? "Y" : "N");
			break;
		case VARTYPE_Array:
			SaveArrayRef(s, name, type, *(ArrayImage**)pVariable);
			break;
		case VARTYPE_Derivative:
			if (IsNullType(type))
			{
				auto** ip = (InterfacePointer*) pVariable;
				SaveInterfaceRefAndObject(name, type, *ip);
			}
			else
			{
				SaveMemberValues(type, pVariable);
			}
			break;
		default:
			Throw(0, "Unhandled type");
			break;
		}
	}

	void AppendTypeNameAndModule(const IStructure& type)
	{
		sb.AppendFormat("%s\t%s", type.Name(), type.Module().Name());
	}

	void SaveTypeAndMemberFormat(const IStructure& type)
	{
		AppendTypeNameAndModule(type);
		sb.AppendChar('\n');
		SaveMemberTypes(type, 0);
	}

	void SaveMemberTypes(const IStructure& type, int depth)
	{
		int startIndex = 0;

		if (type.Prototype().IsClass)
		{
			startIndex = 2 + type.InterfaceCount();
		}

		if (IsPrimitiveType(type.VarType()))
		{
			Throw(0, "Primitive type cannot be archived directly. To archive a primitive place within a structure and archive the structure");
		}

		for (int j = startIndex; j < type.MemberCount(); ++j)
		{
			auto& member = type.GetMember(j);
			auto& memberType = *member.UnderlyingType();

			for (int i = 0; i < depth; i++)
			{
				sb.AppendChar('\t');
			}

			sb.AppendFormat(member.Name());
			sb.AppendChar('\t');

			switch (memberType.VarType())
			{
			case VARTYPE_Int32:
				sb.AppendFormat("i\n");
				break;
			case VARTYPE_Int64:
				sb.AppendFormat("l\n");
				break;
			case VARTYPE_Float32:
				sb.AppendFormat("f\n");
				break;
			case VARTYPE_Float64:
				sb.AppendFormat("d\n");
				break;
			case VARTYPE_Bool:
				sb.AppendFormat("?\n");
				break;
			case VARTYPE_Derivative:
				if (IsNullType(memberType))
				{
					sb.AppendFormat("@\t%s\t%s\n", memberType.GetInterface(0).Name(), memberType.Module().Name());
				}
				else
				{
					AppendTypeNameAndModule(memberType);
					sb.AppendChar('\n');
					SaveMemberTypes(memberType, depth + 1);
				}
				
				break;
			case VARTYPE_Pointer:
				sb.AppendFormat("*\n");
				break;
			case VARTYPE_Array:
				sb.AppendFormat("[]\n");
				break;
			default:
				Throw(0, "Cannot save unhandled var type values");
				break;
			}
		}
	}

	int32 nextArrayIndex = 1;

	void SaveArrayRef(cr_sex s, cstr name, const IStructure& assetType, ArrayImage* arrayData)
	{
		if (!arrayData)
		{
			sb.AppendFormat("<null>\n");
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

		sb.AppendFormat("%s\n", arrayName);
		return;
	}

	void SaveNonContainerObject(const IStructure& type, cstr name, const uint8* rawObjectData)
	{
		if (IsContainerType(type.VarType()))
		{
			Throw(0, "To save an array, list or map, embed it in a struct or a class and save the containing object.");
		}

		sb.AppendFormat("%s\t", name);

		SaveTypeAndMemberFormat(type);
		sb.AppendFormat("#\n", name);
		SaveValues(type, name, rawObjectData);
	}

	// Encoded a text string in tabbed CSV format to the string builder. Note that the text can include null characters.
	void AppendEncodedFString(const fstring& text)
	{
		sb.AppendChar('"');

		for (int32 i = 0; i < text.length; i++)
		{
			switch (text.buffer[i])
			{
			case '\t':
				sb.AppendFormat("\\t");
				break;
			case '"':
				sb.AppendFormat("\\\"");
				break;
			case '\\':
				sb.AppendFormat("\\\\");
				break;
			case '\0':
				sb.AppendFormat("\\0");
				break;
			default:
				sb.AppendChar(text.buffer[i]);
				break;
			}
		}

		sb.AppendChar('"');
	}

	void SaveSexyObject(const IStructure& type, cstr name, ObjectStub* stub)
	{
		sb.AppendFormat("%s\t", name);

		if (IsFastStringBuilderType(type))
		{
			auto& fsb = *(FastStringBuilder*)stub;
			sb.AppendFormat("_SB\t%d\t%d\t", fsb.length, fsb.capacity);
			AppendEncodedFString(fstring{ fsb.buffer, fsb.length });
			sb.AppendChar('\n');
		}
		else if (IsStringConstantType(type))
		{
			auto& sc = *(CStringConstant*)stub;
			sb.AppendFormat("_SC\t%d\t", sc.length);
			AppendEncodedFString(fstring{ sc.pointer, sc.length });
			sb.AppendChar('\n');
		}
		else if (IsNullType(type))
		{
			AppendTypeNameAndModule(type);
			sb.AppendChar('\n');
		}
		else
		{
			SaveTypeAndMemberFormat(type);
			sb.AppendFormat("#\n");
			SaveValues(type, name, (const uint8*) stub);
		}
	}

	void SaveArray(const ArrayDef& def)
	{
		auto& arrayData = *def.image;

		auto& elementType = *arrayData.ElementType;

		sb.AppendFormat("%s\t%s\t%s\t%d\t%d\n", def.arrayName.c_str(), elementType.Name(), elementType.Module().Name(), arrayData.NumberOfElements, arrayData.ElementCapacity);
		
		if (StartsWith(elementType.Name(), "_Null_") || IsPrimitiveType(elementType.VarType()))
		{
			// The type is stated in the meta data.
		}
		else
		{
			SaveMemberTypes(elementType, 0);
		}

		auto* start = (const uint8*)arrayData.Start;

		auto* readPtr = start;

		for (int i = 0; i < arrayData.NumberOfElements; ++i)
		{
			try
			{
				sb.AppendFormat("[%d]\n", i);
				SaveValues(elementType, def.arrayName, readPtr);
				readPtr += arrayData.ElementLength;
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "Error saving element %d of array<%s of %s> %s:\n%s", i, def.image->ElementType->Name(), def.image->ElementType->Module().Name(), def.arrayName.c_str(), ex.Message());
			}
		}
	}
	
	void SaveQueuedObjects()
	{
		// When you save a queued object it can entail creating new objects that are queued for saving

		while (!exportQueue.empty() || !arrayExportQueue.empty())
		{
			while (!exportQueue.empty())
			{
				sb.AppendChar('\n');

				ObjectDef nextObject = exportQueue.back();
				exportQueue.pop_back();

				SaveSexyObject(*nextObject.stub->Desc->TypeInfo, nextObject.objectName, nextObject.stub);
			}

			while (!arrayExportQueue.empty())
			{
				sb.AppendChar('\n');

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

	if (filename.length == 0 || !EndsWith(filename, ".sxya"))
	{
		Throw(args.s[2], "Expecting filename inside the SexyAssetFile to end with .sxya");
	}

	auto& sb = generator->GetReusableStringBuilder();
	sb.Clear();
	sb.AppendFormat("AssetBuilder_TabbedCSV_1.0\n");

	auto& type = args.rhsType;

	try
	{
		AssetBuilder assetBuilder(args.ss, args.s, sb);
		assetBuilder.SaveNonContainerObject(type, "#Object0", (const uint8*)args.rhsData);
		assetBuilder.SaveQueuedObjects();

		generator->Generate(filename, *sb);
	}
	catch (IException& ex)
	{
		Throw(args.s, "Error saving %s of %s:\n%s\n", type.Name(), type.Module().Name(), ex.Message());
	}
}

static void LoadAssetWithSexyParser(IInstallation* installation, ReflectionArguments& args)
{
	Validate_Type_Is_SexyAssetFile(args.s[2], args.lhsType);

	auto* assetFile = reinterpret_cast<SexyAssetFile*>(args.lhsData);
	auto filename = GetString(assetFile->ipStringFilename);

	if (filename.length == 0 || !EndsWith(filename, ".sxya"))
	{
		Throw(args.s[2], "Expecting filename inside the SexyAssetFile to end with .sxya");
	}

	try
	{
		Rococo::IO::LoadAndParseSexyObjectTree(*installation, filename, args.rhsType, args.rhsData, args.ss);
	}
	catch (IException& ex)
	{
		Throw(args.s, "%s", ex.Message());
	}
}

namespace Rococo::Assets
{
	void LinkAssetGenerator(IAssetGenerator& generator, IPublicScriptSystem& ss)
	{
		ss.AddNativeReflectionCall("SaveAsset", ::SaveAssetWithSexyGenerator, &generator);
	}

	void LinkAssetLoader(IInstallation& loader, IPublicScriptSystem& ss)
	{
		ss.AddNativeReflectionCall("LoadAsset", ::LoadAssetWithSexyParser, &loader);
	}
}