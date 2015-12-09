#include "dystopia.h"
#include "rococo.renderer.h"
#include "human.types.h"

#include "rococo.ui.h"
#include "dystopia.ui.h"
#include "rococo.strings.h"

#include "rpg.rules.h"

#include "rococo.strings.h"

using namespace Rococo;
using namespace Dystopia;

void GetBuildVersion(unsigned& major, unsigned& minor);

namespace
{
	float GetAspectRatio(const IRenderer& renderer)
	{
		GuiMetrics metrics;
		renderer.GetGuiMetrics(metrics);
		float aspectRatio = metrics.screenSpan.y / float(metrics.screenSpan.x);
		return aspectRatio;
	}

	Vec2 PixelSpaceToScreenSpace(const Vec2i& v, IRenderer& renderer)
	{
		GuiMetrics metrics;
		renderer.GetGuiMetrics(metrics);

		return{ (2.0f * (float)metrics.cursorPosition.x - metrics.screenSpan.x) / metrics.screenSpan.x,
			-(2.0f * (float)metrics.cursorPosition.y - metrics.screenSpan.y) / metrics.screenSpan.y };
	}

	void DrawHealthBar(IGuiRenderContext& gr, IHumanAI& human)
	{
		Stat health = human.GetStat(StatIndex_Health);

		if (health.cap <= 0 || health.current <= 0) return;

		GuiMetrics metrics;
		gr.Renderer().GetGuiMetrics(metrics);

		float left = 0.985f * (float)metrics.screenSpan.x;
		float bottom = 0.125f * (float)metrics.screenSpan.y;

		float healthRatio = (float)health.current / (float)health.cap;

		bool overLoad = false;
		if (healthRatio > 1.0f)
		{
			overLoad = true;
			healthRatio = 1.0f;
		}

		float height = 0.1f * (float)metrics.screenSpan.y * healthRatio;

		float right = left + 0.005f * (float)metrics.screenSpan.x;
		float top = bottom - height;

		int redness = 63 + (int)(192.0f * healthRatio);

		uint8 yellow = overLoad ? 0xFF : 0x00;

		RGBAb healthColour{ (uint8)redness, yellow, 0, 0xFF };

		GuiVertex q0[6] =
		{
			{ left,   bottom, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }, // bottom left
			{ right,  bottom, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }, // bottom right
			{ left,      top, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }, // top left
			{ left,      top, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }, // top left
			{ right,  bottom, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }, // bottom right
			{ right,     top, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }  // top right
		};

		gr.AddTriangle(q0);
		gr.AddTriangle(q0 + 3);
	}

	class PersonalInfoPanel : public IUIPaneSupervisor, public IEventCallback<ActionMap>
	{
		Environment& e;

	public:
		PersonalInfoPanel(Environment& _e) :
			e(_e)
		{

		}

		virtual void Free()
		{
			delete this;
		}

		virtual void OnEvent(ActionMap& map)
		{
			switch (map.type)
			{
			case ActionMapTypeStats:
				if (map.isActive)
				{
					e.uiStack.PopTop();
				}
			}
		}

		virtual Relay OnTimestep(const TimestepEvent& clock)
		{
			return Relay_None;
		}

		virtual Relay OnKeyboardEvent(const KeyboardEvent& ke)
		{
			e.controls.MapKeyboardEvent(ke, *this);
			return Relay_None;
		}

		virtual Relay OnMouseEvent(const MouseEvent& me)
		{
			e.controls.MapMouseEvent(me, *this);
			return Relay_None;
		}


		virtual void OnPop()
		{

		}

		virtual void RenderObjects(IRenderContext& rc)
		{
		}

		virtual void RenderGui(IGuiRenderContext& grc)
		{
			auto id = e.level.GetPlayerId();

			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);

			auto rect = GuiRect(10, 10, metrics.screenSpan.x - 10, metrics.screenSpan.y - 10);
			Graphics::DrawRectangle(grc, rect, RGBAb(64, 0, 0, 192), RGBAb(0, 0, 64, 192));

			struct StatBinding
			{
				const wchar_t* name;
				StatIndex index;
			};

			StatBinding bindings[] =
			{
				{ L"Health", StatIndex_Health },
			};

			for (size_t i = 0; i < sizeof(bindings) / sizeof(StatBinding); i++)
			{
				auto& b = bindings[i];
				auto stat = e.level.GetHuman(id).ai->GetStat(b.index);

				int32 y = 30 * (1 + (int32)i);

				wchar_t text[64];
				SafeFormat(text, _TRUNCATE, L"%s:", b.name);
				Graphics::RenderHorizontalCentredText(grc, text, RGBAb(255, 255, 255, 192), 2, { 30,y });

				SafeFormat(text, _TRUNCATE, L"%3d / %3d", stat.current, stat.cap);
				Graphics::RenderHorizontalCentredText(grc, text, RGBAb(255, 255, 255, 192), 2, { 130,y });
			}

			wchar_t text[64];
			SafeFormat(text, _TRUNCATE, L"Social Status: lower lower class (double minus)");
			Graphics::RenderHorizontalCentredText(grc, text, RGBAb(255, 255, 255, 192), 2, { 330,30 });

			SafeFormat(text, _TRUNCATE, L"Security clearance: none");
			Graphics::RenderHorizontalCentredText(grc, text, RGBAb(255, 255, 255, 192), 2, { 330,60 });

			GuiRect infoRect(rect.left + 20, rect.top + 100, rect.right - 20, rect.bottom - 20);

			Graphics::StackSpaceGraphics ssg;

			auto& player = e.level.GetHuman(id);

			wchar_t buffer[4096];
			SafeStackString sss(buffer, 4096);
			AutoFree<IStringBuilder> sb = CreateSafeStackStringBuilder(sss);
			sb->AppendFormat(L"The state of the nation:\n");
			sb->AppendFormat(L"\tFollowing the financial collapse Britain has fallen into anarchy ");
			sb->AppendFormat(L"and Prime Minister Theresa Clinton-June has declared a state of emergency. ");
			sb->AppendFormat(L"Looters are burning your home town and the police refuse to intervene.");
			sb->AppendFormat(L"The first thing to do is embarrass the government with severe vigilante reprisals against the looting scum.\n\n");
			sb->AppendFormat(L"Your kudos: (%s)\n", RPG::GetTitle(player.ai->GetStat(StatIndex_Kudos)));
			sb->AppendFormat(L"\t\t%s\n\n", RPG::GetKudosRewardText(player.ai->GetStat(StatIndex_Kudos)));
			sb->AppendFormat(L"Your social status:\n");
			sb->AppendFormat(L"\tAs of late you lost your job and were bankrupted. The operative terminology is 'bum', Your friends call you 'Johnny no mates', well if you had ");
			sb->AppendFormat(L"one he'd probably call you that. Your life is so miserable and meaningless death means nothing to you.");
			auto& job = Graphics::CreateLeftAlignedText(ssg, infoRect, 150, 50, 3, *sb, RGBAb(255, 255, 255));
			grc.RenderText({ 0,0 }, job);

			unsigned major, minor;
			GetBuildVersion(major, minor);

			wchar_t versionText[64];
			SafeFormat(versionText, _TRUNCATE, L"Build %u.%04u", major, minor);

			Graphics::RenderHorizontalCentredText(grc, versionText, RGBAb(255, 255, 255), 9, { rect.right - 240,rect.top });

         auto mem = ProcessMemory();
         
         wchar_t memoryUsage[64];
         SafeFormat(memoryUsage, _TRUNCATE, L"Mem: %I64u MB/%I64u MB", mem.current >> 20, mem.peak >> 20);

         Graphics::RenderHorizontalCentredText(grc, memoryUsage, RGBAb(255, 255, 255), 9, { rect.right - 240,rect.top + 20 });
		}
	};
}

namespace Dystopia
{
	IUIPaneSupervisor* CreatePersonalInfoPanel(Environment& e)
	{
		return new PersonalInfoPanel(e);
	}
}