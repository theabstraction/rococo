#ifndef MESHES_H
#define MESHES_H

#include <rococo.renderer.h>

namespace Rococo
{
	struct IMeshLoader
	{
		virtual ID_MESH LoadMesh(const wchar_t* resourcePath) = 0;
		virtual void Free() = 0;
	};

	IMeshLoader* CreateMeshLoader(IRenderer& renderer, IOS& os);
}

#endif