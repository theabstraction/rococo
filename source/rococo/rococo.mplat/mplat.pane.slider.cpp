#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Strings;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

class PanelSlider : public BasePane, public GUI::ISlider
{
	int32 fontIndex = 1;
	char text[128];
	IPublisher& publisher;
	IRenderer& renderer;
	float minValue;
	float maxValue;
	float value;
public:
	PanelSlider(IPublisher& _publisher, IRenderer& _renderer, int _fontIndex, cstr _text, float _minValue, float _maxValue) :
		publisher(_publisher), renderer(_renderer), fontIndex(_fontIndex), minValue(_minValue), maxValue(_maxValue)
	{
		SafeFormat(text, sizeof(text), "%s", _text);
		value = 0.5f * (maxValue + minValue);
	}

	void Free() override
	{
		delete this;
	}

	bool AppendEvent(const KeyboardEvent&, const Vec2i&, const Vec2i&) override
	{
		return false;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		if (me.HasFlag(me.LUp))
		{
			GuiMetrics metrics;
			renderer.GetGuiMetrics(metrics);

			auto ds = metrics.cursorPosition - absTopLeft;

			int32 width = Width(ClientRect());

			float delta = ds.x / (float)width;

			value = delta * (maxValue - minValue);
		}
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		auto& controlRect = ClientRect();

		float fspan = maxValue - minValue;
		float delta = (value - minValue) / fspan;

		int32 right = (int32)(delta * (controlRect.right - controlRect.left));

		GuiRect sliderRect
		{
		   topLeft.x,
		   topLeft.y,
		   topLeft.x + right,
		   topLeft.y + Height(controlRect)
		};

		auto span = Span(ClientRect());
		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		bool lit = !modality.isUnderModal && IsPointInRect(metrics.cursorPosition, absRect);
		Graphics::DrawRectangle(grc, sliderRect, lit ? Scheme().hi_topLeft : Scheme().topLeft, lit ? Scheme().hi_bottomRight : Scheme().bottomRight);

		char fullText[256];
		SafeFormat(fullText, sizeof(fullText), "%s - %0.0f%%", text, delta * 100.0f);

		RenderLabel(grc, fullText, absRect, 0, 0, { 0,0 }, fontIndex, BasePane::Scheme(), !modality.isUnderModal);

		Graphics::DrawBorderAround(grc, absRect, { 1,1 }, lit ? Scheme().hi_topLeftEdge : Scheme().topLeftEdge, lit ? Scheme().hi_bottomRightEdge : Scheme().bottomRightEdge);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		Rococo::GUI::ISlider* AddSlider(IPublisher& publisher, IRenderer& renderer, BasePane& panel, int32 fontIndex, const fstring& text, const GuiRect& rect, float minValue, float maxValue)
		{
			auto* s = new PanelSlider(publisher, renderer, fontIndex, text, minValue, maxValue);
			panel.AddChild(s);
			s->SetRect(rect);
			return s;
		}
	}
}