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
					Format(wFullPath, L"%ws", item.fullPath);
					paths.push_back(wFullPath);
				}
			}
		} onFile;

		IO::ForEachFileInDirectory(wPath, onFile, true);

		std::sort(onFile.paths.begin(), onFile.paths.end());

		printf("Identified %llu images under %ws\n", onFile.paths.size(), wPath.buf);

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
					Substring sansExt{ imagePath, ext };

					U8FilePath dirPath;
					Strings::SubstringToString(dirPath.buf, U8FilePath::CAPACITY, sansExt);
					StringCat(dirPath.buf, ".mipmaps", U8FilePath::CAPACITY);
					printf("Compiling %s -> %s\n", imagePath, dirPath.buf);

					WideFilePath wDirPathFile;
					bundle.installation.ConvertPingPathToSysPath(dirPath, wDirPathFile);

					IO::CreateDirectoryFolder(wDirPathFile);

					auto onLevel = [&wItemPath, &tx, &ext, &itemPingPath, &dirPath, &bundle](const MipMapLevelDesc& desc)
					{
						cstr newExt = desc.levelspan < 16 ? ".tif" : ext;

						U8FilePath targetFile;
						Format(targetFile, "%s/%ux%u%s", dirPath.buf, desc.levelspan, desc.levelspan, newExt);

						WideFilePath wTargetFile;
						bundle.installation.ConvertPingPathToSysPath(targetFile, wTargetFile);

						U8FilePath u8SysPath;
						Assign(u8SysPath, wTargetFile);

						if (desc.texelBuffer != nullptr)
						{
							Vec2i span { (int)desc.levelspan, (int)desc.levelspan };
							if (EqI(newExt, ".tif") || EqI(newExt, ".tiff"))
							{
								// We want to save the file as a tiff
								bundle.txManager.CompressTiff((const RGBAb*)desc.texelBuffer, span, u8SysPath);
							}
							else
							{
								// We want to save the file as a jpeg
								bundle.txManager.CompressJPeg((const RGBAb*)desc.texelBuffer, span, u8SysPath, 90);
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

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE /* hPrevInstance */, LPSTR /* lpCmdLine */, int /* nCmdShow */)
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

		SaveMipMapTexturesToDirectories(_hInstance, *installation, "!textures/mip-mapped");
	}
	catch (IException& ex)
	{
		Windows::ShowErrorBox(Windows::NoParent(), ex, "rococo.texture.tool");
	}

	return 0;
}