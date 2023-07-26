#include <rococo.os.win32.h>
#include <assets/assets.texture.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.time.h>
#include <vector>
#include <algorithm>

namespace Rococo::Assets
{
	ROCOCO_API_IMPORT void RunTextureScript(HINSTANCE hInstance, IO::IInstallation& installation, Rococo::Function<void(ITextureAssetFactory& textures)> callback);
}

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Assets;

bool ConsumeMessagesAndSleepEx(int sleepMilliseconds)
{
	MsgWaitForMultipleObjectsEx(0, NULL, sleepMilliseconds, QS_ALLINPUT, MWMO_ALERTABLE);

	MSG msg;
	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

void SaveMipMapTexturesToDirectories(HINSTANCE hInstance, IInstallation& installation, cstr pingPath)
{
	auto onRun = [pingPath, &installation](ITextureAssetFactory& textures)
	{
		textures.SetEngineTextureArray(8192, 1);

		WideFilePath wPath;
		installation.ConvertPingPathToSysPath(pingPath, wPath);

		struct ANON : IEventCallback<IO::FileItemData>
		{
			std::vector<WideFilePath> paths;
			void OnEvent(FileItemData& item) override
			{
				auto ext = GetFileExtension(item.fullPath);

				if (!item.isDirectory && (EqI(ext, L".tiff") || EqI(ext, L".tif") || EqI(ext, L".jpg") || EqI(ext, L".jpeg")))
				{
					WideFilePath wFullPath;
					Format(wFullPath, L"%ws", item.fullPath);
					paths.push_back(wFullPath);
				}
			}
		} onFile;

		IO::ForEachFileInDirectory(wPath, onFile, true);

		std::sort(onFile.paths.begin(), onFile.paths.end());

		bool isRunning = true;

		while (isRunning)
		{
			UINT sleepMilliseconds = onFile.paths.empty() ? 1000 : 10;

			isRunning = ConsumeMessagesAndSleepEx(sleepMilliseconds);

			textures.FileAssets().DeliverToThisThreadThisTick();

			if (!onFile.paths.empty())
			{
				WideFilePath wPath = onFile.paths.back();
				U8FilePath itemPingPath;
				installation.ConvertSysPathToPingPath(wPath, itemPingPath);

				auto texture = textures.Create32bitColourTextureAsset(itemPingPath, NoTextureCallback);

				auto onLoad = [&wPath, &itemPingPath](ITextureController& tx, uint32 mipMapLevel)
				{
					if (!tx.AttachToGPU())
					{
						Throw(0, "Failed to attach to the GPU");
					}

					if (!tx.PushMipMapLevel(mipMapLevel))
					{
						Throw(0, "Failed to push the image to the GPU");
					}

					tx.GenerateMipMaps(mipMapLevel);

					auto onLevel = [&wPath, &tx, &itemPingPath](const MipMapLevelDesc& desc)
					{
						if (desc.texelBuffer != nullptr)
						{
							// We can save the texels to a directory named according to the original ping path
							cstr imagePath = tx.AssetContainer().Path();
							cstr ext = GetFileExtension(imagePath);
							Substring sansExt{ imagePath, ext };
							U8FilePath dirPath;
							Strings::SubstringToString(dirPath.buf, U8FilePath::CAPACITY, sansExt);
							StringCat(dirPath.buf, ".mipmaps", U8FilePath::CAPACITY);
							puts(dirPath.buf);
						}
					};

					tx.EnumerateMipMapLevels(onLevel);
					
				};

				texture->Tx().LoadTopMipMapLevel(onLoad);

				onFile.paths.pop_back();
			}
		}
	};

	Rococo::Assets::RunTextureScript(hInstance, installation, onRun);
}

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE /* hPrevInstance */, LPSTR /* lpCmdLine */, int /* nCmdShow */)
{
	AutoFree<IOSSupervisor> io = GetIOS();
	AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *io);

	SaveMipMapTexturesToDirectories(_hInstance, *installation, "!textures/mipMapped");

	return 0;
}