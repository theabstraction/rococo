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

	struct IHullEnumerator
	{
		virtual void OnHull(const BoundingCube& cube) = 0;
	};

	struct IMeshLoader: public IMeshes
	{
		virtual void EvaluatePhysicsHull(ID_MESH id, IHullEnumerator& cb) = 0;
		virtual ID_MESH GetRendererId(int32 editorId) = 0;;
		virtual void Free() = 0;
		virtual void UpdateMesh(const wchar_t* sysFilename) = 0;
	};

	IMeshLoader* CreateMeshLoader(IInstallation& _installation, IRenderer& _renderer, ISourceCache& _sourceCache);
}

#endif