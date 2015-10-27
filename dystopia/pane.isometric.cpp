#include "dystopia.h"
#include "rococo.renderer.h"
#include "human.types.h"

#include "rococo.ui.h"

#include <vector>

using namespace Rococo;
using namespace Dystopia;

namespace
{
	enum
	{
		MENU_ITEM_NONE,
		MENU_ITEM_PICKUP,
		MENU_ITEM_EXAMINE,
		MENU_ITEM_CANCEL
	};

	const Metres PICKUP_DISTANCE{ 1.5f };

	void PickupItem(ID_ENTITY collectorId, ILevel& level, ID_ENTITY itemId, Metres maxPickupRange)
	{
		auto h = level.GetHuman(collectorId);
		if (h.inventory)
		{
			Vec3 collectorPos;
			level.GetPosition(collectorId, collectorPos);

			EquipmentDesc desc;
			if (level.TryGetEquipment(itemId, desc))
			{
				if (IsInRange(collectorPos - desc.worldPosition, maxPickupRange))
				{
					IItem* item = desc.inventory->Swap(0, nullptr);
					IItem* oldItem = h.inventory->Swap(0, item);
					desc.inventory->Swap(0, oldItem);
					level.DeleteEquipment(itemId);
				}
			}
		}
	}

	void ExamineItem(ID_ENTITY id, ILevel& level, IGui& gui)
	{
		AutoFree<IStringBuilder> description(CreateSafeStringBuilder(1024));
		HumanSpec h = level.GetHuman(id);
		if (h.type != HumanType_None)
		{
			h.ai->Describe(*description);
		}
		else
		{
			EquipmentDesc desc;
			if (level.TryGetEquipment(id, desc))
			{
				auto* item = desc.inventory->GetItem(0);
				auto* ranged = item ? item->GetRangedWeaponData() : nullptr;
				if (ranged)
				{
					description->AppendFormat(L"Equipment:\n\tmuzzle velocity: %3.0f m/s", ranged->muzzleVelocity);
				}
			}
			else
			{
				description->AppendFormat(L"Misc solid object");
			}
		}

		gui.ShowDialogBox({ 640, 480 }, 100, 50, fstring{ L"Examine..." , -1 }, fstring{ *description, -1 }, fstring{ L">Continue=null", -1 });
	}

	struct XPlayerIntent: public IIntent
	{
		float forward;
		float left;
		float right;
		float backward;

		int32 fireCount;
		int32 activateCount;

		void Clear()
		{
			left = right = forward = backward = 0;
			fireCount = activateCount = 0;
		}

		XPlayerIntent()
		{
			Clear();
		}

		void OnEvent(ActionMap& map)
		{
			switch (map.type)
			{
			case ActionMapTypeForward:
				forward = map.isActive ? 1.0f : 0;
				break;
			case ActionMapTypeBackward:
				backward = map.isActive ? -1.0f : 0;
				break;
			case ActionMapTypeLeft:
				left = map.isActive ? -1.0f : 0;
				break;
			case ActionMapTypeRight:
				right = map.isActive ? 1.0f : 0;
				break;
			case ActionMapTypeFire:
				if (map.isActive) fireCount++;
				break;
			case ActionMapTypeSelect:
				if (map.isActive) activateCount++;
				break;
			}
		}

		virtual int32 GetFireCount() const
		{
			return fireCount;
		}

		virtual Vec2 GetImpulse() const
		{
			return Vec2{ left + right, forward + backward };
		}

		virtual void SetFireCount(int32 count)
		{
			fireCount = count;
		}

		virtual int32 PollActivateCount()
		{
			return activateCount;
		}
	};

	class PaneIsometric : public IUIControlPane, public IEventCallback<ContextMenuItem>, IEventCallback<ActionMap>
	{
		Environment& e;
		ILevelSupervisor& level;
		float gameTime;

		Vec3 groundCursorProjection;
		Matrix4x4 inverseWorldMatrixProj;

		GlobalState globalState;

		float globalScale;

		Degrees viewTheta;

		bool isRotateLocked;

		XPlayerIntent intent;

		void UpdateGroundCursorPosition(const Vec2i screenPosition)
		{
			Vec2 C = Graphics::PixelSpaceToScreenSpace(screenPosition, e.renderer);

			Vec4 xCursor = Vec4{ C.x, C.y, 0.0f, 1.0f };

			Vec4 cursorPos = inverseWorldMatrixProj * xCursor;
			Vec4 worldDir = inverseWorldMatrixProj * Vec4{ 0, 0, 1.0f, 0.0f };

			// We now have all the information to specify cursor probe. Point on probe are P(t) = cursorPos + t.worldDir.
			// We want intersection with earth z = 0
			// t0.worldDir.z + cursorPos.z = 0
			// t0 = -cursorPos.z / worldDir.z, giving P(t0), intersection with cursor and earth

			float t0 = -cursorPos.z / worldDir.z;
			groundCursorProjection = cursorPos + t0 * worldDir;

			level.SetGroundCursorPosition(groundCursorProjection);
		}

		void UpdateGlobalState()
		{
			float g = 0.04f * powf(1.25f, globalScale);

			auto playerId = level.GetPlayerId();

			Vec3 playerPosition;
			level.GetPosition(playerId, playerPosition);

			Degrees phi{ -45.0f };
			GetIsometricTransforms(globalState.worldMatrix, inverseWorldMatrixProj, globalState.worldMatrixAndProj, g, Graphics::GetAspectRatio(e.renderer), playerPosition, phi, viewTheta, Metres{ 100.0f });
		}
	public:
		PaneIsometric(Environment& _e, ILevelSupervisor& _level):
			e(_e), level(_level), gameTime(0.0f), globalScale(4.0f), viewTheta { 45.0f }, isRotateLocked(true)
		{

		}

		virtual void Free()
		{
			delete this;
		}

		virtual IIntent* PlayerIntent()
		{
			return &intent;
		}

		virtual PaneModality OnTimestep(const TimestepEvent& clock)
		{
			float dt = clock.deltaTicks / (float)clock.hz;
			if (dt < 0.0f) dt = 0.0f;
			if (dt > 0.1f) dt = 0.1f;

			if (dt > 0)
			{
				gameTime += dt;
				AdvanceTimestepEvent ate{ clock, dt, gameTime };
				e.postbox.SendDirect(ate);
				level.UpdateObjects(gameTime, dt);

				auto id = e.uiStack.Top().id;
				if (id != ID_PANE_GUI_WORLD_INTERFACE)
				{
					intent.Clear();
				}
			}

			GuiMetrics metrics;
			e.renderer.GetGuiMetrics(metrics);

			UpdateGroundCursorPosition(metrics.cursorPosition);

			return PaneModality_Modal;
		}

		virtual void OnEvent(ActionMap& map)
		{
			switch (map.type)
			{
			case ActionMapTypeScale:
				{
					if (map.vector.y > 0)
					{
						globalScale += 0.5f;
						globalScale = min(globalScale, 10.0f);
					}
					else if (map.vector.y < 0)
					{
						globalScale -= 0.5f;
						globalScale = max(globalScale, 3.5f);
					}
				}break;
			case ActionMapTypeRotate:
				isRotateLocked = !map.isActive;
				break;
			case ActionMapTypeInventory:
				if (map.isActive) e.uiStack.PushTop(ID_PANE_INVENTORY_SELF);
				break;
			default:
				intent.OnEvent(map);
				break;
			}
		}

		void ActivateContextMenu(cr_vec3 origin, const Vec2i topLeftPosition)
		{
			auto id = level.SelectedId();
			if (id)
			{
				std::vector<ContextMenuItem> menu;

				EquipmentDesc desc;
				if (level.TryGetEquipment(id, desc))
				{
					bool isInRange = IsInRange(origin - desc.worldPosition, PICKUP_DISTANCE);
					menu.push_back({ L"Pickup", MENU_ITEM_PICKUP, id, isInRange });
				}

				menu.push_back({ L"Examine", MENU_ITEM_EXAMINE, id, true });
				menu.push_back({ L"Cancel",  MENU_ITEM_CANCEL,  id, true });
				menu.push_back({ nullptr, 0, 0 });

				e.uiStack.PushTop(CreateContextMenu(e, topLeftPosition, &menu[0], *this), ID_PANE_GENERIC_CONTEXT_MENU);
			}
		}

		virtual void OnEvent(ContextMenuItem& item)
		{
			ID_ENTITY id = item.context;

			switch (item.commandId)
			{
			case MENU_ITEM_PICKUP:
				PickupItem(level.GetPlayerId(), level, id, PICKUP_DISTANCE);
				break;
			case MENU_ITEM_EXAMINE:
				ExamineItem(id, level, e.gui);
				break;
			}
		}

		virtual PaneModality OnKeyboardEvent(const KeyboardEvent& key)
		{
			e.controls.MapKeyboardEvent(key, *this);
			return PaneModality_Modal;
		}

		virtual PaneModality OnMouseEvent(const MouseEvent& me)
		{
			if (me.dx != 0 && !isRotateLocked)
			{
				float newTheta = fmodf(viewTheta + 0.25f * me.dx, 360.0f);
				viewTheta = { newTheta };
			}
			e.controls.MapMouseEvent(me, *this);
			return PaneModality_Modal;
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			UpdateGlobalState();

			rc.SetGlobalState(globalState);
			level.RenderObjects(rc);
		}

		virtual void RenderGui(IGuiRenderContext& grc)
		{
		}
	};
}

namespace Dystopia
{
	IUIControlPane* CreatePaneIsometric(Environment& e, ILevelSupervisor& level)
	{
		return new PaneIsometric(e, level);
	}
}