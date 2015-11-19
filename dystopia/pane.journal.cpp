#include "dystopia.h"
#include "dystopia.ui.h"
#include "rococo.renderer.h"

#include <vector>
#include <string>
#include <algorithm>

#include "dystopia.post.h"
#include "rococo.strings.h"

#include "rococo.ui.h"

namespace Dystopia
{
	using namespace Rococo;

	enum HistoricEventType
	{
		HistoricEventType_Narrative,
		HistoricEventType_CompletedGoal,
		HistoricEventType_FailedGoal
	};

	struct HistoricEvent
	{
		std::wstring title;
		std::wstring body;
		int32 yTitlePixelRow;
		int32 yBodyPixelRow;
		int32 yEndOfBodyPixelRow;
		HistoricEventType type;
	};
}

namespace
{
	using namespace Rococo;
	using namespace Dystopia;

	typedef std::vector<HistoricEvent> THistory;

	RGBAb white(255, 255, 255);

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
				auto& evaljob1 = Graphics::CreateLeftAlignedText(ssg, allRect, 150,20, 3, event.title.c_str(), white);
				Vec2i span = grc.EvalSpan({ 0,0 }, evaljob1);
				y0 = y + span.y;
			}

			{
				auto& evaljob2 = Graphics::CreateLeftAlignedText(ssg, allRect, 150,20, 7, event.body.c_str(), white);
				Vec2i span = grc.EvalSpan({ 0,0 }, evaljob2);
				y1 = y0 + span.y + 2;
			}

			y = y1 + 20;
	
			event.yBodyPixelRow = y0;
			event.yEndOfBodyPixelRow = y1;
		}
	}

	typedef std::vector<IGoalSupervisor*> TGoals;

	class Goal_MeetObject: public IGoalSupervisor, public Post::IRecipient
	{
		ID_ENTITY a;
		ID_ENTITY b;

		std::wstring title;
		std::wstring body;

		Metres completionRadius;
		GoalState state;

		ILevel& level;

		Post::Subscribtion<AdvanceTimestepEvent> sub_Timestep;
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
			sub_Timestep.Start(e.postbox, this);
		}

		virtual void OnPost(const Mail& mail)
		{
			auto* timestepEvent = Post::InterpretAs<AdvanceTimestepEvent>(mail);
			if (timestepEvent)
			{
				OnTimestep(timestepEvent->gameTime, timestepEvent->dt);
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual const wchar_t* Title() const { return title.c_str();  }
		virtual const wchar_t* Body() const { return body.c_str(); }

		void OnTimestep(float gameTime, float dt)
		{
			if (state == GoalState_Complete) return;

			const Vec3* posA, *posB;
			posA = level.TryGetPosition(a);
			posB = level.TryGetPosition(b);

			if (!posA || !posB)
			{
				state = GoalState_Failed;
				return;
			}

			if (IsInRange(*posA - *posB, completionRadius))
			{
				state = GoalState_Complete;
				return;
			}
		}

		virtual GoalState State() const
		{
			return state;
		}
	};

	void UpdateGoals(TGoals& goals, THistory& history, Environment& e)
	{
		for (auto& g : goals)
		{
			if (g->State() == GoalState_Complete)
			{
				wchar_t newBody[4096];
				SafeFormat(newBody, _TRUNCATE, L"This goal was completed - \"%s\"", g->Body());
				HistoricEvent he{ g->Title(), newBody, 0, 0, 0, HistoricEventType_CompletedGoal };
				history.push_back(he);

				e.gui.Add3DHint(e.level.GetPosition(e.level.GetPlayerId()), L"Goal complete!"_fstring, 2.0_seconds);
			}
			else if (g->State() == GoalState_Failed)
			{
				wchar_t newBody[4096];
				SafeFormat(newBody, _TRUNCATE, L"You failed to complete this goal - \"%s\"", g->Body());
				HistoricEvent he{ g->Title(), newBody, 0, 0, 0, HistoricEventType_FailedGoal };
				history.push_back(he);

				e.gui.Add3DHint(e.level.GetPosition(e.level.GetPlayerId()), L"Goal failed!"_fstring, 2.0_seconds);
			}
		}

		auto r = std::remove_if(goals.begin(), goals.end(), [](IGoal* goal) {
			return goal->State() != GoalState_Ongoing;
		});

		for (auto i = r; i != goals.end(); ++i)
		{
			(*i)->Free();
		}

		goals.erase(r, goals.end());
	}

	class Journal : public IJournalSupervisor, public Post::IRecipient
	{
		Environment& e;
		THistory history;
		TGoals goals;

		Post::Subscribtion<TimestepEvent> sub_Timestep;

	public:
		Journal(Environment& _e): e(_e)
		{
		}

		~Journal()
		{
			for (auto& g : goals)
			{
				g->Free();
			}
		}

		virtual void EnumerateGoals(IEnumerator<IGoal>& cb)
		{
			for (auto& g : goals)
			{
				cb(*g);
			}
		}

		virtual void PostConstruct()
		{
			sub_Timestep.Start(e.postbox, this);
		}

		virtual void OnPost(const Mail& mail)
		{
			auto* timestepEvent = Post::InterpretAs<TimestepEvent>(mail);
			if (timestepEvent)
			{
				UpdateGoals(goals, history, e);
			}
		}

		virtual void AddHistory(const fstring& title, const fstring& body)
		{
			HistoricEvent he{ title.buffer, body.buffer, 0, 0, 0, HistoricEventType_Narrative };
			history.push_back(he);
		}

		virtual void AddGoalMeet(const fstring& title, const fstring& body, ID_ENTITY a, ID_ENTITY b, Metres radius)
		{
			auto* goal = CreateGoal_MeetObject(e, radius, title, body, a, b);
			goals.push_back(goal);
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

	struct ScrollBar
	{
		GuiRect rect;
		int32 domain;
		int32 pageSize;
		int32 pos = 0;
		Vec2i holdPos = { -1,-1 };
	};

	struct VertScrollBar: public ScrollBar
	{

	};

	GuiRect GetBarRect(const VertScrollBar& vbar)
	{
		GuiRect innerRect = Expand(vbar.rect, -2);

		float q = vbar.domain == 0 ? 1.0f : (vbar.pageSize / (float)vbar.domain);

		if (q < 0) q = 0;
		if (q > 1.0f) q = 1.0f;

		int32 barHeight = int32(q * Height(innerRect));
		int32 barY = innerRect.top + (int32) (q * vbar.pos);

		return GuiRect(innerRect.left, barY, innerRect.right, min(barY + barHeight, innerRect.bottom));
	}

	void SendClickToScrollBar(VertScrollBar& vbar, Vec2i clickPos, bool isHeld)
	{
		if (IsPointInRect(clickPos, GetBarRect(vbar)))
		{
			if (isHeld)
			{
				vbar.holdPos = clickPos;
			}
		}

		if (!isHeld)
		{
			vbar.holdPos = { -1,-1 };
		}
	}

	void DrawScrollbar(IGuiRenderContext& grc, VertScrollBar& scrollBar)
	{
		GuiRect outerRect = Expand(scrollBar.rect, -1);

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		if (scrollBar.holdPos.x >= 0)
		{
			auto ds = metrics.cursorPosition - scrollBar.holdPos;

			float pixelScale = scrollBar.domain == 0 ? 0.0f : ((float) scrollBar.domain / scrollBar.pageSize);

			int32 deltaPos = (int32) (pixelScale * ds.y);

			scrollBar.pos += deltaPos;

			scrollBar.pos = max(0, scrollBar.pos);
			scrollBar.pos = min(scrollBar.pos, scrollBar.domain - scrollBar.pageSize);

			scrollBar.holdPos = metrics.cursorPosition;
		}

		GuiRect barRect = GetBarRect(scrollBar);

		RGBAb barColour1 = (scrollBar.holdPos.x >= 0 || IsPointInRect(metrics.cursorPosition, barRect)) ? RGBAb(255, 128, 128, 128) : RGBAb(255, 128, 128, 32);
		RGBAb barColour2 = (scrollBar.holdPos.x >= 0 || IsPointInRect(metrics.cursorPosition, barRect)) ? RGBAb(128, 128, 255, 128) : RGBAb(128, 128, 255, 32);

		Graphics::DrawRectangle(grc, barRect, barColour1, barColour2);
		Graphics::DrawBorderAround(grc, barRect, Vec2i{ 1,1 }, white, white);
		Graphics::DrawBorderAround(grc, outerRect, Vec2i{ 1,1 }, white, white);
	}

	class JournalPane : public IUIPaneSupervisor, public IEventCallback<ActionMap>
	{
	private:
		Environment& e;
		bool isHistoryMode;

		GuiRect lastHistoryRect;
		GuiRect lastGoalRect;
		GuiRect lastContinueRect;

		Vec2i lastSpan;

		VertScrollBar historyBar;

		void DrawGoals(IGuiRenderContext& grc, const GuiRect& outerRect)
		{
			struct : IEnumerator<IGoal>
			{
				int y;
				IGuiRenderContext* grc;

				GuiRect allRect;
				GuiRect titleRect;
				GuiRect bodyRect;

				int y0;
				int y1;

				virtual void operator()(const IGoal& goal)
				{
					Graphics::StackSpaceGraphics ssg;

					{
						auto& evaljob1 = Graphics::CreateLeftAlignedText(ssg, allRect, 150,20, 3, goal.Title(), white);
						Vec2i span = grc->EvalSpan({ 0,0 }, evaljob1);
						y0 = y + span.y;
					}

					{
						auto& evaljob2 = Graphics::CreateLeftAlignedText(ssg, allRect, 150,20, 7, goal.Body(), white);
						Vec2i span = grc->EvalSpan({ 0,0 }, evaljob2);
						y1 = y0 + span.y + 2;
					}

					{
						titleRect.top = y;
						titleRect.bottom = y0;
						auto& drawjob1 = Graphics::CreateLeftAlignedText(ssg, titleRect, 150,20, 3, goal.Title(), white);
						grc->RenderText({ 0,0 }, drawjob1);
					}

					{
						bodyRect.top = y0 + 2;
						bodyRect.bottom = y1;
						auto& drawjob2 = Graphics::CreateLeftAlignedText(ssg, bodyRect, 150,20, 7, goal.Body(), white);
						grc->RenderText({ 0,0 }, drawjob2);
					}

					y = y1 + 20;
				}
			} renderGoal;

			renderGoal.grc = &grc;
			renderGoal.y = outerRect.top;
			renderGoal.allRect = renderGoal.titleRect = renderGoal.bodyRect = GuiRect(outerRect.left, 0, outerRect.right, 0x7FFFFFFF);

			e.journal.EnumerateGoals(renderGoal);
		}

		void DrawHistory(IGuiRenderContext& grc, const GuiRect& outerRect)
		{
			if (!e.journal.Count()) return;
			
			GuiRect textRect = Expand(outerRect, -4);

			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);

			if (!e.journal.IsReadyForRender() || lastSpan != metrics.screenSpan)
			{
				lastSpan = metrics.screenSpan;
				ComputeLayout(grc, textRect, e.journal);
				historyBar.pos = 0;
			}

			wchar_t mouseBuffer[64];
			SafeFormat(mouseBuffer, _TRUNCATE, L"%d %d", metrics.cursorPosition.x, metrics.cursorPosition.y);
			Graphics::RenderHorizontalCentredText(grc, mouseBuffer, white, 9, { 50, 400 });

			historyBar.domain = e.journal.GetEvent(e.journal.Count() - 1).yEndOfBodyPixelRow;
			historyBar.pageSize = Height(outerRect);
			historyBar.rect = GuiRect(outerRect.right - 20, outerRect.top, outerRect.right, outerRect.bottom);
			DrawScrollbar(grc, historyBar);
	
			auto dy = (int32) (historyBar.pos * ((float) historyBar.domain / historyBar.pageSize)); 
			for (size_t i = 0; i < e.journal.Count(); ++i)
			{
				auto& event = e.journal.GetEvent(i);

				RGBAb textColour;

				switch (event.type)
				{
				case HistoricEventType_CompletedGoal:
					textColour = RGBAb(255, 255, 128);
					break;
				case HistoricEventType_FailedGoal:
					textColour = RGBAb(255, 128, 128);
					break;
				case HistoricEventType_Narrative:
					textColour = white;
					break;
				}

				if (event.yEndOfBodyPixelRow - dy < 0)
				{
					continue;
				}

				if (event.yTitlePixelRow - dy > Height(textRect))
				{
					break;
				}

				Graphics::StackSpaceGraphics ssg;

				GuiRect titleRect(textRect.left, event.yTitlePixelRow + textRect.top - dy, textRect.right, event.yBodyPixelRow + textRect.top);
				auto& titleJob = Graphics::CreateLeftAlignedText(ssg, titleRect, 150,20, 3, event.title.c_str(), textColour);
				grc.RenderText({ 0,0 }, titleJob, &textRect);

				GuiRect bodyRect(textRect.left, event.yBodyPixelRow + textRect.top - dy, textRect.right, event.yEndOfBodyPixelRow + textRect.top);

				auto& bodyJob = Graphics::CreateLeftAlignedText(ssg, bodyRect, 150,20, 7, event.body.c_str(), textColour);
				grc.RenderText({ 0,0 }, bodyJob, &textRect);	
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
			GuiMetrics metrics;
			e.renderer.GetGuiMetrics(metrics);

			switch (arg.type)
			{
			case ActionMapTypeJournal:
				if (arg.isActive) e.uiStack.PopTop();
				break;
			case ActionMapTypeSelect:
				if (arg.isActive)
				{
					if (IsPointInRect(metrics.cursorPosition, lastHistoryRect))
					{
						isHistoryMode = true;
					}
					else if (IsPointInRect(metrics.cursorPosition, lastGoalRect))
					{
						isHistoryMode = false;
					}
					else if (IsPointInRect(metrics.cursorPosition, lastContinueRect))
					{
						e.uiStack.PopTop();
					}
					else if (isHistoryMode && IsPointInRect(metrics.cursorPosition, historyBar.rect))
					{
						SendClickToScrollBar(historyBar, metrics.cursorPosition, arg.isActive);
					}
				}
				else
				{
					SendClickToScrollBar(historyBar, metrics.cursorPosition, false);
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
			if (me.HasFlag(MouseEvent::MouseWheel))
			{
				int32 dy = -(int32)(int16)me.buttonData;
				if (dy < 0)
				{
					if (isHistoryMode)
					{
						historyBar.pos -= historyBar.pageSize;
						historyBar.pos = max(0, historyBar.pos);
					}
				}
				else if (dy > 0)
				{
					if (isHistoryMode)
					{
						historyBar.pos += historyBar.pageSize;
						historyBar.pos = min(historyBar.pos, historyBar.domain - historyBar.pageSize);
					}
				}
			}
			else
			{
				e.controls.MapMouseEvent(me, *this);
			}
			return Relay_None;
		}

		void DrawButton(IGuiRenderContext& grc, Vec2i focusPoint, const wchar_t* label, const GuiRect& rect)
		{
			RGBAb white(255, 255, 255);
			RGBAb grey(160, 160, 160);

			RGBAb fontColour = IsPointInRect(focusPoint, rect) ? white : grey;
			Graphics::DrawRectangle(grc, rect, RGBAb(64, 0, 0, 64), RGBAb(0, 0, 64, 64));

			Graphics::RenderVerticalCentredText(grc, Centre(rect).x, rect.top, fontColour, label, 3);
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

			DrawButton(grc, metrics.cursorPosition, L"History -->", historyRect);

			auto goalRect = GuiRect(0, 0, 150, 30) + Vec2i{ controlRect.left + 50, controlRect.top + 120 };
			lastGoalRect = goalRect;
			
			DrawButton(grc, metrics.cursorPosition, L"Goals -->", goalRect);

			auto contRect = GuiRect(0, 0, 150, 30) + Vec2i{ controlRect.left + 50, controlRect.top + 160 };
			lastContinueRect = contRect;
			
			DrawButton(grc, metrics.cursorPosition, L"Continue...", contRect);

			auto textRect = GuiRect(240, 10, metrics.screenSpan.x - 10, metrics.screenSpan.y - 10);
			Graphics::DrawRectangle(grc, textRect, RGBAb(64, 0, 0, 192), RGBAb(0, 0, 64, 192));
		
			Graphics::DrawBorderAround(grc, isHistoryMode ? historyRect : goalRect, { 2,2 }, white, white);

			if (isHistoryMode)
			{
				DrawHistory(grc, textRect);
			}
			else
			{
				DrawGoals(grc, textRect);
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

	IJournalSupervisor* CreateJournal(Environment& e)
	{
		return new Journal(e);
	}

	IGoalSupervisor* CreateGoal_MeetObject(Environment& e, Metres radius, const wchar_t* title, const wchar_t* body, ID_ENTITY a, ID_ENTITY b)
	{
		return new Goal_MeetObject(e, radius, title, body, a, b);
	}
}