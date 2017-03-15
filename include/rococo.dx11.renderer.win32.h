#ifndef ROCOCO_DX11_RENDERER_WIN32_H
#define ROCOCO_DX11_RENDERER_WIN32_H

namespace Rococo
{
	struct IAppFactory;
	struct IInstallation;
	void CALLBACK RendererMain(HANDLE hInstanceLock, IInstallation& installation, IAppFactory& factory);
   
   ROCOCOAPI IDX11Window
   {
      virtual void Free() = 0;
      virtual Windows::IWindow& Window() = 0;
      virtual IRenderer& Renderer() = 0;
      virtual void Run(HANDLE hInstanceLock, IApp& app);
   };

   IDX11Window* CreateDX11Window(IInstallation& installation);
}

#endif