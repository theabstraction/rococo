#ifndef ROCOCO_DX11_RENDERER_WIN32_H
#define ROCOCO_DX11_RENDERER_WIN32_H

namespace Rococo
{
	struct IAppFactory;
	struct IInstallation;
	void CALLBACK RendererMain(HANDLE hInstanceLock, IInstallation& installation, IAppFactory& factory);
}

#endif