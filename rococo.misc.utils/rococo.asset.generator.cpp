#include <rococo.asset.generator.h>
#include <rococo.strings.h>
#include <rococo.io.h>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::IO;
	using namespace Rococo::Assets;

	struct AssetGenerator_SexyContentFile : public IAssetGenerator
	{
		IInstallation& installation;

		AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(1024);

		AssetGenerator_SexyContentFile(IInstallation& _installation): installation(_installation)
		{
		}

		StringBuilder& GetReusableStringBuilder() override
		{
			return dsb->Builder();
		}

		void Generate(cstr filename, const fstring& stringSerialization) override
		{
			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(filename, sysPath);

			AutoFree<IBinaryArchive> writer = Rococo::IO::CreateNewBinaryFile(sysPath);
			writer->Write(1, stringSerialization.length, stringSerialization.buffer);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Assets
{
	IAssetGenerator* CreateAssetGenerator_SexyContentFile(IInstallation& installation)
	{
		return new ANON::AssetGenerator_SexyContentFile(installation);
	}
}