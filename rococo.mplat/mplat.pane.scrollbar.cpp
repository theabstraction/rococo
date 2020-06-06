#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

class Scrollbar : public IScrollbar
{
	int trapCount = 0;
	Vec2i grabPoint{ -1,-1 };
	bool isVertical = false;
	int32 grabPixelValue = 0;
	Vec2i span{ 0,0 };

	int32 minValue = 0;
	int32 maxValue = 0;
	int32 value = 0;
	int32 rowSize = 0;
	int32 pageSize = 0;

	int32 PixelRange() const
	{
		return isVertical ? span.y : span.x;
	}

	int32 LogicalRange() const
	{
		return maxValue - minValue;
	}

	int32 PixelSelectValue(const Vec2i& relPos) const
	{
		return (isVertical) ? relPos.y : relPos.x;
	}

	int32 LogicalValue(int32 pixelValue)
	{
		float pr = (float)PixelRange();
		return pr == 0 ? 0 : (int32)((pixelValue / pr * (float)LogicalRange())) + minValue;
	}

	void CapValue(int32 candidateValue)
	{
		if (candidateValue < minValue) value = minValue;
		else if (candidateValue + pageSize > maxValue) value = maxValue - pageSize;
		else
		{
			value = candidateValue;
		}
	}

	int32 PixelValue(int32 logicalValue)
	{
		float lr = (float)LogicalRange();
		return lr == 0 ? 0 : (int32)((logicalValue / lr) * (float)PixelRange());
	}

	int32 PageSizeInPixels()
	{
		float lr = (float)LogicalRange();
		return lr == 0 ? 0 : (int32)((pageSize / lr) * (float)PixelRange());
	}

public:
	Scrollbar(bool _isVertical) : isVertical(_isVertical)
	{
	}

	~Scrollbar()
	{
	}

	void Free() override
	{
		delete this;
	}

	void GetScrollState(ScrollEvent& s)
	{
		s.logicalMinValue = minValue;
		s.logicalMaxValue = maxValue;
		s.logicalValue = value;
		s.logicalPageSize = pageSize;
		s.rowSize = rowSize;
	}

	void SetScrollState(const ScrollEvent& s)
	{
		minValue = s.logicalMinValue;
		maxValue = s.logicalMaxValue;
		value = s.logicalValue;
		pageSize = s.logicalPageSize;
		rowSize = s.rowSize;
	}

	bool AppendEvent(const KeyboardEvent& k, ScrollEvent& updateStatus)
	{
		using namespace Rococo::IO;

		if (!k.IsUp())
		{
			bool consume = true;

			switch (k.VKey)
			{
			case VKCode_HOME:
				value = minValue;
				break;
			case VKCode_END:
				value = maxValue - pageSize;
				break;
			case VKCode_PGUP:
				value -= pageSize;
				break;
			case VKCode_PGDOWN:
				value += pageSize;
				break;
			case VKCode_UP:
				value -= rowSize;
				break;
			case VKCode_DOWN:
				value += rowSize;
				break;
			default:
				consume = false;
			}

			CapValue(value);

			if (consume)
			{
				updateStatus.logicalMinValue = minValue;
				updateStatus.logicalMaxValue = maxValue;
				updateStatus.logicalValue = value;
				updateStatus.logicalPageSize = pageSize;
				updateStatus.fromScrollbar = true;
				updateStatus.rowSize = rowSize;
			}

			return consume;
		}
		return false;
	}

	bool AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft, ScrollEvent& updateStatus) override
	{
		int32 oldValue = value;

		if (me.HasFlag(me.LDown))
		{
			Vec2i ds = me.cursorPos - absTopLeft;
			int32 pixelValue = PixelSelectValue(ds);
			if (pixelValue < PixelValue(value))
			{
				value -= pageSize;
			}
			else if (pixelValue > PixelValue(value + pageSize))
			{
				value += pageSize;
			}
			else
			{
				grabPoint = me.cursorPos;
				grabPixelValue = PixelValue(value);
			}

			CapValue(value);
		}

		if (me.HasFlag(me.RDown))
		{
			Vec2i ds = me.cursorPos - absTopLeft;
			int32 pixelValue = PixelSelectValue(ds);
			if (pixelValue < PixelValue(value))
			{
				value -= rowSize;
			}
			else if (pixelValue > PixelValue(value + pageSize))
			{
				value += rowSize;
			}

			CapValue(value);
		}

		if (me.HasFlag(me.MouseWheel))
		{
			int32 delta = (int32)(((short)me.buttonData) / 120);
			value -= rowSize * delta;
			CapValue(value);
		}

		if (oldValue != value)
		{
			updateStatus.logicalMinValue = minValue;
			updateStatus.logicalMaxValue = maxValue;
			updateStatus.logicalValue = value;
			updateStatus.logicalPageSize = pageSize;
			updateStatus.fromScrollbar = true;
			updateStatus.rowSize = rowSize;
			return true;
		}

		if (me.HasFlag(me.LUp))
		{
			grabPoint = { -1,-1 };
			grabPixelValue = -1;
		}

		return false;
	}


	void Render(IGuiRenderContext& grc, const GuiRect& absRect, const Modality& modality, RGBAb hilightColour, RGBAb baseColour, RGBAb hilightEdge, RGBAb baseEdge, IEventCallback<ScrollEvent>& populator, const EventIdRef& populationEventId)
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		span = Span(absRect);

		if (LogicalRange() == 0)
		{
			ScrollEvent ev;
			ev.fromScrollbar = false;
			populator.OnEvent(ev);

			maxValue = ev.logicalMaxValue;
			minValue = ev.logicalMinValue;
			value = ev.logicalValue;
			pageSize = ev.logicalPageSize;
			rowSize = ev.rowSize;
		}

		bool lit = !modality.isUnderModal && IsPointInRect(metrics.cursorPosition, absRect);

		if (lit)
		{
			trapCount = 0;

			if (grabPoint.x >= 0)
			{
				Vec2i delta = metrics.cursorPosition - grabPoint;
				int32 dPixels = isVertical ? delta.y : delta.x;
				int32 pixelPos = grabPixelValue + dPixels;
				int32 newValue = LogicalValue(pixelPos);

				if (dPixels != 0)
				{
					if (newValue < minValue)
					{
						value = minValue;
						grabPoint = metrics.cursorPosition;
						grabPixelValue = 0;
					}
					else if (newValue + pageSize > maxValue)
					{
						value = maxValue - pageSize;
						grabPoint = metrics.cursorPosition;
						grabPixelValue = PixelValue(value);
					}
					else
					{
						value = newValue;
					}

					ScrollEvent s;
					s.logicalMinValue = minValue;
					s.logicalMaxValue = maxValue;
					s.logicalValue = value;
					s.logicalPageSize = pageSize;
					s.fromScrollbar = true;
					s.rowSize = rowSize;

					populator.OnEvent(s);
				}
			}
		}
		else
		{
			trapCount++;
			if (trapCount > 50)
			{
				trapCount = 0;
				grabPoint = { -1,-1 };
			}
		}

		Graphics::DrawRectangle(grc, absRect, lit ? hilightColour : baseColour, lit ? hilightColour : baseColour);
		Graphics::DrawBorderAround(grc, absRect, { 1,1 }, lit ? hilightEdge : baseEdge, lit ? hilightEdge : baseEdge);

		GuiRect sliderRect;

		if (isVertical)
		{
			sliderRect = GuiRect{ 1, 1, span.x - 2, PageSizeInPixels() } + Vec2i{ 0, PixelValue(value) } + TopLeft(absRect);
		}
		else
		{
			sliderRect = GuiRect{ 1, 1, PageSizeInPixels(), span.y - 2 } + Vec2i{ PixelValue(value), 0 } + TopLeft(absRect);
		}

		Graphics::DrawRectangle(grc, sliderRect, lit ? hilightColour : baseColour, lit ? hilightColour : baseColour);
		Graphics::DrawBorderAround(grc, sliderRect, { 1,1 }, lit ? hilightEdge : baseEdge, lit ? hilightEdge : baseEdge);
	}
};

class PanelScrollbar : public BasePane, public IScroller, IObserver, IEventCallback<ScrollEvent>
{
	Scrollbar scrollbar;
	IPublisher& publisher;
	EventIdRef setScrollId;
	EventIdRef getScrollId;
	EventIdRef uiScrollId;
	EventIdRef routeKeyId;
	EventIdRef routeMouseId;

	void OnEvent(Event& ev) override
	{
		if (ev == setScrollId)
		{
			auto& s = As<ScrollEvent>(ev);
			scrollbar.SetScrollState(s);
		}
		else if (ev == getScrollId)
		{
			auto& s = As<ScrollEvent>(ev);
			scrollbar.GetScrollState(s);
		}
		else if (ev == routeKeyId)
		{
			auto& r = As<RouteKeyboardEvent>(ev);

			Events::ScrollEvent se;
			if (scrollbar.AppendEvent(*r.ke, se))
			{
				r.consume = true;
				publisher.Publish(se, uiScrollId);
			}
		}
		else if (ev == routeMouseId)
		{
			auto& r = As<RouteMouseEvent>(ev);

			Events::ScrollEvent se;
			if (scrollbar.AppendEvent(*r.me, r.absTopleft, se))
			{
				publisher.Publish(se, uiScrollId);
			}
		}
	}

	void OnEvent(ScrollEvent& se)
	{
		publisher.Publish(se, uiScrollId);
	}
public:
	PanelScrollbar(IPublisher& _publisher, cstr _key, boolean32 isVertical) :
		scrollbar(isVertical), publisher(_publisher),
		setScrollId(""_event), getScrollId(""_event), uiScrollId(""_event), routeKeyId(""_event), routeMouseId(""_event)
	{
		char eventText[256];

		{
			SecureFormat(eventText, sizeof(eventText), "%s_set", _key);
			EventIdRef id = publisher.CreateEventIdFromVolatileString(eventText);
			memcpy(&setScrollId, &id, sizeof(id));
		}

		{
			SecureFormat(eventText, sizeof(eventText), "%s_get", _key);
			EventIdRef id = publisher.CreateEventIdFromVolatileString(eventText);
			memcpy(&getScrollId, &id, sizeof(id));
		}

		{
			SecureFormat(eventText, sizeof(eventText), "%s_ui", _key);
			EventIdRef id = publisher.CreateEventIdFromVolatileString(eventText);
			memcpy(&uiScrollId, &id, sizeof(id));
		}

		{
			SecureFormat(eventText, sizeof(eventText), "%s_sendkey", _key);
			EventIdRef id = publisher.CreateEventIdFromVolatileString(eventText);
			memcpy(&routeKeyId, &id, sizeof(id));
		}

		{
			SecureFormat(eventText, sizeof(eventText), "%s_sendmouse", _key);
			EventIdRef id = publisher.CreateEventIdFromVolatileString(eventText);
			memcpy(&routeMouseId, &id, sizeof(id));
		}

		publisher.Subscribe(this, getScrollId);
		publisher.Subscribe(this, setScrollId);
		publisher.Subscribe(this, routeKeyId);
		publisher.Subscribe(this, routeMouseId);
	}

	~PanelScrollbar()
	{
		publisher.Unsubscribe(this);
	}

	void Free() override
	{
		delete this;
	}

	bool AppendEvent(const KeyboardEvent& k, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		ScrollEvent updateStatus;
		updateStatus.fromScrollbar = true;
		if (scrollbar.AppendEvent(k, updateStatus))
		{
			publisher.Publish(updateStatus, uiScrollId);
			return true;
		}

		return false;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		ScrollEvent updateStatus;
		updateStatus.fromScrollbar = true;
		if (scrollbar.AppendEvent(me, absTopLeft, updateStatus))
		{
			publisher.Publish(updateStatus, uiScrollId);
		}
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		auto span = Span(ClientRect());
		GuiRect absRect = GuiRect{ 0, 0, span.x, span.y } + topLeft;
		scrollbar.Render(grc, absRect, modality, Scheme().hi_topLeft, Scheme().topLeft, Scheme().hi_topLeftEdge, Scheme().topLeftEdge, *this, uiScrollId);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		Rococo::IScroller* AddScroller(IPublisher& publisher, BasePane& panel, const fstring& key, const GuiRect& rect, boolean32 isVertical)
		{
			auto* scroller = new PanelScrollbar(publisher, key, isVertical);
			panel.AddChild(scroller);
			scroller->SetRect(rect);
			return scroller;
		}

		IScrollbar* CreateScrollbar(bool _isVertical)
		{
			return new Scrollbar(_isVertical);
		}
	}
}