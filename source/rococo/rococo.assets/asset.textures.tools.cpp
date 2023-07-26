#include <rococo.os.win32.h>
# define ROCOCO_ASSETS_API ROCOCO_API_EXPORT
#include <assets/assets.texture.h>
#include <rococo.win32.rendering.h>
#include <rococo.renderer.h>

using namespace Rococo::Assets;

namespace Rococo::Assets
{
	void GetWindowSpec(HINSTANCE hInstance, WindowSpec& ws)
	{
		ws.exStyle = 0;
		ws.style = WS_OVERLAPPEDWINDOW;
		ws.hInstance = hInstance;
		ws.hParentWnd = nullptr;
		ws.messageSink = nullptr;
		ws.minSpan = { 1024, 640 };
		ws.X = CW_USEDEFAULT;
		ws.Y = CW_USEDEFAULT;
		ws.Width = 768;
		ws.Height = 432;
	}

	ROCOCO_API_EXPORT void RunTextureScript(HINSTANCE hInstance, IO::IInstallation& installation, Rococo::Function<void(ITextureAssetFactory& textures)> callback)
	{
		AutoFree<IGraphicsLogger> logger = CreateStandardOutputLogger();

		FactorySpec defaultSpec = { 0 };
		AutoFree<IGraphicsWindowFactory> windowFactory = CreateGraphicsWindowFactory(installation, *logger, defaultSpec);

		WindowSpec ws;
		AutoFree<IGraphicsWindow> graphicsWindow = windowFactory->CreateGraphicsWindow(ws, true);

		graphicsWindow->MakeRenderTarget();
		auto& engineTextures = graphicsWindow->Renderer().Textures();

		AutoFree<IAssetManagerSupervisor> assetManager = CreateAssetManager();
		AutoFree<IFileAssetFactorySupervisor> files = CreateFileAssetFactory(*assetManager, installation);
		AutoFree<ITextureAssetFactorySupervisor> textures = CreateTextureAssetFactory(engineTextures, *files);

		callback.Invoke(*textures);
	}

	// Copy and paste this code into your Win32 app to run a texture script. Ensure you link to the asset libs and #include <assets/assets.texture.h>
	//namespace Rococo::Assets
	//{
	//	ROCOCO_API_IMPORT void RunTextureScript(HINSTANCE hInstance, IO::IInstallation& installation, Rococo::Function<void(ITextureAssetFactory& textures)> callback);
	//}
}