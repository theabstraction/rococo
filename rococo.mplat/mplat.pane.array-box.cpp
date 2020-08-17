#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

struct PanelArrayBox : BasePane, IArrayBox, IObserver, IEventCallback<ScrollEvent>
{
	Platform& platform;
	ID_FONT idFont;
	int32 activeIndex = -1;
	HString populateArrayEventText;
	HString itemSelectedEventText;
	EventIdRef evPopulate = { 0,0 };
	EventIdRef evItemSelected = { 0,0 };
	EventIdRef evScrollToLine = { 0,0 };
	RGBAb hi_focusColour{ 0,0,64,255 };
	RGBAb focusColour{ 0,0,48,255 };
	int32 lineHeight = 0;
	int32 fontHeight = 0;
	GuiRect borders{ 0, 0, 0, 0 };
	AutoFree<IScrollbar> vscroll;

	PanelArrayBox(Platform& _platform, int _fontIndex, cstr _populateArrayEventText) :
		platform(_platform),
		vscroll(_platform.utilities.CreateScrollbar(true))
	{
		idFont = platform.utilities.GetHQFonts().GetSysFont(Graphics::HQFont_EditorFont);
		fontHeight = lineHeight = platform.renderer.GetFontMetrics(idFont).height;
		populateArrayEventText = _populateArrayEventText;
		evPopulate = platform.publisher.CreateEventIdFromVolatileString(populateArrayEventText);
	}
	
	~PanelArrayBox()
	{
		platform.publisher.Unsubscribe(this);
	}

	void OnEvent(ScrollEvent& ev) override
	{
		vscrollPos = ev.logicalValue;
	}

	void SetLineBorders(int32 left, int32 top, int32 right, int32 bottom)
	{
		borders = GuiRect{ left, top, right, bottom };
		lineHeight = fontHeight + top + bottom;
	}

	void Free() override
	{
		delete this;
	}

	void SetItemSelectEvent(const fstring& eventText) override
	{
		itemSelectedEventText = eventText;
		evItemSelected = platform.publisher.CreateEventIdFromVolatileString(eventText);
	}

	void SetScrollToItemEvent(const fstring& eventText) override
	{
		evScrollToLine = platform.publisher.CreateEventIdFromVolatileString(eventText);
		platform.publisher.Subscribe(this, evScrollToLine);
	}

	void OnEvent(Event& ev)
	{
		if (ev == evScrollToLine)
		{
			auto& lineNumber = As<TEventArgs<int32>>(ev);
			vscrollPos = lineNumber.value * lineHeight;
			vscrollPos = min(vscrollPos, vscrollSpan - pageSize);
			vscrollPos = max(0, vscrollPos);
			this->activeIndex = lineNumber.value;
		}
	}

	void SetFocusColours(RGBAb normal, RGBAb hilight) override
	{
		focusColour = normal;
		hi_focusColour = hilight;
	}

	bool AppendEvent(const KeyboardEvent& ke, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		ScrollEvent se;
		if (vscroll->AppendEvent(ke, se))
		{
			vscrollPos = se.logicalValue;
			return true;
		}
		return false;
	}

	struct SVArgs
	{
		GuiRect lineRect;
		cstr text;
		int32 index;
	};

	void EnumerateStringVector(IStringVector& sv, const GuiRect absRect, IEventCallback<SVArgs>& eventHandler)
	{
		GuiRect lineRect = absRect;
		lineRect.top -= vscrollPos;
		for (int i = 0; i < sv.Count(); ++i)
		{
			lineRect.bottom = lineRect.top + lineHeight - 1;

			char text[256];
			sv.GetItem(i, text, sizeof text);
			SVArgs args{ lineRect, text, i };
			eventHandler.OnEvent(args);

			lineRect.top = lineRect.bottom + 1;
		}
	}

	GuiRect absRect = { 0,0,0,0 };

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		ScrollEvent se;
		if (vscroll->AppendEvent(me, absTopLeft, se))
		{
			vscrollPos = se.logicalValue;
			return;
		}

		int dz = ((int32)(short)me.buttonData) / 120;

		if (dz != 0)
		{
			if (IsPointInRect(me.cursorPos, absRect))
			{
				dz *= -1;

				vscrollPos += dz * lineHeight;
				vscrollPos = max(0, vscrollPos);
				vscrollPos = min(vscrollPos, vscrollSpan - pageSize);
			}
		}

		if (me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
		{
			int32 previousIndex = activeIndex;
			struct: IEventCallback<SVArgs>
			{
				PanelArrayBox* This;
				Vec2i pos;

				void OnEvent(SVArgs& args) override
				{
					if (IsPointInRect(pos, args.lineRect))
					{
						This->activeIndex = args.index;
					}
				}
			} routeClick;
			routeClick.This = this;
			routeClick.pos = me.cursorPos;

			T2EventArgs<IStringVector*,int32> args;
			args.value1 = nullptr;
			args.value2 = -1;
			platform.publisher.Publish(args, evPopulate);

			if (args.value1 != nullptr)
			{
				EnumerateStringVector(*args.value1, absRect, routeClick);
			}

			if (previousIndex != activeIndex)
			{
				TEventArgs<int> selectArgs;
				selectArgs.value = activeIndex;
				platform.publisher.Publish(selectArgs, evItemSelected);
			}
		}
		else if (me.HasFlag(me.LDown) || me.HasFlag(me.RDown))
		{
		}
	}

	int32 vscrollSpan = 0;
	int32 pageSize = 0;
	int32 vscrollPos = 0;

	void RenderRows(IGuiRenderContext& grc, IStringVector& sv)
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		struct : IEventCallback<SVArgs>
		{
			IGuiRenderContext* grc;
			PanelArrayBox* This;
			Vec2i cursorPos;
			ID_FONT fontId;

			void OnEvent(SVArgs& args) override
			{
				auto& b = This->borders;

				GuiRect textRect
				{
					args.lineRect.left + b.left,
					args.lineRect.top + b.top,
					args.lineRect.right - b.right,
					args.lineRect.bottom - b.bottom
				};

				bool isLit = IsPointInRect(cursorPos, args.lineRect);
				auto fcolour = isLit ? This->Scheme().hi_fontColour : This->Scheme().fontColour;

				if (args.index == This->activeIndex)
				{
					auto bcol = isLit ? This->hi_focusColour : This->focusColour;
					Rococo::Graphics::DrawRectangle(*grc, args.lineRect, bcol, bcol);
				}

				Graphics::RenderHQText_LeftAligned_VCentre(*grc, fontId, textRect, args.text, fcolour);
			}
		} renderLine;

		renderLine.fontId = idFont;
		renderLine.grc = &grc;
		renderLine.This = this;
		renderLine.cursorPos = metrics.cursorPosition;

		vscrollSpan = sv.Count() * lineHeight;
		pageSize = absRect.bottom - absRect.top;

		EnumerateStringVector(sv, absRect, renderLine);
	}

	enum { vsWidth = 24 };

	int32 lastCount = -1;
	int32 lastPageSize = -1;

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		RenderBackground(grc, topLeft, modality);

		auto span = Span(ClientRect());
		absRect = GuiRect { topLeft.x+1, topLeft.y + 1, topLeft.x + span.x - vsWidth - 1, topLeft.y + span.y };

		T2EventArgs<IStringVector*,int32> args;
		args.value1 = nullptr;
		platform.publisher.Publish(args, evPopulate);

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		if (args.value1 != nullptr)
		{
			grc.FlushLayer();
			grc.SetScissorRect(Dequantize(absRect));
			RenderRows(grc, *args.value1);
			grc.FlushLayer();
			grc.ClearScissorRect();

			int32 pageSize = Height(absRect);

			if (lastCount != args.value1->Count())
			{
				lastCount = args.value1->Count();
				lastPageSize = -1;
			}

			if (args.value1 && lastPageSize != pageSize)
			{
				lastPageSize = pageSize;

				ScrollEvent state;
				state.fromScrollbar = false;
				state.logicalMinValue = 0;
				state.logicalMaxValue = max(0, lastCount * lineHeight);
				state.logicalPageSize = pageSize;
				state.logicalValue = 0;
				state.rowSize = lineHeight;
				vscroll->SetScrollState(state);
			}
		}

		GuiRect scrollRect = absRect;
		scrollRect.left = absRect.right;
		scrollRect.right = topLeft.x + span.x - 1;

		if (vscrollSpan <= pageSize) vscrollPos = 0;

		auto& s = Scheme();

		vscroll->Render(
			grc,
			scrollRect,
			modality,
			RGBAb(128, 128, 128, 255),
			RGBAb(96, 96, 96, 255),
			RGBAb(160, 160, 160, 255),
			RGBAb(140, 140, 140, 255),
			s.hi_topLeftEdge,
			s.topLeftEdge,
			*this,
			{0,0}
		);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		IArrayBox* AddArrayBox(Platform& platform, BasePane& panel, int32 fontIndex, const fstring& populatorEventKey, const GuiRect& rect)
		{
			auto* ab = new PanelArrayBox(platform, fontIndex, populatorEventKey);
			panel.AddChild(ab);
			ab->SetRect(rect);
			return ab;
		}
	}
}