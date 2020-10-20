#include "rococo.mplat.h"
#include "mplat.landscapes.h"

using namespace Rococo;
using namespace Rococo::Graphics;

namespace
{
	class LandscapeTesselator : public ILandscapeTesselatorSupervisor
	{
	public:
		void AddQuadField(int32 base2exponentDivisions, Metres span, Metres maxAltitude)
		{

		}

		void Clear()
		{

		}

		ID_MESH/* id */ CommitToMesh(const fstring& meshName)
		{
			return ID_MESH::Invalid();
		}

		void Generate()
		{

		}

		void GetBounds(Vec3& minPoint, Vec3& maxPoint)
		{

		}

		void RaiseMountain(const Vec3& atPosition, Metres deltaHeight, Metres spread)
		{

		}

		void SetHeights(const Vec2i& p0, const Vec2i& p1, Metres height)
		{

		}

		void SetSeed(int64 seedNumber)
		{

		}

		void TranslateEachCell(const Vec3& delta)
		{

		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Graphics
{
	ILandscapeTesselatorSupervisor* CreateLandscapeTesselator(IMeshBuilderSupervisor& meshes)
	{
		return new LandscapeTesselator();
	}
}