#include "dystopia.h"
#include <rococo.maths.h>
#include "human.types.h"
#include "dystopia.post.h"

#include <rococo.strings.h>

namespace 
{
	Rococo::Random::RandomMT rng;
}

namespace Dystopia
{
	namespace AI
	{
		namespace Low
		{
			void RunForward(ILevel& level, ID_ENTITY actorId, MetresPerSecond speed)
			{
				level.SetVelocity(actorId, speed * level.GetForwardDirection(actorId));
			}

			void RunInRandomDirection(ILevel& level, ID_ENTITY actorId, Seconds gameTime, Seconds runPeriod, MetresPerSecond runSpeed)
			{
				Degrees theta = Degrees{ (float)(Random::Next(rng) % 360) };
				level.SetHeading(actorId, theta);
				RunForward(level, actorId, runSpeed);
				level.SetNextAIUpdate(actorId, gameTime + runPeriod);
			}

			void Stop(ILevel& level, ID_ENTITY actorId)
			{
				level.SetVelocity(actorId, Vec3{ 0,0,0 });
			}

			float TurnTowardsTarget(ILevel& level, ID_ENTITY actorId, cr_vec3 targetPoint, Radians turnAnglePerSec, Seconds dt, float snapAlpha)
			{
				cr_vec3 actorPosition = level.GetPosition(actorId);
				Vec3 delta = targetPoint - actorPosition;
				delta.z = 0;

				Vec3 targetDirection;
				if (!TryNormalize(delta, targetDirection)) return -2.0f;

				Vec3 forward = level.GetForwardDirection(actorId);

				auto headingTheta = GetHeadingOfVector(forward.x, forward.y);
				auto directionTheta = GetHeadingOfVector(targetDirection.x, targetDirection.y);

				float alpha = Dot(forward, targetDirection);

				if (alpha > snapAlpha - dt) // Twiddle snapalpha a little, so systems with low frame rates snap rather than oscillate
				{
					level.SetHeading(actorId, directionTheta);
				}
				else
				{
					float dTheta = turnAnglePerSec * dt;

					Vec3 v = Cross(forward, targetDirection);

					// if direction is to left of heading then z component +ve, consider heading  = i, direction = j, then v = k
					if (v.z < 0)
					{
						dTheta *= -1;
					}

					float nextTheta = headingTheta + dTheta;

					level.SetHeading(actorId, Radians{ nextTheta });
				}

				return alpha;
			}

			RangedWeapon* GetRangedWeapon(ILevel& level, ID_ENTITY actorId, bool& isUsedTwoHanded, IItem** ppItem)
			{
				if (ppItem) *ppItem = nullptr;

				auto* inv = level.GetInventory(actorId);
				if (inv)
				{
					if (inv->HasPaperDoll())
					{
						IItem* leftItem = inv->GetItem(PAPER_DOLL_SLOT_LEFT_HAND);
						IItem* rightItem = inv->GetItem(PAPER_DOLL_SLOT_RIGHT_HAND);

						isUsedTwoHanded = leftItem == nullptr || rightItem == nullptr;

						auto* ranged = leftItem ? leftItem->GetRangedWeaponData() : nullptr;
						if (ranged)
						{
							if (ppItem) *ppItem = leftItem;
							return ranged;
						}

						ranged = rightItem ? rightItem->GetRangedWeaponData() : nullptr;
						if (ppItem) *ppItem = rightItem;
						return ranged;
					}
				}

				return nullptr;
			}

			void FireRangedWeapon(ILevel& level, Post::IPostbox* hintBox, ID_ENTITY creditToActorId, IItem* weapon, const ProjectileDef& def, Seconds now)
			{
				RangedWeapon* ranged = weapon->GetRangedWeaponData();

				if (!ranged->ammunitionIndex)
				{
					level.AddProjectile(def, now);
					return;
				}
				auto* magazineCache = weapon->GetContents();
				if (magazineCache)
				{
					auto* magazine = magazineCache->GetItem(0);
					if (magazine)
					{
						auto* ammo = magazine->GetAmmo();
						if (ammo)
						{
							if (ammo->count > 0)
							{
								ammo->count--;
								level.AddProjectile(def, now);

								if (ammo->count == 0)
								{
									auto* detachedAmmo = magazineCache->Swap(0, nullptr);
									detachedAmmo->Free();

									if (hintBox)
									{
										HintMessage3D hint;
										hint.duration = 2.5_seconds;
										hint.position = level.GetPosition(level.GetPlayerId());
										SafeFormat(hint.message, L"clip empty!");
										hintBox->PostForLater(hint, true);
									}
								}
							}
						}
					}
				}
			}
		}

		namespace Middle
		{

		}

		namespace High
		{

		}
	}
}