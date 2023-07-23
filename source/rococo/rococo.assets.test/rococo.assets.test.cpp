#include <assets/assets.files.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <stdio.h>

using namespace Rococo;
using namespace Rococo::Assets;

void TestFactory(IFileAssetFactory& factory)
{
	bool hasLoadCompleted = false;
	auto onLoad = [&hasLoadCompleted](IFileAsset& asset)
	{
		printf("Asset %s loaded. File length %llu bytes\n", asset.Path(), asset.RawData().nBytes);
		hasLoadCompleted = true;
	};

	AssetRef<IFileAsset> ref = factory.CreateFileAsset("!scripts/native/Sys.Maths.sxy", onLoad);

	int waitCount = 0;
	while (!hasLoadCompleted && waitCount < 10)
	{
		factory.DeliverToThisThreadThisTick();
		OS::SleepUntilAsync(100);
		waitCount++;
	}

	if (!hasLoadCompleted)
	{
		Throw(0, "TestFactory failed. Timeout");
	}
}

int main(int argc, char* argv)
{
	UNUSED(argc);
	UNUSED(argv);

	OS::SetBreakPoints(OS::BreakFlag_All);

	AssetRef<IAsset> asset;

	try
	{
		AutoFree<IO::IOSSupervisor> io = IO::GetIOS();
		AutoFree<IO::IInstallationSupervisor> installation = IO::CreateInstallation(L"content.indicator.txt", *io);
		AutoFree<IAssetManagerSupervisor> assetManager = Rococo::Assets::CreateAssetManager();
		AutoFree<IFileAssetFactorySupervisor> assetFactory = Rococo::Assets::CreateFileAssetFactory(*assetManager, *installation);

		TestFactory(*assetFactory);

		printf("Finishing tests...\n");
	}
	catch (IException& ex)
	{
		fprintf(stderr, "Code %d: %s", ex.ErrorCode(), ex.Message());
	}

	printf("Tests have completed\n");

	return 0;
}