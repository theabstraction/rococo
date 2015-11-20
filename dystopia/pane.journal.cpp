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
}

namespace
{
	using namespace Rococo;
	using namespace Dystopia;

	RGBAb white(255, 255, 255);

	void ComputeLayout(IGuiRenderContext& grc, const GuiRect& textRect, IJournalSupervisor& history)
	{
		struct : IMutableEnumerator<IHistoricEvent>
		{
			IGuiRenderContext* grc;
			GuiRect allRect;

			int32 y = 0;

			int32 y0, y1;

			virtual void operator()(IHistoricEvent& event)
			{
				event.Layout().yTitlePixelRow = y;

				Graphics::StackSpaceGraphics ssg;

				{
					auto& evaljob1 = Graphics::CreateLeftAlignedText(ssg, allRect, 150, 20, 3, event.Title(), white);
					Vec2i span = grc->EvalSpan({ 0,0 }, evaljob1);
					y0 = y + span.y;
				}

				{
					auto& evaljob2 = Graphics::CreateLeftAlignedText(ssg, allRect, 150, 20, 7, event.Body(), white);
					Vec2i span = grc->EvalSpan({ 0,0 }, evaljob2);
					y1 = y0 + span.y + 2;
				}

				y = y1 + 20;

				event.Layout().yBodyPixelRow = y0;
				event.Layout().yEndOfBodyPixelRow = y1;
			}
		} layout;

		layout.allRect = GuiRect(textRect.left, 0, textRect.right, 0x7FFFFFFF);
		layout.grc = &grc;

		history.History().Enumerate(layout);
	}

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
			GuiRect textRect = Expand(outerRect, -4);

			auto& history = e.journal.History();

			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);

			if (!e.journal.IsReadyForRender() || lastSpan != metrics.screenSpan)
			{
				lastSpan = metrics.screenSpan;
				ComputeLayout(grc, textRect, e.journal);
				historyBar.pos = 0;
			}

			historyBar.domain = history[history.Count()-1].Layout().yEndOfBodyPixelRow;
			historyBar.pageSize = Height(outerRect);
			historyBar.rect = GuiRect(outerRect.right - 20, outerRect.top, outerRect.right, outerRect.bottom);
			DrawScrollbar(grc, historyBar);
	
			auto dy = (int32) (historyBar.pos * ((float) historyBar.domain / historyBar.pageSize)); 
			for (size_t i = 0; i < history.Count(); ++i)
			{
				auto& event = history[i];
				auto& layout = event.Layout();

				RGBAb textColour;

				switch (event.Type())
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

				if (layout.yEndOfBodyPixelRow - dy < 0)
				{
					continue;
				}

				if (layout.yTitlePixelRow - dy > Height(textRect))
				{
					break;
				}

				Graphics::StackSpaceGraphics ssg;

				GuiRect titleRect(textRect.left, layout.yTitlePixelRow + textRect.top - dy, textRect.right, layout.yBodyPixelRow + textRect.top);
				auto& titleJob = Graphics::CreateLeftAlignedText(ssg, titleRect, 150,20, 3, event.Title(), textColour);
				grc.RenderText({ 0,0 }, titleJob, &textRect);

				GuiRect bodyRect(textRect.left, layout.yBodyPixelRow + textRect.top - dy, textRect.right, layout.yEndOfBodyPixelRow + textRect.top);

				auto& bodyJob = Graphics::CreateLeftAlignedText(ssg, bodyRect, 150,20, 7, event.Body(), textColour);
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
}