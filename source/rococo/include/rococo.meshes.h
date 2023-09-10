#pragma once

#include <rococo.types.h>

namespace Rococo
{
	// ID_MESH are artist defined indices. The artist chooses a number to associate with a mesh.
	// Rough convention:
	//    id  0          invalid
	//    ids 1          to 0x1FFFFFFF are defined in script files and level editors
	//    ids 0x20000000 to 0x20FFFFFF are procedurally generated paths, roads and rivers created in C++
	//    ids 0x21000000 t0 0x21FFFFFF are procedurally generated houses created in C++.
	//    ids 0x40000000 to 0x41000000 are gameplay generated meshes such as explosions created in C++.
	//	  ids 0x41000001 to 0x42000000 are skeletal animation meshes
	ROCOCO_ID(ID_MESH, int32, 0)

	// ID_SYS_MESH are renderer defined indices that are generated when meshes are loaded into the renderer
	ROCOCO_ID(ID_SYS_MESH, size_t,(size_t) - 1LL)
}

namespace Rococo::Entities
{
	struct IAnimation;
	struct ISkeleton;
	struct ISkeletons;
}

namespace Rococo::Graphics
{
	enum class ESoftBoxVertexPurpose : int32;

	struct SoftBoxVertex
	{
		Vec3 pos;
		Vec3 normal;
		Vec2 uv;
		ESoftBoxVertexPurpose purpose;
	};

	struct SoftBoxQuad
	{
		// top left
		SoftBoxVertex a;

		// top right
		SoftBoxVertex b;

		// bottom left
		SoftBoxVertex c;

		// bottom right
		SoftBoxVertex d;
	};

	struct SoftBoxTriangle
	{
		SoftBoxVertex a;
		SoftBoxVertex b;
		SoftBoxVertex c;
	};

	// Defines the top face of a box. The top face has constant z. It has four adjoining faces, the North face, the East face, the South face and the West face.
	// The North face faces (0 1 0) while the East face faces (1 0 0). The top face faces (0 0 1)
	struct SoftBoxTopSpec
	{
		// The total width of the box from West to East (width 0 0)
		float width;

		// The total breadth of the box from South to North (0 breadth 0)
		float breadth;

		// the radius of the cylinder from the top face to the North face
		float northRadius;

		// the radius of the cylinder from the top face to the South face
		float southRadius;

		// the radius of the cylinder from the top face to the East face
		float eastRadius;

		// the radius of the cylinder from the top face to the West face
		float westRadius;

		// The z value of the top face
		float ztop;

		// Number of radial divisions across the edge cylinder from the top face to the North face
		int32 northEdgeDivisions;

		// Number of radial divisions across the edge cylinder from the top face to the South face
		int32 southEdgeDivisions;

		// Number of radial divisions across the edge cylinder from the top face to the East face
		int32 eastEdgeDivisions;

		// Number of radial divisions across the edge cylinder from the top face to the West face
		int32 westEdgeDivisions;
	};

	struct RoundCornersShelfSpec
	{
		float zTop;
		float zBottom;
		float width;
		float breadth;
		float radiusNW;
		float radiusNE;
		float radiusSW;
		float radiusSE;
		int32 divisionsNW;
		int32 divisionsNE;
		int32 divisionsSW;
		int32 divisionsSE;
		boolean32 addBottom;
	};
}