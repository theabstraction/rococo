#include "dystopia.h"
#include "dystopia.ui.h"
#include "rococo.renderer.h"

#include <vector>
#include <string>

namespace Dystopia
{
	using namespace Rococo;

	struct HistoricEvent
	{
		std::wstring title;
		std::wstring body;
		int32 yTitlePixelRow;
		int32 yBodyPixelRow;
		int32 yEndOfBodyPixelRow;
	};
}

namespace
{
	using namespace Rococo;
	using namespace Dystopia;

	typedef std::vector<HistoricEvent> THistory;

	RGBAb textColour(255, 255, 255);

	void ComputeLayout(IGuiRenderContext& grc, const GuiRect& textRect, IJournalSupervisor& history)
	{
		int32 y = 0;

		int32 y0, y1;

		GuiRect allRect(textRect.left, 0, textRect.right, 0x7FFFFFFF);

		for (size_t i = 0; i < history.Count(); ++i)
		{
			auto& event = history.GetEvent(i);
		
			event.yTitlePixelRow = y;

			Graphics::StackSpaceGraphics ssg;

			{
				auto& evaljob1 = Graphics::CreateLeftAlignedText(ssg, allRect, 100, 50, 3, event.title.c_str(), textColour);
				Vec2i span = grc.EvalSpan({ 0,0 }, evaljob1);
				y0 = y + span.y;
			}

			{
				auto& evaljob2 = Graphics::CreateLeftAlignedText(ssg, allRect, 100, 50, 7, event.body.c_str(), textColour);
				Vec2i span = grc.EvalSpan({ 0,0 }, evaljob2);
				y1 = y0 + span.y + 2;
			}

			y = y1 + 20;
	
			event.yBodyPixelRow = y0;
			event.yEndOfBodyPixelRow = y1;
		}
	}

	typedef std::vector<IGoalSupervisor*> TGoals;

	class Goal_MeetObject: public IGoalSupervisor
	{
		ID_ENTITY a;
		ID_ENTITY b;

		std::wstring title;
		std::wstring body;

		Metres completionRadius;
		GoalState state;

		ILevel& level;
	public:
		Goal_MeetObject(Environment& e, Metres _radius, const wchar_t* _title, const wchar_t* _body, ID_ENTITY _a, ID_ENTITY _b): 
			level(e.level), 
			state(GoalState_Ongoing), 
			completionRadius(_radius),
			title(_title),
			body(_body),
			a(_a),
			b(_b)
		{

		}

		virtual void Free()
		{
			delete this;
		}

		virtual const wchar_t* Title() const { return title.c_str();  }
		virtual const wchar_t* Body() const { return body.c_str(); }

		virtual GoalState OnTimestep(float gameTime, float dt)
		{
			if (state == GoalState_Complete) return state;

			const Vec3* posA, *posB;
			posA = level.TryGetPosition(a);
			posB = level.TryGetPosition(b);

			if (!posA || !posB)
			{
				state = GoalState_Failed;
				return state;
			}

			if (IsInRange(*posA - *posB, completionRadius))
			{
				state = GoalState_Complete;
				return state;
			}

			return state;
		}

		virtual GoalState State() const
		{
			return state;
		}
	};

	class Journal : public IJournalSupervisor
	{
		THistory history;
		TGoals goals;
	public:
		Journal()
		{
		}

		virtual void AddHistory(const fstring& title, const fstring& body)
		{
			HistoricEvent he{ title.buffer, body.buffer, 0, 0, 0 };
			history.push_back(he);
		}

		virtual bool IsReadyForRender() const
		{
			return history.back().yEndOfBodyPixelRow != 0;
		}

		virtual size_t Count() const
		{
			return history.size();
		}

		virtual HistoricEvent& GetEvent(size_t index)
		{
			return history[index];
		}

		virtual void Free()
		{
			delete this;
		}
	};

	class JournalPane : public IUIPaneSupervisor, public IEventCallback<ActionMap>
	{
	private:
		Environment& e;
		bool isHistoryMode;

		GuiRect lastHistoryRect;
		GuiRect lastGoalRect;

		Vec2i lastSpan;

		void DrawHistory(IGuiRenderContext& grc, const GuiRect& outerRect)
		{
			if (!e.journal.Count()) return;
			
			GuiRect textRect = Expand(outerRect, -4);

			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);

			if (e.journal.IsReadyForRender() || lastSpan != metrics.screenSpan)
			{
				metrics.screenSpan = lastSpan;
				ComputeLayout(grc, textRect, e.journal);
			}
	
			auto y = textRect.top;
			for (size_t i = 0; i < e.journal.Count(); ++i)
			{
				auto& event = e.journal.GetEvent(i);

				Graphics::StackSpaceGraphics ssg;

				GuiRect titleRect(textRect.left, event.yTitlePixelRow + textRect.top, textRect.right, event.yBodyPixelRow + textRect.top);
				auto& titleJob = Graphics::CreateLeftAlignedText(ssg, titleRect, 100, 50, 3, event.title.c_str(), textColour);
				grc.RenderText({ 0,0 }, titleJob);

				GuiRect bodyRect(textRect.left, event.yBodyPixelRow + textRect.top, textRect.right, event.yEndOfBodyPixelRow + textRect.top);
				auto& bodyJob = Graphics::CreateLeftAlignedText(ssg, bodyRect, 100, 50, 7, event.body.c_str(), textColour);
				grc.RenderText({ 0,0 }, bodyJob);		
			}
		}
	public:
		JournalPane(Environment& _e) : e(_e), isHistoryMode(false)
		{
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void OnEvent(ActionMap& arg)
		{
			switch (arg.type)
			{
			case ActionMapTypeJournal:
				if (arg.isActive) e.uiStack.PopTop();
				break;
			case ActionMapTypeSelect:
				if (arg.isActive)
				{
					GuiMetrics metrics;
					e.renderer.GetGuiMetrics(metrics);
					if (IsPointInRect(metrics.cursorPosition, lastHistoryRect))
					{
						isHistoryMode = true;
					}
					else if (IsPointInRect(metrics.cursorPosition, lastGoalRect))
					{
						isHistoryMode = false;
					}
				}
				break;
			}
		}

		virtual Relay OnTimestep(const TimestepEvent& timestep)
		{
			return Relay_None;
		}

		virtual Relay OnKeyboardEvent(const KeyboardEvent& key)
		{
			e.controls.MapKeyboardEvent(key, *this);
			return Relay_None;
		}

		virtual Relay OnMouseEvent(const MouseEvent& me)
		{
			e.controls.MapMouseEvent(me, *this);
			return Relay_None;
		}

		virtual void RenderGui(IGuiRenderContext& grc)
		{
			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);

			auto controlRect = GuiRect(10, 10, 230, 250);
			Graphics::DrawRectangle(grc, controlRect, RGBAb(64, 0, 0, 192), RGBAb(0, 0, 64, 192));

			Graphics::RenderHorizontalCentredText(grc, L"Journal", RGBAb(255, 255, 255), 0, { controlRect.left + 2, controlRect.top });

			auto historyRect = GuiRect(0, 0, 150, 30) + Vec2i{ controlRect.left + 50, controlRect.top + 80 };
			lastHistoryRect = historyRect;
			Graphics::DrawRectangle(grc, historyRect, RGBAb(64, 0, 0, 64), RGBAb(0, 0, 64, 64));

			RGBAb white(255, 255, 255);
			RGBAb grey(160, 160, 160);

			RGBAb fontColour1 = IsPointInRect(metrics.cursorPosition, historyRect) ? white : grey;
			Graphics::RenderVerticalCentredText(grc, Centre(historyRect).x, historyRect.top, fontColour1, L"History...", 3);

			auto objRect = GuiRect(0, 0, 150, 30) + Vec2i{ controlRect.left + 50, controlRect.top + 120 };
			lastGoalRect = objRect;
			Graphics::DrawRectangle(grc, objRect, RGBAb(64, 0, 0, 64), RGBAb(0, 0, 64, 64));
			RGBAb fontColour2 = IsPointInRect(metrics.cursorPosition, objRect) ? white : grey;

			Graphics::RenderVerticalCentredText(grc, Centre(objRect).x, objRect.top, fontColour2, L"Goals...", 3);

			auto textRect = GuiRect(240, 10, metrics.screenSpan.x - 10, metrics.screenSpan.y - 10);
			Graphics::DrawRectangle(grc, textRect, RGBAb(64, 0, 0, 192), RGBAb(0, 0, 64, 192));
		
			Graphics::DrawBorderAround(grc, isHistoryMode ? historyRect : objRect, { 2,2 }, white, white);

			if (isHistoryMode)
			{
				DrawHistory(grc, textRect);
			}
		} 

		virtual void RenderObjects(IRenderContext& rc)
		{

		}
	};
}

namespace Dystopia
{
	IUIPaneSupervisor* CreateJournalPane(Environment& e)
	{
		return new JournalPane(e);
	}

	IJournalSupervisor* CreateJournal()
	{
		return new Journal();
	}

	IGoalSupervisor* CreateGoal_MeetObject(Environment& e, Metres _radius, const wchar_t* _title, const wchar_t* _body, ID_ENTITY _a, ID_ENTITY _b)
	{
		return new Goal_MeetObject(e, _radius, _title, _body, _a, _b);
	}
}