#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

class PanelArrayBox : public BasePane, public IArrayBox
{
	int32 fontIndex = 1;
	char populateArrayEventText[128] = "";
	IPublisher& publisher;
public:
	PanelArrayBox(IPublisher& _publisher, int _fontIndex, cstr _populateArrayEventText) :
		fontIndex(_fontIndex), publisher(_publisher)
	{
		CopyString(populateArrayEventText, sizeof populateArrayEventText, _populateArrayEventText);
	}

	void Free() override
	{
		delete this;
	}

	void SetFocusColours(RGBAb normal, RGBAb hillight) override
	{

	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		if (me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
		{
		}
		else if (me.HasFlag(me.LDown) || me.HasFlag(me.RDown))
		{
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
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		IArrayBox* AddArrayBox(IPublisher& publisher, BasePane& panel, int32 fontIndex, const fstring& populatorEventKey, const GuiRect& rect)
		{
			auto* ab = new PanelArrayBox(publisher, fontIndex, populatorEventKey);
			panel.AddChild(ab);
			ab->SetRect(rect);
			return ab;
		}
	}
}