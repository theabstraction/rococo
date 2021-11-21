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

		bool addHumanReadableReferences;

		int indent = 0;

		void AppendIndents()
		{
			for (int i = 0; i < indent; ++i)
			{
				sb <<",";
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
			sb.AppendFormat("%s,%s,%s\n", name, type, moduleName);
		}

		void AppendValue(int32 value) override
		{
			AppendIndents();
			sb.AppendFormat("I,0x%X", value);

			if (addHumanReadableReferences)
			{
				sb.AppendFormat(",%d", value);
			}
		}

		void AppendValue(int64 value) override
		{
			AppendIndents();
			sb.AppendFormat("L,0x%XLL", value);
			if (addHumanReadableReferences)
			{
				sb.AppendFormat(",%lld", value);
			}
		}

		void AppendValue(float value) override
		{
			AppendIndents();
			sb.AppendFormat("F,0x%X", *(int*)(float*) &value);
			if (addHumanReadableReferences)
			{
				sb.AppendFormat(",%g", value);
			}
		}

		void AppendValue(double value) override
		{
			AppendIndents();
			sb.AppendFormat("D,0x%XLL", *(int64*)(float*)&value);
			if (addHumanReadableReferences)
			{
				sb.AppendFormat(",%llg", value);
			}
		}

		void AppendValue(bool value) override
		{
			AppendIndents();
			sb.AppendFormat("B,%s", value ? "1": "0");
		}

		void EnterMembers(cstr name, cstr type, cstr moduleName) override
		{
			sb.AppendFormat("%s,%s,%s\n", name, type, moduleName);
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