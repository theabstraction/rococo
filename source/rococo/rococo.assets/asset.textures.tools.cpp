#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
# define ROCOCO_ASSETS_API ROCOCO_API_EXPORT
#include <assets/assets.texture.h>
#include <rococo.win32.rendering.h>
#include <rococo.renderer.h>
#include <rococo.window.h>

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
		ws.minSpan = { 512, 128 };
		ws.X = CW_USEDEFAULT;
		ws.Y = CW_USEDEFAULT;
		ws.Width = 512;
		ws.Height = 128;
	}

	struct ShaderOptions : Rococo::Graphics::IShaderOptions
	{
		size_t NumberOfOptions() const override
		{
			return 0;
		}

		void GetOption(size_t index, OUT cstr& interfaceName, OUT cstr& className) override
		{
			UNUSED(index);
			UNUSED(interfaceName);
			UNUSED(className);
		}
	};

	ROCOCO_API_EXPORT void RunTextureScript(cstr title, HINSTANCE hInstance, IO::IInstallation& installation, Rococo::Function<void(TextureBundle& bundle)> callback)
	{
		AutoFree<IGraphicsLogger> logger = CreateStandardOutputLogger();

		ShaderOptions shaderOptions;

		FactorySpec defaultSpec = { 0 };
		AutoFree<IGraphicsWindowFactory> windowFactory = CreateGraphicsWindowFactory(installation, *logger, defaultSpec, shaderOptions);

		struct NullHandler : Rococo::Graphics::IWindowEventHandler
		{
			void OnPostResize(bool isFullscreen, Vec2i span) override
			{
				UNUSED(isFullscreen);
				UNUSED(span);
			}
		} evHandler;

		WindowSpec ws;
		GetWindowSpec(hInstance, ws);
		AutoFree<IGraphicsWindow> graphicsWindow = windowFactory->CreateGraphicsWindow(evHandler, ws, true);

		graphicsWindow->MakeRenderTarget();
		auto& engineTextures = graphicsWindow->Renderer().Textures();

		SetWindowTextA(graphicsWindow->Window(), title);

		AutoFree<IAssetManagerSupervisor> assetManager = CreateAssetManager();
		AutoFree<IFileAssetFactorySupervisor> files = CreateFileAssetFactory(*assetManager, installation);
		AutoFree<ITextureAssetFactorySupervisor> textures = CreateTextureAssetFactory(engineTextures, *files);

		TextureBundle bundle{ installation, *logger, *windowFactory, *graphicsWindow, engineTextures, *assetManager, *files, *textures };

		callback.Invoke(bundle);
	}

	// Copy and paste this code into your Win32 app to run a texture script. Ensure you link to the asset libs and #include <assets/assets.texture.h>
	//namespace Rococo::Assets
	//{
	//	ROCOCO_API_IMPORT void RunTextureScript(HINSTANCE hInstance, IO::IInstallation& installation, Rococo::Function<void(TextureBundle& bundle)> callback);
	//}
}