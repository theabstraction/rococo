#pragma once
#include <rococo.types.h>

namespace Rococo
{
	struct Triangle;
}

namespace Rococo::Graphics
{
	struct VertexTriangle;
	struct ObjectVertex;
	struct BoneWeights;
}

#include "..\..\rococo.mplat\code-gen\Rococo.Graphics.IMeshBuilder.sxh.h"

namespace Rococo
{
	struct IMathsVenue;
}

namespace Rococo::Graphics
{
	ROCOCO_INTERFACE IMeshBuilderSupervisor : public IMeshBuilder
	{
		virtual void Free() = 0;
		virtual void SaveCSV(cstr name, IExpandingBuffer& buffer) = 0;
		virtual bool TryGetByName(cstr name, ID_SYS_MESH& id, AABB& bounds) = 0;
		virtual IMathsVenue* Venue() = 0;
		virtual const fstring GetName(ID_SYS_MESH id) const = 0;
		virtual const VertexTriangle* GetTriangles(ID_SYS_MESH id, size_t& nTriangles) const = 0;
		virtual const Triangle* GetPhysicsHull(ID_SYS_MESH id, size_t& nTriangles) const = 0;
		virtual AABB Bounds(ID_SYS_MESH id) const = 0;
	};
}