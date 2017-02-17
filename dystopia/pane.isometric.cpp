#include "dystopia.h"
#include "rococo.renderer.h"
#include "human.types.h"

#include "rococo.ui.h"
#include "dystopia.post.h"
#include "dystopia.ui.h"
#include "rococo.strings.h"

#include <vector>

using namespace Rococo;
using namespace Dystopia;

namespace
{
	const Metres PICKUP_DISTANCE{ 1.5f };

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

	Vec3 ComputeGroundZeroPosition(const Vec2i screenPosition, const Matrix4x4& invWorldProj, IRenderer& renderer)
	{
		Vec2 C = Graphics::PixelSpaceToScreenSpace(screenPosition, renderer);

		Vec4 xCursor = Vec4{ C.x, C.y, 0.0f, 1.0f };

		Vec4 cursorPos = invWorldProj * xCursor;
		Vec4 worldDir = invWorldProj * Vec4{ 0, 0, 1.0f, 0.0f };

		// We now have all the information to specify cursor probe. Point on probe are P(t) = cursorPos + t.worldDir.
		// We want intersection with earth z = 0
		// t0.worldDir.z + cursorPos.z = 0
		// t0 = -cursorPos.z / worldDir.z, giving P(t0), intersection with cursor and earth

		float t0 = -cursorPos.z / worldDir.z;
		return cursorPos + t0 * worldDir;
	}

	class PaneIsometric : public IUIControlPane, IEventCallback<ActionMap>, public Post::IRecipient
	{
		Environment& e;

		Vec3 groundCursorProjection;
		Matrix4x4 inverseWorldMatrixProj;

		GlobalState globalState;

		float globalScale;

		Degrees viewTheta;

		bool isRotateLocked;

		XPlayerIntent intent;

		std::vector<HintMessage3D> hints3D;

		void UpdateGroundCursorPosition(const Vec2i screenPosition)
		{
			Vec3 gzero = ComputeGroundZeroPosition(screenPosition, inverseWorldMatrixProj, e.renderer);
			e.level.SetGroundCursorPosition(gzero);
		}

		void UpdateGlobalState()
		{
			float g = 0.04f * powf(1.25f, globalScale);

         auto viewRadius = Metres{ 2.0f * sqrtf(2.0f) / g };
         e.level.SetRenderRadius(viewRadius);

			auto playerId = e.level.GetPlayerId();

			cr_vec3 playerPosition=  e.level.GetPosition(playerId);

			auto phi = -45.0_degrees;
			GetIsometricTransforms(globalState.worldMatrix, inverseWorldMatrixProj, globalState.worldMatrixAndProj, g, Graphics::GetAspectRatio(e.renderer), playerPosition, phi, viewTheta, Metres{ 100.0f });

			Vec3 sunlight = Normalize(Vec3{ -1.0f, -2.0f, -1.0f });
			globalState.sunlightDirection = Vec4::FromVec3(sunlight, 0.0f);
		}
	public:
		PaneIsometric(Environment& _e):
			e(_e), globalScale(4.0f), viewTheta { 45.0_degrees }, isRotateLocked(true)
		{
			e.postbox.Subscribe<HintMessage3D>(this);
			e.postbox.Subscribe<VerbDropAtCursor>(this);
			e.postbox.Subscribe<UIStackPaneChange>(this);
		}

		virtual void Free()
		{
			delete this;
		}

		virtual IIntent* PlayerIntent()
		{
			return &intent;
		}

		virtual void OnPost(const Mail& mail)
		{
			auto* hint3D = Post::InterpretAs<HintMessage3D>(mail);
			if (hint3D)
			{
				hints3D.push_back(*hint3D);
			}

			auto* dropAt = Post::InterpretAs<VerbDropAtCursor>(mail);
			if (dropAt)
			{
				auto* inv = e.level.GetInventory(dropAt->entityId);
				if (inv)
				{
					auto* item = inv->Swap(dropAt->inventoryIndex, nullptr);
					Vec3 gzero = ComputeGroundZeroPosition(dropAt->cursorPosition, inverseWorldMatrixProj, e.renderer);

					e.level.CreateStash(item, gzero);
				}
			}

			auto* uiChange = Post::InterpretAs<UIStackPaneChange>(mail);
			if (uiChange)
			{
				isRotateLocked = true;
				intent.Clear();
			}
		}

		virtual Relay OnTimestep(const TimestepEvent& clock)
		{
			float dt = clock.deltaTicks / (float)clock.hz;
			if (dt < 0.0f) dt = 0.0f;
			if (dt > 0.1f) dt = 0.1f;

			if (dt > 0)
			{
				e.level.UpdateGameTime(dt);

				auto id = e.uiStack.Top().id;
				if (id != ID_PANE_STATS)
				{
					intent.Clear();
				}

				auto i = hints3D.begin();
				while (i != hints3D.end())
				{
					i->duration -= dt;

					if (i->duration > 0)
					{
						i++;
					}
					else
					{
						i = hints3D.erase(i);
					}
				}
			}

			GuiMetrics metrics;
			e.renderer.GetGuiMetrics(metrics);

			UpdateGroundCursorPosition(metrics.cursorPosition);

			return Relay_None;
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
						globalScale = min(globalScale, 20.0f);
					}
					else if (map.vector.y < 0)
					{
						globalScale -= 0.5f;
						globalScale = max(globalScale, -20.0f);
					}
				}break;
			case ActionMapTypeStats:
				if (map.isActive) e.uiStack.PushTop(ID_PANE_PERSONAL_INFO);
				break;
         case ActionMapTypeCV:
            if (map.isActive) e.uiStack.PushTop(ID_PANE_CV);
            break;
			case ActionMapTypeRotate:
				isRotateLocked = !map.isActive;
				break;
			case ActionMapTypeInventory:
				if (map.isActive) e.uiStack.PushTop(ID_PANE_INVENTORY_SELF);
				break;
			case ActionMapTypeSelect:
				if (map.isActive && e.level.SelectedId())
				{
					SelectItemOnGround sitog{ e.level.SelectedId() };
					e.postbox.PostForLater(sitog, false);	
				}
				break;
			case ActionMapTypeJournal:
				if (map.isActive) e.uiStack.PushTop(ID_PANE_JOURNAL);
				break;
			default:
				intent.OnEvent(map);
				break;
			}
		}

		virtual Relay OnKeyboardEvent(const KeyboardEvent& key)
		{
			e.controls.MapKeyboardEvent(key, *this);
			return Relay_None;
		}

		virtual Relay OnMouseEvent(const MouseEvent& me)
		{
			if (me.dx != 0 && !isRotateLocked)
			{
				float newTheta = fmodf(viewTheta + 0.25f * me.dx, 360.0_degrees);
				viewTheta = { newTheta };
			}
			e.controls.MapMouseEvent(me, *this);
			
			e.level.SetHeading(e.level.GetPlayerId(), -viewTheta);

			return Relay_None;
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			UpdateGlobalState();

			rc.SetGlobalState(globalState);
			e.level.RenderObjects(rc);
		}

		void RenderHint(IGuiRenderContext& grc, const HintMessage3D& hint)
		{
			Vec4 screenPos = globalState.worldMatrixAndProj * Vec4::FromVec3(hint.position, 1.0f);

			GuiMetrics metrics;
			e.renderer.GetGuiMetrics(metrics);

			int32 x = (int32) (0.5f * metrics.screenSpan.x * (screenPos.x + 1.0f));
			int32 y = (int32) (0.5f * metrics.screenSpan.y * (1.0f - screenPos.y));

			float fAlpha = hint.duration > 1.0f ? 1.0f : hint.duration;
			RGBAb colour(255, 255, 255, (uint8) (fAlpha * 255.0f));

			Graphics::StackSpaceGraphics ss1;
			auto& horzTextLight = Graphics::CreateHorizontalCentredText(ss1, 3, hint.message, colour);

			Graphics::StackSpaceGraphics ss2;
			auto& horzTextShadow = Graphics::CreateHorizontalCentredText(ss2, 3, hint.message, RGBAb(0,0,0, (uint8)(fAlpha * 255.0f)));
			Vec2i span = grc.EvalSpan({ 0,0 }, horzTextLight);

			grc.RenderText({ x-2,y-2 }, horzTextShadow);
			grc.RenderText({ x,y }, horzTextLight);
		}

		virtual void RenderGui(IGuiRenderContext& grc)
		{
			for (auto& hint : hints3D)
			{
				RenderHint(grc, hint);
			}

			auto* name = e.level.TryGetName(e.level.NearestRoadId());
			if (name)
			{
				Graphics::StackSpaceGraphics ssg;
				auto& job = Graphics::CreateHorizontalCentredText(ssg, 8, name, RGBAb(255, 255, 255, 255));
				auto& span = grc.EvalSpan({ 0,0 }, job);

				GuiRect addressRect(10, 10, 10 + span.x + 4, 30);

				Graphics::DrawRectangle(grc, addressRect, RGBAb(0, 0, 0, 64), RGBAb(0, 0, 0, 64));
				grc.RenderText({ addressRect.left + 2, addressRect.top }, job);
			}

         GuiMetrics metrics;
         grc.Renderer().GetGuiMetrics(metrics);

         e.gui.RenderDebugElementsAndClear(grc, 9, metrics.screenSpan.x - 320);
		}

		virtual void OnLostTop()
		{
		}
	};
}

namespace Dystopia
{
	IUIControlPane* CreatePaneIsometric(Environment& e)
	{
		return new PaneIsometric(e);
	}
}