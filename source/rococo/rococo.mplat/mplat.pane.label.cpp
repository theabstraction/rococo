#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;
using namespace Rococo::Graphics;
using namespace Rococo::Strings;

class PanelLabel : public BasePane, public GUI::ILabelPane
{
	int32 fontIndex = 1;
	char text[128];
	int32 horzAlign = 0;
	int32 vertAlign = 0;
	Vec2i padding{ 0,0 };
	IPublisher& publisher;
	EventIdRef evEnabler;

	RGBAb greyout1{ 0 };
	RGBAb greyout2{ 0 };
public:
	PanelLabel(IPublisher& _publisher, int _fontIndex, cstr _text) : fontIndex(_fontIndex), publisher(_publisher)
	{
		SafeFormat(text, sizeof(text), "%s", _text);
	}

	void Free() override
	{
		delete this;
	}

	void SetAlignment(int32 horz, int32 vert, int32 paddingX, int paddingY)
	{
		horzAlign = horz;
		vertAlign = vert;
		padding = { paddingX, paddingY };
	}

	bool IsEnabled() const
	{
		if (evEnabler.hashCode == 0) return true;

		TEventArgs<bool> args;
		args.value = true;
		publisher.Publish(args, evEnabler);
		return args.value;
	}

	void SetEnableEvent(const fstring& enablerEventName, RGBAb grey1, RGBAb grey2) override
	{
		evEnabler = publisher.CreateEventIdFromVolatileString(enablerEventName);
		greyout1 = grey1;
		greyout2 = grey2;
	}

	bool isPressed = false;

	void AppendEvent(const MouseEvent& me, const Vec2i&) override
	{
		if (me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
		{
			isPressed = false;
			if (IsEnabled())
			{
				BasePane::Invoke(publisher, 1);
			}
		}
		else if (me.HasFlag(me.LDown) || me.HasFlag(me.RDown))
		{
			if (IsEnabled())
			{
				isPressed = true;
				BasePane::Invoke(publisher, 0);
			}
		}
	}

	bool AppendEvent(const KeyboardEventEx&, const Vec2i&, const Vec2i&) override
	{
		return false;
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		auto span = Span(ClientRect());
		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		bool isEnabled = IsEnabled();
		bool isLit = isEnabled && !modality.isUnderModal && IsPointInRect(metrics.cursorPosition, absRect);
		auto& s = Scheme();

		RGBAb backColour = isPressed && isLit ? s.bottomRight : s.topLeft;

		Graphics::DrawRectangle(grc, absRect, backColour, backColour);

		if (Eq(text, "$up"))
		{
			int ds = (absRect.right - absRect.left) >> 3;
			GuiRect triRect{ absRect.left + ds, absRect.top + ds, absRect.right - ds, absRect.bottom - ds };
			Graphics::DrawTriangleFacingUp(grc, triRect, isLit ? s.hi_fontColour : s.fontColour);
		}
		else if (Eq(text, "$down"))
		{
			int ds = (absRect.right - absRect.left) >> 3;
			GuiRect triRect{ absRect.left + ds, absRect.top + ds, absRect.right - ds, absRect.bottom - ds };
			Graphics::DrawTriangleFacingDown(grc, triRect, isLit ? s.hi_fontColour : s.fontColour);
		}
		else
		{
			BasePane::RenderBkImage(grc, topLeft, modality);
			RenderLabel(grc, text, absRect, horzAlign, vertAlign, padding, fontIndex, s, !modality.isUnderModal && isEnabled);
		}

		if (!isEnabled)
		{
			GuiRect greyRect{ absRect.left + 1, absRect.top + 1, absRect.right - 1, absRect.bottom - 1 };
			Graphics::DrawRectangle(grc, greyRect, greyout1, greyout2);
		}

		RGBAb edge1 = isLit ? s.hi_bottomRightEdge : s.bottomRightEdge;
		RGBAb edge2 = isLit ? s.hi_topLeftEdge : s.topLeftEdge;
		Graphics::DrawBorderAround(grc, absRect, { 1,1 }, edge1, edge2);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		Rococo::GUI::ILabelPane* AddLabel(IPublisher& publisher, BasePane& panel, int32 fontIndex, const fstring& text, const GuiRect& rect)
		{
			auto* label = new PanelLabel(publisher, fontIndex, text);
			panel.AddChild(label);
			label->SetRect(rect);
			return label;
		}
	}
}