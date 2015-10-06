#ifndef MESHES_H
#define MESHES_H

#include <rococo.renderer.h>

namespace Rococo
{
	struct IMeshLoader
	{
		virtual ID_MESH GetRendererId(int32 editorId) = 0;
		virtual void LoadMeshes(const wchar_t* resourcePath, bool isReloading) = 0;
		virtual void Free() = 0;
		virtual void UpdateMesh(const wchar_t* sysFilename) = 0;
	};

	IMeshLoader* CreateMeshLoader(IRenderer& renderer, IInstallation& installation);
}

#endif