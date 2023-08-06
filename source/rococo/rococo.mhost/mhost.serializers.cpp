#include "mhost.h"

#include <rococo.hashtable.h>
#include <vector>

#include <sexy.types.h>
#include <Sexy.S-Parser.h>

#include <rococo.os.h>

namespace Anon
{
	using namespace MHost;
	using namespace Rococo;
	using namespace Rococo::Sex;

	struct Variant
	{
		VARTYPE type;
		VariantValue value;
	};

	char ToHex(int32 c)
	{
		static const char* const digits = "01234567890ABCDEF";
		return digits[c & 0xF];
	}

	void AppendAsciiCode(StringBuilder& sb, char c)
	{
		char buf[3] = { 0,0,0 };
		buf[0] = ToHex(c >> 16);
		buf[1] = ToHex(c);
		sb << buf;
	}

	void AppendEscapedString(StringBuilder& sb, cstr text)
	{
		for (const char* s = text; *s != 0; s++)
		{
			char c = *s;
			switch (c)
			{
			case '\a': sb << "&a"; break;
			case '\b': sb << "&b"; break;
			case '\f': sb << "&f"; break;
			case '\r': sb << "&r"; break;
			case '\n': sb << "&n"; break;
			case '\t': sb << "&t"; break;
			default:
				if (c < 31 || c > 127)
				{
					sb << "&x";
					AppendAsciiCode(sb, c);
				}
				else
				{
					char buf[2] = { c,0 };
					sb << buf;
				}
				break;
			}
		}
	}

	void AppendValue(StringBuilder& sb, cstr key, const Variant& v)
	{
		sb << "(";

		switch (v.type)
		{
		case VARTYPE_Bool:     sb << "B32"; break;
		case VARTYPE_Float32:  sb << "F32"; break;
		case VARTYPE_Float64:  sb << "F64"; break;
		case VARTYPE_Int32:    sb << "I32"; break;
		case VARTYPE_Int64:    sb << "I64"; break;
		case VARTYPE_Pointer:  sb << "STR"; break;
		default: Throw(0, "AppendValue(StringBuilder& sb, cstr key, const Variant& v): Unhandled type");
		}

		sb << " ";
		sb << key;
		sb << " ";

		switch (v.type)
		{
		case VARTYPE_Bool:     sb << ((v.value.int32Value != 0) ? "true" : "false"); break;
		case VARTYPE_Float32:  sb << v.value.floatValue; break;
		case VARTYPE_Float64:  sb << v.value.doubleValue; break;
		case VARTYPE_Int32:    sb << v.value.int32Value; break;
		case VARTYPE_Int64:    sb << v.value.int64Value; break;
		case VARTYPE_Pointer: 
			{
				sb << "\"";
				AppendEscapedString(sb, v.value.charPtrValue);
				sb << "\"";
			}
			break;
		default: Throw(0, "AppendValue(StringBuilder& sb, cstr key, const Variant& v): Unhandled type");
		}
		sb << ")";
	}

	static const char* const typeId = "S-Format-Dictionary-v1.0";

	struct DictionaryStream : IDicionaryStreamSupervisor
	{
		IO::IInstallation& installation;
		bool prohibitOverwrite = false;

		stringmap<Variant> map;
		std::vector<HString> keysByOriginalOrder;

		DictionaryStream(IO::IInstallation& _installation) : installation(_installation)
		{

		}

		~DictionaryStream()
		{
			Clear();
		}

		void Add(const fstring& key, const Variant& v)
		{
			auto i = map.find(key);
			if (i != map.end())
			{
				if (prohibitOverwrite)
				{
					Throw(0, "DictionaryStream: operation would overwrite key [%s]", (cstr) i->first);
				}

				if (v.type == VARTYPE_Pointer)
				{
					free(i->second.value.charPtrValue);
				}
			}
			else
			{
				keysByOriginalOrder.push_back(HString(key));
			}

			if (v.type == VARTYPE_Pointer)
			{
				Variant newV;
				newV.type = VARTYPE_Pointer;
				newV.value.charPtrValue = _strdup(v.value.charPtrValue);
				map[(cstr)key] = newV;
			}
			else
			{
				map[(cstr)key] = v;
			}
		}

		void AddBool(const fstring& name, boolean32 value) override
		{
			Variant v;
			v.value.int32Value = value;
			v.type = VARTYPE_Bool;
			Add(name, v);
		}

		void AddI32(const fstring& name, int32 value) override
		{
			Variant v;
			v.value.int32Value = value;
			v.type = VARTYPE_Int32;
			Add(name, v);
		}

		void AddI64(const fstring& name, int64 value) override
		{
			Variant v;
			v.value.int64Value = value;
			v.type = VARTYPE_Int64;
			Add(name, v);
		}

		void AddF32(const fstring& name, float value) override
		{
			Variant v;
			v.value.floatValue = value;
			v.type = VARTYPE_Float32;
			Add(name, v);
		}

		void AddF64(const fstring& name, double value) override
		{
			Variant v;
			v.value.doubleValue = value;
			v.type = VARTYPE_Float64;
			Add(name, v);
		}

		void AddString(const fstring& name, const fstring& value) override
		{
			Variant v;
			v.value.charPtrValue = const_cast<char*>(value.buffer);
			v.type = VARTYPE_Pointer;
			Add(name, v);
		}

		void Clear() override
		{
			keysByOriginalOrder.clear();

			for (auto& i : map)
			{
				if (i.second.type == VARTYPE_Pointer)
				{
					auto* s = i.second.value.charPtrValue;
					free(s);
				}
			}
			map.clear();
		}

		static VARTYPE ParseType(cstr typeString, cr_sex src)
		{
			if (Eq(typeString, "I32")) return VARTYPE_Int32;
			if (Eq(typeString, "I64")) return VARTYPE_Int64;
			if (Eq(typeString, "F32")) return VARTYPE_Float32;
			if (Eq(typeString, "F64")) return VARTYPE_Float64;
			if (Eq(typeString, "B32")) return VARTYPE_Bool;
			if (Eq(typeString, "STR")) return VARTYPE_Pointer;
			Throw(src, "Cannot determine VARTYPE. Must be one of I32, I64, F32, F64, B32, STR");
		}

		void Add(VARTYPE type, cstr name, cstr value)
		{
			switch (type)
			{
			case VARTYPE_Int32:
				AddI32(to_fstring(name), atoi(value));
				break;
			case VARTYPE_Int64:
				AddI64(to_fstring(name), _atoi64(value));
				break;
			case VARTYPE_Float32:
				AddF32(to_fstring(name), (float) atof(value));
				break;
			case VARTYPE_Float64:
				AddF64(to_fstring(name), atof(value));
				break;
			case VARTYPE_Bool:
				AddBool(to_fstring(name), Eq(value, "true") ? 1 : 0);
				break;
			case VARTYPE_Pointer:
				AddString(to_fstring(name), to_fstring(value));
				break;
			default:
				Throw(0, "Unexpected VARTYPE");
				break;
			}
		}

		void Parse(cr_sex root)
		{
			if (root.NumberOfElements() != 6)
			{
				Throw(root, "Expecting 6 elements in an S-Format dictionary root level element");
			}

			cr_sex stype = root[1];
			AssertAtomic(stype);

			if (!Eq(stype.c_str(), typeId))
			{
				Throw(stype, "Expecting %s", typeId);
			}

			cr_sex sElementCount = root[3];
			AssertAtomic(sElementCount);

			int nElements = atoi(sElementCount.c_str());
			if (nElements < 0 || nElements > 10000000)
			{
				Throw(sElementCount, "Bad element count. Expecting 0 to 10,000,000");
			}

			keysByOriginalOrder.reserve(nElements);
			map.reserve(nElements);

			cr_sex sEntries = root[5];
			if (!IsCompound(sEntries) && !IsNull(sEntries))
			{
				Throw(sEntries, "Expecting compound expression");
			}

			for (int i = 0; i < sEntries.NumberOfElements(); i++)
			{
				cr_sex entry = sEntries[i];
				if (entry.NumberOfElements() != 3) Throw(entry, "Expecting 3 elements, <type> <name> <value>");
				cr_sex sType = GetAtomicArg(entry, 0);
				cr_sex sName = GetAtomicArg(entry, 1);
				cr_sex sValue = GetAtomicArg(entry, 2);

				VARTYPE type = ParseType(sType.c_str(), sType);
				cstr name = sName.c_str();
				cstr value = sValue.c_str();

				Add(type, name, value);
			}
		}

		static cstr ToString(VARTYPE type)
		{
			switch (type)
			{
			case VARTYPE_Bool: return "Bool";
			case VARTYPE_Float32: return "Float32";
			case VARTYPE_Float64: return "Float64";
			case VARTYPE_Int32: return "Int32";
			case VARTYPE_Int64: return "Int64";
			case VARTYPE_Pointer: return "IString";
			default: return "Unknown";
			}
		}

		boolean32/* value */ GetBool(const fstring& name, boolean32 defaultValue) override
		{
			auto i = map.find((cstr)name);
			if (i == map.end()) return defaultValue;
			else if (i->second.type != VARTYPE_Bool)
			{
				Throw(0, "DictionaryStream::GetBool [%s] was not a boolean32 but a %s", name.buffer, ToString(i->second.type));
			}

			return i->second.value.int32Value;
		}

		int32/* value */ GetInt32(const fstring& name, int32 defaultValue) override
		{
			auto i = map.find((cstr)name);
			if (i == map.end()) return defaultValue;
			else if (i->second.type != VARTYPE_Int32)
			{
				Throw(0, "DictionaryStream::GetInt32 [%s] was not an Int32 but a %s", name.buffer, ToString(i->second.type));
			}

			return i->second.value.int32Value;
		}

		int64/* value */ GetInt64(const fstring& name, int64 defaultValue) override
		{
			auto i = map.find((cstr)name);
			if (i == map.end()) return defaultValue;
			else if (i->second.type != VARTYPE_Int64)
			{
				Throw(0, "DictionaryStream::GetInt64 [%s] was not an Int64 but a %s", name.buffer, ToString(i->second.type));
			}

			return i->second.value.int64Value;
		}

		float/* value */ GetFloat32(const fstring& name, float defaultValue) override
		{
			auto i = map.find((cstr)name);
			if (i == map.end()) return defaultValue;
			else if (i->second.type != VARTYPE_Float32)
			{
				Throw(0, "DictionaryStream::GetFloat32 [%s] was not an Float32 but a %s", name.buffer, ToString(i->second.type));
			}

			return i->second.value.floatValue;
		}

		double/* value */ GetFloat64(const fstring& name, double defaultValue) override
		{
			auto i = map.find((cstr)name);
			if (i == map.end()) return defaultValue;
			else if (i->second.type != VARTYPE_Float64)
			{
				Throw(0, "DictionaryStream::GetFloat64 [%s] was not an Float64 but a %s", name.buffer, ToString(i->second.type));
			}

			return i->second.value.doubleValue;
		}

		void AppendString(const fstring& name, const fstring& defaultString, Strings::IStringPopulator& sb) override
		{
			auto i = map.find((cstr)name);
			if (i == map.end()) sb.Populate(defaultString);
			else if (i->second.type != VARTYPE_Pointer)
			{
				Throw(0, "DictionaryStream::AppendString [%s] was not an IString but a %s", name.buffer, ToString(i->second.type));
			}

			sb.Populate(i->second.value.charPtrValue);
		}

		void LoadFrom(const fstring& pingPath) override
		{
			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(pingPath, sysPath);

			Auto<ISParser> sparser = Sexy_CreateSexParser_2_0(Rococo::Memory::CheckedAllocator());
			Auto<ISourceCode> src = sparser->LoadSource(sysPath, { 1,1 });
			Auto<ISParserTree> tree;

			try
			{
				tree = sparser->CreateTree(*src);
				Clear();
				Parse(tree->Root());
			}
			catch (ParseException& ex)
			{
				auto a = ex.Start();
				auto b = ex.End();
				Throw(ex.ErrorCode(), "Error loading %s:\n (%d.%d) to (%d.%d). %s", sysPath.buf, a.x, a.y, b.x, b.y, ex.Message());
			}
		}

		void SaveTo(const fstring& pingPath) override
		{
			AutoFree<Rococo::IDynamicStringBuilder> isb = CreateDynamicStringBuilder(map.size() * 64);

			auto& sb = isb->Builder();

			sb << "// Rococo::Sex::S-Parser is recommended for parsing this file\n\n";

			sb.AppendFormat("(' %s ( \n", typeId, map.size());
			for(auto& key: keysByOriginalOrder)
			{
				sb << "\t";
				auto i = map.find(key);
				if (i == map.end()) Throw(0, "DictionaryStream::ArchiveTo(...) expecting value for [%s]", key.c_str());
				AppendValue(sb, key.c_str(), i->second);
				sb << "\n";
			}
			sb << "))\n";

			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(pingPath, sysPath);
			Rococo::IO::SaveAsciiTextFile(Rococo::IO::TargetDirectory_Root, sysPath, *sb);
		}

		void ProhibitOverwrite() override
		{
			prohibitOverwrite = true;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace MHost
{
	IDicionaryStreamSupervisor* CreateDictionaryStream(IO::IInstallation& installation)
	{
		return new Anon::DictionaryStream(installation);
	}
}