#ifndef ROCOCO_DX11_RENDERER_WIN32_H
#define ROCOCO_DX11_RENDERER_WIN32_H

namespace Rococo
{
	struct IAppFactory;
	struct IInstallation;
	struct IApp;

	void CALLBACK RendererMain(HANDLE hInstanceLock, IInstallation& installation, IAppFactory& factory);

	ROCOCOAPI IDX11Window
	{
	   virtual void Free() = 0;
	   virtual Windows::IWindow& Window() = 0;
	   virtual IRenderer& Renderer() = 0;
	   virtual void Run(HANDLE hInstanceLock, IApp& app);
	};

	ROCOCOAPI IDX11Factory
	{
		virtual IDX11Window* CreateDX11Window() = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX11Logger
	{
		virtual void Log(cstr message, ...) = 0;
		virtual void Free() = 0;
	};

	IDX11Factory* CreateDX11Factory(IInstallation& installation, IDX11Logger& logger);
	IDX11Logger* CreateStandardOutputLogger();
}

#endif