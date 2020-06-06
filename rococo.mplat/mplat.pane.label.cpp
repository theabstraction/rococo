#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

class PanelLabel : public BasePane, public ILabelPane
{
	int32 fontIndex = 1;
	char text[128];
	int32 horzAlign = 0;
	int32 vertAlign = 0;
	Vec2i padding{ 0,0 };
	IPublisher& publisher;
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

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		if (me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
		{
			BasePane::Invoke(publisher, 1);
		}
		else if (me.HasFlag(me.LDown) || me.HasFlag(me.RDown))
		{
			BasePane::Invoke(publisher, 0);
		}
	}

	bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		return false;
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		RenderBackground(grc, topLeft, modality);

		auto span = Span(ClientRect());
		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

		RenderLabel(grc, text, absRect, horzAlign, vertAlign, padding, fontIndex, BasePane::Scheme(), !modality.isUnderModal);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		Rococo::ILabelPane* AddLabel(IPublisher& publisher, BasePane& panel, int32 fontIndex, const fstring& text, const GuiRect& rect)
		{
			auto* label = new PanelLabel(publisher, fontIndex, text);
			panel.AddChild(label);
			label->SetRect(rect);
			return label;
		}
	}
}