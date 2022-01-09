#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

class PanelTextOutput : virtual public ITextOutputPane, virtual public BasePane
{
	int32 fontIndex = 1;
	int32 horzAlign = 0;
	int32 vertAlign = 0;
	Vec2i padding{ 0,0 };
	IPublisher& publisher;
	EventIdRef id;
	std::string activateKey;
public:
	PanelTextOutput(IPublisher& _publisher, int _fontIndex, cstr _key) :
		id(publisher.CreateEventIdFromVolatileString(_key)),
		publisher(_publisher), fontIndex(_fontIndex)
	{
	}

	~PanelTextOutput()
	{
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

	void SetActivateKey(const fstring& key) override
	{
		activateKey = key;
	}

	bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		return false;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		if (activateKey.empty())
			return;

		if (me.HasFlag(MouseEvent::Flags::RUp))
		{
			static auto emitEventId = "mplat.pane.textout.click"_event;

			TextOutputEvent getTextValue;
			getTextValue.isGetting = true;
			*getTextValue.text = 0;
			publisher.Publish(getTextValue, id);

			TextOutputClickedEvent emitValue;
			emitValue.key = this->activateKey.c_str();
			emitValue.value = getTextValue.text;
			publisher.Publish(emitValue, emitEventId);
		}
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		RenderBackground(grc, topLeft, modality);

		auto span = Span(ClientRect());
		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

		TextOutputEvent event;
		event.isGetting = true;
		*event.text = 0;
		publisher.Publish(event, id);

		RenderLabel(grc, event.text, absRect, horzAlign, vertAlign, padding, fontIndex, Scheme(), !modality.isUnderModal);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		Rococo::ITextOutputPane* AddTextOutput(IPublisher& publisher, BasePane& panel, int32 fontIndex, const fstring& eventKey, const GuiRect& rect)
		{
			auto* to = new PanelTextOutput(publisher, fontIndex, eventKey);
			panel.AddChild(to);
			to->SetRect(rect);
			return to;
		}
	}
}