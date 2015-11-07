#ifndef MESHES_H
#define MESHES_H

#include <rococo.renderer.h>

namespace Dystopia
{
	using namespace Rococo;

	struct Environment;

	struct Triangle
	{
		Vec3 a;
		Vec3 b;
		Vec3 c;
	};

	struct IMeshLoader: public IMeshes
	{
		// Pass array of clockwise-visible triangles for the mesh. 
		// Diffuse.w is the texture blend parameter. Set to zero to use pure colours
		virtual void BuildMesh(const ObjectVertex* vertices, size_t vertexCount, ID_MESH id) = 0;
		virtual size_t ForEachPhysicsHull(ID_MESH id, IEnumerator<BoundingCube> &cb) = 0;
		virtual void Free() = 0;
		virtual ID_SYS_MESH GetRendererId(ID_MESH editorId) = 0;;
		virtual void UpdateMesh(const wchar_t* sysFilename) = 0;	
	};

	IMeshLoader* CreateMeshLoader(IInstallation& _installation, IRenderer& _renderer, ISourceCache& _sourceCache);
}

#endif