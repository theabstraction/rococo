#include "dystopia.h"
#include <vector>
#include "rococo.maths.h"
#include "meshes.h"

namespace
{
	using namespace Dystopia;
	using namespace Rococo;

	Vec3 GetSpan(ID_MESH mesh, IMeshLoader& meshes)
	{
		struct : IEnumerator<BoundingCube>
		{
			BoundingCube cube;
			virtual void operator()(const BoundingCube& cube)
			{
				this->cube = cube;
			}
		} hull;
		meshes.ForEachPhysicsHull(mesh, hull);
		Vec3 span = hull.cube.topVertices.v.ne - hull.cube.bottomVertices.v.sw;
		return span;
	}
}

namespace Dystopia
{
	using namespace Rococo;

	void PopulateQuad(Environment& e, IVectorEnumerator<ID_MESH>& randomHouses, const GuiRectf& quad, cr_vec3 pos, IRandom& rng)
	{
		ID_MESH houseMeshId = randomHouses.begin()[rng() % randomHouses.size()];

		Vec3 span = GetSpan(houseMeshId, e.meshes);

		float xborder = Random::NextFloat(rng, 2.0f, 10.0f);
		float yborder = Random::NextFloat(rng, 2.0f, 10.0f);

		float XspanPlusSpace = span.x + xborder;
		float YspanPlusSpace = span.y + yborder;

		int32 xCount = int32(Width(quad) / XspanPlusSpace);
		int32 yCount = int32((quad.top - quad.bottom) / YspanPlusSpace);

		if (xCount <= 0 || yCount <= 0) return;

		float adjustedBorder_x = (Width(quad) - xCount * span.x) / xCount;
		float adjustedBorder_y = ((quad.top - quad.bottom) - yCount * span.y) / yCount;

		float x = quad.left + adjustedBorder_x * 0.5f + span.x * 0.5f;

		for (int32 i = 0; i < xCount; ++i)
		{
			float y = quad.bottom + adjustedBorder_y * 0.5f + span.y * 0.5f;;

			for (int32 j = 0; j < yCount; ++j)
			{
				if (i == 0 || i == xCount - 1 || j == 0 || j == yCount - 1)
				{
					auto id = e.level.Builder().AddSolid(pos + Vec3{ x,y,0 }, houseMeshId, SolidFlags_Obstacle);
				}
				y += span.y + adjustedBorder_y;
			}

			x += span.x + adjustedBorder_x;
		}
	}
}