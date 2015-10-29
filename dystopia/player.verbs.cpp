#include "dystopia.h"
#include "dystopia.post.h"
#include "human.types.h"
#include "dystopia.ui.h"

namespace Dystopia
{
	namespace Verb
	{
		void Examine(const VerbExamine& target, Environment& e)
		{
			auto* inv = e.level.GetInventory(target.entityId);
			if (inv)
			{
				auto* item = inv->GetItem(target.inventoryIndex);
				if (item)
				{
					AutoFree<IStringBuilder> sb(CreateSafeStringBuilder(4096));

					auto* rwd = item->GetRangedWeaponData();
					if (rwd)
					{
						if (rwd->muzzleVelocity < 300.0f)
						{
							sb->AppendFormat(L"Ranged Weapon:\n\t\t\tMuzzle Velocity: %.0f m/s", rwd->muzzleVelocity);
						}
						else if (rwd->muzzleVelocity < 3000)
						{
							sb->AppendFormat(L"Ranged Weapon:\n\t\t\tMuzzle Velocity: MACH %.2f", rwd->muzzleVelocity / 330.0f);
						}
						else if (rwd->muzzleVelocity < 100000000)
						{
							sb->AppendFormat(L"Ranged Weapon:\n\t\t\tMuzzle Velocity: %.2f km/s", rwd->muzzleVelocity / 1000.0f);
						}
						else
						{
							sb->AppendFormat(L"Ranged Weapon:\n\t\t\tParticle beam");
						}
					}

					struct : IEventCallback<GuiEventArgs>
					{
						virtual void OnEvent(GuiEventArgs& arg) {}
					} unused;
					e.uiStack.PushTop(CreateDialogBox(e, unused, L"Examine...", *sb, L"", { 640, 480 }, 100, 50), ID_PANE_GENERIC_DIALOG_BOX);
				}
			}
		}
	}
}
