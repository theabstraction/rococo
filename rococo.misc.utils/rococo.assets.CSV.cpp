#include <rococo.asset.generator.h>
#include <rococo.strings.h>
#include <rococo.io.h>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::IO;
	using namespace Rococo::Assets;

	struct AssetBuilder_CSV : IAssetBuilder
	{
		AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(1024);
		StringBuilder& sb;
		HString pingPath;
		IInstallation& installation;
		char indentChar = '\t';

		bool addHumanReadableReferences;

		int indent = 0;

		void Indent()
		{
			sb.AppendChar(indentChar);
		}

		void AppendIndents()
		{
			for (int i = 0; i < indent; ++i)
			{
				Indent();
			}
		}

		AssetBuilder_CSV(bool _addHumanReadableReferences, const fstring& _pingPath, IInstallation& _installation):
			sb(dsb->Builder()),
			addHumanReadableReferences(_addHumanReadableReferences),
			pingPath(_pingPath),
			installation(_installation)
		{
			sb.AppendFormat("AssetBuilder_CSV\n");
		}

		void AppendHeader(cstr name, cstr type, cstr moduleName) override
		{
			AppendIndents();
			sb.AppendFormat("%s", name);
			Indent();
			sb.AppendFormat("%s", type);
			Indent();
			sb.AppendFormat("\"%s\"", moduleName);
			sb.AppendChar('\n');
		}

		void AppendInterfaceType(cstr type, cstr name, cstr moduleName)
		{
			AppendIndents();
			sb.AppendFormat("%s", name);
			Indent();
			sb.AppendFormat("@");
			Indent();
			sb.AppendFormat("%s", type);
			Indent();
			sb.AppendFormat("\"%s\"", moduleName);
		}

		void AppendCharSequence(cstr buffer, int32 length)
		{
			sb.AppendChar('"');

			for (int32 i = 0; i < length; i++)
			{
				switch (buffer[i])
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
					sb.AppendChar(buffer[i]);
					break;
				}
			}

			sb.AppendChar('"');
		}

		void AppendFString(const fstring& text) override
		{
			Indent();
			AppendCharSequence(text.buffer, text.length);
		}

		void AppendInt32(int32 value) override
		{
			Indent();
			sb.AppendFormat("%d", value);
		}

		void NextLine() override
		{
			sb.AppendChar('\n');
		}

		void AppendObjectDesc(cstr type, cstr moduleName) override
		{
			Indent();
			sb.AppendFormat("%s", type);
			Indent();
			sb.AppendFormat("\"%s\"", moduleName);
		}

		void AppendStringConstant(cstr name, cstr buffer, int32 length) override
		{
			AppendIndents();
			sb.AppendFormat("%s", name);
			Indent();
			sb.AppendFormat("$");
			Indent();
			AppendCharSequence(buffer, length);
			Indent();
			sb.AppendFormat("%d\n", length);
		}

		void AppendValue(cstr fieldName, int32 value) override
		{
			AppendIndents();
			sb.AppendFormat("%s", fieldName);
			Indent();
			sb.AppendFormat("i");
			Indent();
			sb.AppendFormat("0x%X", value);

			if (addHumanReadableReferences)
			{
				Indent();
				sb.AppendFormat("%d", value);
			}
			sb.AppendChar('\n');
		}

		void AppendValue(cstr fieldName, int64 value) override
		{
			AppendIndents();
			sb.AppendFormat("%s", fieldName);
			Indent();
			sb.AppendFormat("l");
			Indent();
			sb.AppendFormat("0x%XLL", value);

			if (addHumanReadableReferences)
			{
				Indent();
				sb.AppendFormat("\t%lld", value);
			}
			sb.AppendChar('\n');
		}

		static uint32 FloatToBinary(float value)
		{
			return *(uint32*)&value;
		}

		void AppendValue(cstr fieldName, float value) override
		{
			AppendIndents();
			sb.AppendFormat("%s", fieldName);
			Indent();
			sb.AppendFormat("f");
			Indent();
			sb.AppendFormat("0x%X", FloatToBinary(value));

			if (addHumanReadableReferences)
			{
				Indent();
				sb.AppendFormat("%g", value);
			}
			sb.AppendChar('\n');
		}

		static uint64 DoubleToBinary(double value)
		{
			return *(uint64*)&value;
		}

		void AppendValue(cstr fieldName, double value) override
		{
			AppendIndents();
			sb.AppendFormat("%s", fieldName);
			Indent();
			sb.AppendFormat("d");
			Indent();
			sb.AppendFormat("0x%llX", DoubleToBinary(value));
			if (addHumanReadableReferences)
			{
				Indent();
				sb.AppendFormat("%llg", value);
			}
			sb.AppendChar('\n');
		}

		void AppendValue(cstr fieldName, bool value) override
		{
			AppendIndents();
			sb.AppendFormat("%s", fieldName);
			Indent();
			sb.AppendFormat("?");
			Indent();
			sb.AppendFormat("%s", value ? "Y" : "N");
			sb.AppendChar('\n');
		}

		virtual void AppendSimpleString(cstr text) override
		{
			Indent();
			sb.AppendFormat("%s", text);
		}

		void EnterMembers(cstr name, cstr type, cstr moduleName) override
		{
			AppendIndents();
			sb.AppendFormat("%s", name);
			Indent();
			sb.AppendFormat("%s", type);
			Indent();
			sb.AppendFormat("\"%s\"", moduleName);
			sb.AppendChar('\n');
			indent++;
		}

		void LeaveMembers() override
		{
			indent--;
		}

		void Free() override
		{
			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(pingPath.c_str(), sysPath);
			AutoFree<IBinaryArchive> archive = CreateNewBinaryFile(sysPath);

			const auto& text = *sb;
			archive->Write(1, text.length, text.buffer);
			delete this;
		}
	};

	struct AssetGenerator_CSV : public IAssetGenerator
	{
		IInstallation& installation;
		bool addHumanReadableReferences;

		AssetGenerator_CSV(IInstallation& _installation, bool _addHumanReadableReferences):
			installation(_installation),
			addHumanReadableReferences(_addHumanReadableReferences)
		{

		}

		IAssetBuilder* CreateAssetBuilder(const fstring& pingPath) override
		{
			return new AssetBuilder_CSV(addHumanReadableReferences, pingPath, installation);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Assets
{
	IAssetGenerator* CreateAssetGenerator_CSV(IInstallation& installation, bool addHumanReadableReferences)
	{
		return new ANON::AssetGenerator_CSV(installation, addHumanReadableReferences);
	}
}