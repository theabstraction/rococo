#include "hv.h"
#include <rococo.clock.h>

namespace HV
{
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
}