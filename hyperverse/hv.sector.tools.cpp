#include "hv.h"
#include <rococo.clock.h>

namespace HV
{
	HV::ICorridor* FactoryConstructHVCorridor(HV::ICorridor* _context)
	{
		return _context;
	}

	void AdvanceInTime(ISectors& sectors, const IUltraClock& clock)
	{
		if (clock.DT() > 0.0f)
		{
			for (auto s : sectors)
			{
				s->OnTick(clock);
			}
		}
	}

	HV::ISectorAIBuilder* FactoryConstructHVSectorAIBuilder(Cosmos* c, int32 sectorId)
	{
		for (auto s : c->sectors)
		{
			if (s->Id() == (uint32)sectorId)
			{
				return &s->GetSectorAIBuilder();
			}
		}

		Throw(0, "HV.SectorAIBuilder: no sector with id #%d", __FUNCTION__, sectorId);
	}

	float GetHeightAtPointInSector(cr_vec3 p, ISector& sector)
	{
		int32 index = sector.GetFloorTriangleIndexContainingPoint({ p.x, p.y });
		if (index >= 0)
		{
			auto* v = sector.FloorVertices().v;
			Triangle t;
			t.A = v[3 * index].position;
			t.B = v[3 * index + 1].position;
			t.C = v[3 * index + 2].position;

			float h;
			if (GetTriangleHeight(t, { p.x,p.y }, h))
			{
				return h;
			}
		}

		return 0.0f;
	}
}