#include "hv.h"

namespace HV
{
	ISector* GetFirstSectorContainingPoint(Vec2 a, ISectors& sectors)
	{
		for (auto* s : sectors)
		{
			auto& aabb = s->GetAABB();

			if (aabb.HoldsPoint(a))
			{
				int32 i = s->GetFloorTriangleIndexContainingPoint(a);
				if (i >= 0)
				{
					return s;
				}
			}
		}

		return nullptr;
	}

	HV::ISectorLayout* GetSector(int32 index, ISectors& sectors) 
	{
		if (index < 0 || index >= sectors.size())
		{
			if (sectors.empty())
				Throw(0, "No sectors");
			else
				Throw(0, "Invalid sector index %d. Range is [0,%d]", index, (int32)sectors.size() - 1);
		}
		return sectors[index].Layout();
	}

	HV::ISectorLayout* GetSectorById(int32 id, ISectors& sectors)
	{
		int32 index = id - 1;
		if (index < 0 || index >= sectors.size())
		{
			if (sectors.empty())
				Throw(0, "No sectors");
			else
				Throw(0, "Invalid sector id %d. Range is [1,%d]", index, (int32)sectors.size());
		}

		auto& s = sectors[index];
		if (s.Id() != id) Throw(0, "Sector id mismatch #%d to #%d", s.Id(), id);
		return sectors[index].Layout();
	}

	ISector* GetFirstSectorCrossingLine(Vec2 a, Vec2 b, ISectors& sectors)
	{
		for (auto* s : sectors)
		{
			if (s->DoesLineCrossSector(a, b))
			{
				return s;
			}
		}

		return nullptr;
	}

	SectorAndSegment GetFirstSectorWithVertex(Vec2 a, ISectors& sectors)
	{
		for (auto* s : sectors)
		{
			if (s->GetAABB().HoldsPoint(a))
			{
				int32 seg = s->GetPerimeterIndex(a);
				if (seg >= 0)
				{
					return{ s, seg };
				}
			}
		}

		return{ nullptr, -1 };
	}
}