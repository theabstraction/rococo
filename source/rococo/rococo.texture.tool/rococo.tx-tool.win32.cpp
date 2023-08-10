#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <assets/assets.texture.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.time.h>
#include <vector>
#include <algorithm>
#include <rococo.win32.rendering.h>
#include <rococo.renderer.h>
#include <rococo.bakes.h>

namespace Rococo::Assets
{
	ROCOCO_API_IMPORT void RunTextureScript(cstr title, HINSTANCE hInstance, IO::IInstallation& installation, Rococo::Function<void(TextureBundle& bundle)> callback);
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

void OnError(cstr path, cstr message, int errorCode)
{
	printf("%s: %s code %d\n", path, message, errorCode);
	Throw(errorCode, "%s: %s", path, message);
}

void FindAllDotMipMapSubfolders(std::vector<WideFilePath>& dotMipMaps, const wchar_t* root)
{
	dotMipMaps.clear();

	struct ANON : IEventCallback<IO::FileItemData>
	{
		std::vector<WideFilePath>& paths;
		void OnEvent(FileItemData& item) override
		{
			auto ext = GetFileExtension(item.fullPath);

			if (item.isDirectory && Eq(ext, L".mipmaps"))
			{
				WideFilePath wFullPath;
				Format(wFullPath, L"%ls", item.fullPath);
				paths.push_back(wFullPath);
			}
		}

		ANON(std::vector<WideFilePath>& _paths) : paths(_paths)
		{

		}
	} onFile(dotMipMaps);

	IO::ForEachFileInDirectory(root, onFile, true);

	std::sort(dotMipMaps.begin(), dotMipMaps.end());
}

void SaveMipMapTexturesToDirectories(HINSTANCE hInstance, IInstallation& installation, cstr pingPath)
{
	auto onRun = [pingPath, &installation](TextureBundle& bundle)
	{
		bundle.textures.FileAssets().SetErrorHandler(OnError);

		cstr consoleTitle = "Rococo  Texture Tool - Console";

		SetConsoleTitleA(consoleTitle);

		HWND hConsole = nullptr;
		
		bundle.textures.SetEngineTextureArray(8192, 1, true, true);

		WideFilePath wPath;
		installation.ConvertPingPathToSysPath(pingPath, wPath);

		struct ANON : IEventCallback<IO::FileItemData>
		{
			std::vector<WideFilePath> paths;
			void OnEvent(FileItemData& item) override
			{
				auto ext = GetFileExtension(item.fullPath);

				if (!item.isDirectory && wcsstr(item.fullPath, L".mipmaps") == nullptr && (EqI(ext, L".tiff") || EqI(ext, L".tif") || EqI(ext, L".jpg") || EqI(ext, L".jpeg")))
				{
					WideFilePath wFullPath;
					Format(wFullPath, L"%ls", item.fullPath);
					paths.push_back(wFullPath);
				}
			}
		} onFile;

		IO::ForEachFileInDirectory(wPath, onFile, true);

		std::sort(onFile.paths.begin(), onFile.paths.end());

		printf("Identified %llu images under %ls\n", onFile.paths.size(), wPath.buf);

		bool isRunning = true;

		int count = 1;

		int statusCode = 0;
		char errMessage[1024] = { 0 };

		while (isRunning && IsWindowVisible(bundle.window.Window()))
		{
			UINT sleepMilliseconds = onFile.paths.empty() ? 1000 : 10;

			isRunning = ConsumeMessagesAndSleepEx(sleepMilliseconds);

			if (!hConsole)
			{
				hConsole = FindWindow(NULL, consoleTitle);
				if (hConsole)
				{
					BringWindowToTop(hConsole);
				}
			}

			bundle.textures.FileAssets().DeliverToThisThreadThisTick();

			if (*errMessage)
			{
				printf("%s\n", errMessage);
				*errMessage = 0;
			}

			if (!onFile.paths.empty())
			{
				WideFilePath wItemPath = onFile.paths.back();
				U8FilePath itemPingPath;
				installation.ConvertSysPathToPingPath(wItemPath, itemPingPath);

				printf("%d: Creating bitmap for %s\n", count++, itemPingPath.buf);

				auto texture = bundle.textures.Create32bitColourTextureAsset(itemPingPath);

				auto onLoad = [&statusCode, &errMessage, &wItemPath, &itemPingPath, &bundle](ITextureController& tx, uint32 mipMapLevel)
				{
					if (mipMapLevel == (uint32)-1)
					{
						char msg[1024];
						tx.AssetContainer().GetErrorAndStatusLength(statusCode, msg, 1024);
						SafeFormat(errMessage, "%s: texture load failed: %s", itemPingPath.buf, msg);
						return;
					}

					if (!tx.AttachToGPU())
					{
						char msg[1024];
						tx.AssetContainer().GetErrorAndStatusLength(statusCode, msg, 1024);
						SafeFormat(errMessage, "%s:failed to attach to the GPU: %s", itemPingPath.buf);
						return;
					}

					if (!tx.PushMipMapLevel(mipMapLevel))
					{
						char msg[1024];
						tx.AssetContainer().GetErrorAndStatusLength(statusCode, msg, 1024);
						SafeFormat(errMessage, "%s: failed to push mip level %u to the GPU.\n\t%s", itemPingPath.buf, mipMapLevel, msg);
					}

					tx.GenerateMipMaps(mipMapLevel);

					tx.FetchAllMipMapLevels();

					cstr imagePath = tx.AssetContainer().Path();

					cstr ext = GetFileExtension(imagePath);
	
					U8FilePath dirPath;
					Assign(dirPath, imagePath);
					if (!IO::TrySwapExtension(dirPath, nullptr, ".mipmaps"))
					{
						Throw(0, "%s: Failed to swap extension on %s", __FUNCTION__, dirPath.buf);
					}

					printf("Compiling %s -> %s\n", imagePath, dirPath.buf);
					U8FilePath sysDirPath;
					bundle.installation.ConvertPingPathToSysPath(dirPath, sysDirPath);

					IO::CreateDirectoryFolder(sysDirPath);

					auto onLevel = [&tx, &ext, &itemPingPath, &sysDirPath, &bundle](const MipMapLevelDesc& desc)
					{
						cstr newExt = desc.levelspan < 16 ? ".tif" : ext;

						U8FilePath targetFile;
						Format(targetFile, "%s\\%ux%u%s", sysDirPath.buf, desc.levelspan, desc.levelspan, newExt);

						if (desc.texelBuffer != nullptr)
						{
							Vec2i span { (int)desc.levelspan, (int)desc.levelspan };
							if (EqI(newExt, ".tif") || EqI(newExt, ".tiff"))
							{
								// We want to save the file as a tiff
								bundle.txManager.CompressTiff((const RGBAb*)desc.texelBuffer, span, targetFile);
							}
							else
							{
								// We want to save the file as a jpeg
								bundle.txManager.CompressJPeg((const RGBAb*)desc.texelBuffer, span, targetFile, 90);
							}

							// We can save the texels to a directory named according to the original ping path	
							printf("\t saving %s\n", targetFile.buf);
						}
					};

					tx.EnumerateMipMapLevels(onLevel);
					
				};

				texture->Tx().LoadTopMipMapLevel(onLoad);

				onFile.paths.pop_back();
				if (onFile.paths.empty())
				{
					puts("All paths are queued for processing\n");
				}
			}
		}
	};

	Rococo::Assets::RunTextureScript("Rococo Texture Tool", hInstance, installation, onRun);
}

Strings::CLI::CommandLineOption SWITCH_help { "-?"_fstring, "Print this help message" };
Strings::CLI::CommandLineOption SWITCH_genDirectories { "-D"_fstring, "Generate MipMip directories for each square JPG or TIFF with span an integer power of 2, up to 8192x8192" };
Strings::CLI::CommandLineOption KEYVALUE_targetPath { "-T:"_fstring, "Specified the target path for the tool. Defaults to '!textures/mip-mapped'" };
Strings::CLI::CommandLineOption SWITCH_genSexyTxBakes { "-B"_fstring, "Bake mip map directories into a compressed mip map archive file" };
Strings::CLI::CommandLineOption SWITCH_extractTxBakes { "-X"_fstring, "Extract mip map images to a directory next to the target" };

Strings::CLI::CommandLineOption allOptions[] =
{
	SWITCH_help,
	SWITCH_genDirectories,
	KEYVALUE_targetPath,
	SWITCH_genSexyTxBakes,
	SWITCH_extractTxBakes
};

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE /* hPrevInstance */, LPSTR lpCmdLine, int /* nCmdShow */)
{
	try
	{
		if (!AllocConsole())
		{
			Throw(GetLastError(), "No console!");
		}

		FILE* fDummy;
		freopen_s(&fDummy, "CONIN$", "r", stdin);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONOUT$", "w", stdout);

		OS::SetBreakPoints(OS::Flags::BreakFlag_All);
		AutoFree<IOSSupervisor> io = GetIOS();
		AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *io);

		U8FilePath pingPath;
		Strings::CLI::GetCommandLineArgument(KEYVALUE_targetPath.prefix, lpCmdLine, pingPath.buf, U8FilePath::CAPACITY, "!textures/mip-mapped");

		if (Strings::CLI::HasSwitch(SWITCH_help))
		{
			for (auto& opt : allOptions)
			{
				printf("%s: %s\n", opt.prefix.buffer, opt.helpString.buffer);
			}
		}

		if (Strings::CLI::HasSwitch(SWITCH_genDirectories))
		{
			SaveMipMapTexturesToDirectories(_hInstance, *installation, pingPath);
			return 0;
		}

		if (Strings::CLI::HasSwitch(SWITCH_genSexyTxBakes))
		{
			WideFilePath sysPath;
			installation->ConvertPingPathToSysPath(pingPath, sysPath);

			std::vector<WideFilePath> dotMipMaps;
			FindAllDotMipMapSubfolders(dotMipMaps, sysPath);

			for (auto& mipMapFile : dotMipMaps)
			{
				U8FilePath mipMapPath;
				Assign(mipMapPath, mipMapFile);
				Rococo::Bakes::BakeMipMappedTextureDirectory(mipMapPath);
			}

			return 0;
		}

		if (Strings::CLI::HasSwitch(SWITCH_extractTxBakes))
		{
			U8FilePath sysPath;
			installation->ConvertPingPathToSysPath(pingPath, sysPath);
			Rococo::Bakes::ExtractAsImageListFromBakedFile(sysPath);

			return 0;
		}
	}
	catch (IException& ex)
	{
		Windows::ShowErrorBox(Windows::NoParent(), ex, "rococo.texture.tool");
		return ex.ErrorCode();
	}

	return 0;
}