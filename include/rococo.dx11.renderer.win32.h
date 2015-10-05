#ifndef ROCOCO_DX11_RENDERER_WIN32_H
#define ROCOCO_DX11_RENDERER_WIN32_H

namespace Rococo
{
	struct IAppFactory;
	struct IOS;
	void CALLBACK RendererMain(HANDLE hInstanceLock, IOS& os, IAppFactory& factory);
}

#endif