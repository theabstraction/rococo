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
}