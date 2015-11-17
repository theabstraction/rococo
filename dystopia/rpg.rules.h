#ifndef RPG_RULES_H
#define RPG_RULES_H

#include "human.types.h"

namespace Dystopia {
	namespace RPG
	{
		using namespace Rococo;

		void EarnKudos(Stat& kudosStat, StatValue delta, ID_ENTITY actorId, Environment& e);

		StatValue GetFirstCap();
		int32 GetKudosLevel(Stat kudos);
		StatValue GetNextCap(Stat kudos);

		const wchar_t* GetTitle(Stat kudos);
		const wchar_t* GetKudosRewardText(Stat kudos);
	}
}

#endif // RPG_RULES_H
