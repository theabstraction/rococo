#include "dystopia.h"
#include "rpg.rules.h"

namespace
{
	using namespace Dystopia;

	struct CapBind
	{
		StatValue statcap;
		const wchar_t* title;
		const wchar_t* rewardText;
	};

	CapBind kudosCaps[]
	{
		{3,			L"irrelevant",		L"Having lived the decadent life, your gentle disposition and effete mannerism can convince nobody of anything. "
										"You need to draw upon the fighting spirit, and you do that...by fighting..." },
		{10,		L"murderer",		L"You have tasted the fighting spirit. With the current state of the country, you can always pass off your first kills as self-defence." },
		{25,		L"spree killer",	L"'It was self-defence' is not going to wash. But 'I did it for Queen and Country' may just about rub." },
		{100,		L"serial killer",	L"You are steeped in blood, having killed dozens. How to clear your name when the emergency is over is going to be a tough one." },
		{250,		L"vigilante",		L"Kill one bad man and the world scolds you as the murderer. Kill a hundred and you become a folk hero."
										"Still, sharing History's stage with Billy the Kid is not the ultimate goal." },
		{1000,		L"Hero vigilante",	L"You have progressed from rogue folk hero to a public icon. All that fawning from the low life crowd is over. Now the great scheme can begin." },
		{2500,		L"Super vigilante", L"You have earned your place in history. The lax prancing of former governments has become as socially acceptable as child sacrifice." },
		{10000,		L"Revolutionary",	L"Your deeds have changed the vocabulary of public discourse. No more is vigilante a dirty word." },
		{25000,		L"Leader",			L"You have the airs of leadership - the stuff of reform and rebelliion" },
		{45000,		L"Statesman",		L"Your are the bearer of a new philosophy - those that stand against it shall be deemed degnerate and surplus to the ideal." },
		{85000,		L"Legendary",		L"Yours is the politics of the aeon. The ideas you espouse may dim, but never will they die." },
		{0x7FFFFFFF,L"Redeemer",		L"Ten thousand years may go by, but the redeemer will never be forgotten." },
		{ 0,  L"Hacker",	L"No mortal man could have done what you have done...unless you cheated." },
	};
}

namespace Dystopia {
	namespace RPG
	{
		int32 GetKudosLevel(Stat kudos)
		{
			for (int i = 0; kudosCaps[i].statcap != 0x7FFFFFFF; ++i)
			{
				if (kudos.cap == kudosCaps[i].statcap)
				{
					return i;
				}
			}

			return 0;
		}

		const wchar_t* GetTitle(Stat kudos)
		{
			return kudosCaps[GetKudosLevel(kudos)].title;
		}

		StatValue GetNextCap(Stat kudos)
		{
			return kudosCaps[GetKudosLevel(kudos) + 1].statcap;
		}

		const wchar_t* GetKudosRewardText(Stat kudos)
		{
			return kudosCaps[GetKudosLevel(kudos)].rewardText;
		}

		StatValue GetFirstCap()
		{
			return kudosCaps[0].statcap;
		}

		void EarnKudos(Stat& kudosStat, StatValue delta, ID_ENTITY actorId, Environment& e)
		{
			if (!delta) return;

			kudosStat.current += delta;

			int32 levelsGained = 0;

			while (kudosStat.current >= kudosStat.cap)
			{
				levelsGained++;
 				kudosStat.cap = GetNextCap(kudosStat);
			}

			if (actorId == e.level.GetPlayerId())
			{
				cr_vec3 playerPos = e.level.GetPosition(actorId);

				if (levelsGained > 0)
				{
					fstring rewardText = to_fstring(GetKudosRewardText(kudosStat));
					e.gui.ShowDialogBox({ 640,480 }, 150, 50, L"Life improves!"_fstring, rewardText, L"Continue=0"_fstring);
				}
				else if (delta > 0)
				{
					e.gui.Add3DHint(playerPos, L"Kudos!"_fstring, 1.0_seconds);
				}
				else
				{
					e.gui.Add3DHint(playerPos, L"Shame!"_fstring, 1.0_seconds);
				}	
			}
		}
	}
}