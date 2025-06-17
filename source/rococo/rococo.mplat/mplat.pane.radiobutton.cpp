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

class PanelRadioButton : public BasePane, public GUI::IRadioButton, public IObserver
{
	int32 fontIndex = 1;
	char text[128];
	EventIdRef id;
	char value[128];
	int32 horzAlign = 0;
	int32 vertAlign = 0;
	Vec2i padding{ 0,0 };
	IPublisher& publisher;

	int32 stateIndex = -1; // indeterminate
public:
	PanelRadioButton(IPublisher& _publisher, int _fontIndex, cstr _text, cstr _key, cstr _value) :
		id(_publisher.CreateEventIdFromVolatileString(_key)),
		fontIndex(_fontIndex), publisher(_publisher)
	{
		SafeFormat(text, sizeof(text), "%s", _text);
		SafeFormat(value, sizeof(value), "%s", _value);

		publisher.Subscribe(this, id);
	}

	virtual ~PanelRadioButton()
	{
		publisher.Unsubscribe(this);
	}

	void OnEvent(Event& ev) override
	{
		if (id == ev)
		{
			auto& toe = As<TextOutputEvent>(ev);
			if (!toe.isGetting)
			{
				stateIndex = (toe.text && Eq(toe.text, value)) ? 1 : 0;
			}
		}
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

	bool AppendEvent(const KeyboardEventEx&, const Vec2i&, const Vec2i&) override
	{
		return false;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i&) override
	{
		if (stateIndex == 0 && me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
		{
			TextOutputEvent toe;
			toe.isGetting = false;
			SecureFormat(toe.text, sizeof(toe.text), value);
			publisher.Publish(toe, id);
		}
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		if (stateIndex == -1)
		{
			TextOutputEvent toe;
			toe.isGetting = true;
			publisher.Publish(toe, id);
			stateIndex = (toe.text && Eq(toe.text, value)) ? 1 : 0;
		}

		auto p = metrics.cursorPosition;

		GuiRect absRect = GuiRect{ 0, 0, Width(ClientRect()), Height(ClientRect()) } + topLeft;
		auto& scheme = Scheme();

		if (stateIndex != 0)
		{
			Graphics::DrawRectangle(grc, absRect, scheme.hi_topLeft, scheme.hi_bottomRight);
			Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.hi_topLeftEdge, scheme.hi_bottomRightEdge);
		}
		else
		{
			Graphics::DrawRectangle(grc, absRect, scheme.topLeft, scheme.bottomRight);
			Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.topLeftEdge, scheme.bottomRightEdge);
		}

		RenderLabel(grc, text, absRect, horzAlign, vertAlign, padding, fontIndex, scheme, !modality.isUnderModal);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		Rococo::GUI::IRadioButton* AddRadioButton(IPublisher& publisher, BasePane& panel, int32 fontIndex, const fstring& text, const fstring& key, const fstring& value, const GuiRect& rect)
		{
			auto* radio = new PanelRadioButton(publisher, fontIndex, text, key, value);
			panel.AddChild(radio);
			radio->SetRect(rect);
			return radio;
		}
	}
}