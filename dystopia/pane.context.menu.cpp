#include "dystopia.h"
#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "rococo.fonts.h"

#include <string>

#include <vector>
#include "dystopia.ui.h"

using namespace Rococo;
using namespace Dystopia;

namespace
{
	struct ContextMenuItemImpl
	{
		std::wstring buttonName;
		int32 commandId;
		int64 context;
		GuiRect lastRenderedRect;
		bool isActive;
	};

	class ContextMenu : public IUIPaneSupervisor
	{
		Environment& e;
		std::vector<ContextMenuItemImpl> items;
		Vec2i topLeft;
		GuiRect lastRenderedRect;
		ContextMenuItemImpl lastHighlight;
		IEventCallback<ContextMenuItem>& onClick;
	
	public:
		ContextMenu(Environment& _e, Vec2i _topLeft, ContextMenuItem newMenu[], IEventCallback<ContextMenuItem>& _onClick): 
			e(_e), onClick(_onClick), topLeft(_topLeft)
		{
			for (auto cmi = newMenu; cmi->buttonName != nullptr; cmi++)
			{
				items.push_back({ cmi->buttonName, cmi->commandId, cmi->context,{ 0,0,0,0 }, cmi->isActive });
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual Relay OnTimestep(const TimestepEvent& clock)
		{
			return Relay_Next;
		}

		virtual Relay OnKeyboardEvent(const KeyboardEvent& ke)
		{
			return Relay_Next;
		}

		virtual Relay OnMouseEvent(const MouseEvent& me)
		{
			if (me.HasFlag(MouseEvent::LUp))
			{
				if (!lastHighlight.buttonName.empty())
				{
					ContextMenuItem value{ lastHighlight.buttonName.c_str(), lastHighlight.commandId, lastHighlight.context };
					e.uiStack.PopTop();
					onClick.OnEvent(value);
					Free();
				}	
			}

			return Relay_None;
		}

		virtual void OnPop()
		{

		}

		virtual void OnTop()
		{

		}

		virtual void RenderObjects(IRenderContext& rc)
		{
		}

		virtual void RenderGui(IGuiRenderContext& gc)
		{
			const int X_BORDER = 6;
			const int Y_BORDER = 4;

			Vec2i totalSpan = { 0,Y_BORDER };

			int fontHeight = 0;

			for (auto i : items)
			{
				Graphics::StackSpaceGraphics ss;
				auto& item = CreateHorizontalCentredText(ss, 3, i.buttonName.c_str(), 0xFFFFFFFF);
				auto span = gc.EvalSpan({ 0,0 }, item);
				totalSpan.y += span.y + Y_BORDER;
				totalSpan.x = max(span.x, totalSpan.x);
				fontHeight = span.y;
			}

			totalSpan.x += 2 * X_BORDER;

			lastRenderedRect = GuiRect(topLeft.x, topLeft.y, topLeft.x + totalSpan.x, topLeft.y + totalSpan.y);

			Graphics::DrawRectangle(gc, lastRenderedRect, RGBAb(64, 0, 0), RGBAb(0, 64, 0));
			Graphics::DrawBorderAround(gc, lastRenderedRect, { 2,2 }, RGBAb(128, 128, 128), RGBAb(192, 192, 192));

			Vec2i p = topLeft + Vec2i{ X_BORDER, Y_BORDER };

			GuiMetrics metrics;
			gc.Renderer().GetGuiMetrics(metrics);

			lastHighlight.buttonName.clear();

			for (auto i : items)
			{
				RGBAb fontColour(192, 192, 192);
				i.lastRenderedRect = GuiRect(p.x - X_BORDER, p.y, p.x + totalSpan.x - X_BORDER, p.y + fontHeight);

				if (IsPointInRect(metrics.cursorPosition, i.lastRenderedRect) && i.isActive)
				{
					lastHighlight = i;
					Graphics::DrawRectangle(gc, i.lastRenderedRect, RGBAb(128, 0, 0), RGBAb(0, 0, 128));
					fontColour = RGBAb(255, 255, 255);
				}

				if (!i.isActive) fontColour = RGBAb(64, 64, 64);

				Graphics::StackSpaceGraphics ss;
				auto& item = CreateHorizontalCentredText(ss, 3, i.buttonName.c_str(), fontColour);
				gc.RenderText(p, item);
				p.y += fontHeight + Y_BORDER;
			}
		}
	};
}

namespace Dystopia
{
	IUIPaneSupervisor* CreateContextMenu(Environment& e, Vec2i topLeft, ContextMenuItem newMenu[], IEventCallback<ContextMenuItem>& onClick)
	{
		return new ContextMenu(e, topLeft, newMenu, onClick);
	}
}