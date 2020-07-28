#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

struct PanelArrayBox : public BasePane, public IArrayBox, public IObserver
{
	int32 fontIndex = 1;
	int32 activeIndex = -1;
	HString populateArrayEventText;
	HString itemSelectedEventText;
	EventIdRef evPopulate = { 0,0 };
	EventIdRef evItemSelected = { 0,0 };
	EventIdRef evScrollToLine = { 0,0 };
	IPublisher& publisher;
	RGBAb hi_focusColour{ 0,0,64,255 };
	RGBAb focusColour{ 0,0,48,255 };
	int32 lineHeight = 20;
	GuiRect borders{ 0, 0, 0, 0 };

	PanelArrayBox(IPublisher& _publisher, int _fontIndex, cstr _populateArrayEventText) :
		fontIndex(_fontIndex), publisher(_publisher)
	{
		populateArrayEventText = _populateArrayEventText;
		evPopulate = publisher.CreateEventIdFromVolatileString(populateArrayEventText);
	}
	
	~PanelArrayBox()
	{
		publisher.Unsubscribe(this);
	}

	void SetLineHeight(int32 pixels)
	{
		lineHeight = pixels;
	}

	void SetLineBorders(int32 left, int32 top, int32 right, int32 bottom)
	{
		borders = GuiRect{ left, top, right, bottom };
	}

	void Free() override
	{
		delete this;
	}

	void SetItemSelectEvent(const fstring& eventText) override
	{
		itemSelectedEventText = eventText;
		evItemSelected = publisher.CreateEventIdFromVolatileString(eventText);
	}

	void SetScrollToItemEvent(const fstring& eventText) override
	{
		evScrollToLine = publisher.CreateEventIdFromVolatileString(eventText);
		publisher.Subscribe(this, evScrollToLine);
	}

	void OnEvent(Event& ev)
	{
		if (ev == evScrollToLine)
		{
			auto& lineNumber = As<TEventArgs<int32>>(ev);
			vscrollPos = lineNumber.value * lineHeight;
			vscrollPos = max(0, vscrollPos);
			vscrollPos = min(vscrollPos, vscrollSpan - pageSize);
			this->activeIndex = lineNumber.value;
		}
	}

	void SetFocusColours(RGBAb normal, RGBAb hilight) override
	{
		focusColour = normal;
		hi_focusColour = hilight;
	}

	bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
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
			if (IsPointInRect(me.cursorPos, vscroller.upButton))
			{
				vscrollPos -= lineHeight;
				vscrollPos = max(0, vscrollPos);
				return;
			}
			else if (IsPointInRect(me.cursorPos, vscroller.downButton))
			{
				vscrollPos += lineHeight;
				vscrollPos = min(vscrollPos, vscrollSpan - pageSize);
				return;
			}
			else if (IsPointInRect(me.cursorPos, vscroller.back))
			{
				if (me.cursorPos.y > vscroller.upButton.bottom && me.cursorPos.y < vscroller.slider.top)
				{
					vscrollPos -= pageSize;
					vscrollPos = max(0, vscrollPos);
				}
				else if (me.cursorPos.y > vscroller.slider.bottom && me.cursorPos.y < vscroller.downButton.top)
				{
					vscrollPos += pageSize;
					vscrollPos = min(vscrollPos, vscrollSpan - pageSize);
				}
				return;
			}

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
			publisher.Publish(args, evPopulate);

			if (args.value1 != nullptr)
			{
				EnumerateStringVector(*args.value1, absRect, routeClick);
			}

			if (previousIndex != activeIndex)
			{
				TEventArgs<int> selectArgs;
				selectArgs.value = activeIndex;
				publisher.Publish(selectArgs, evItemSelected);
			}
		}
		else if (me.HasFlag(me.LDown) || me.HasFlag(me.RDown))
		{
		}
	}

	void RenderHQText_LeftAligned_VCentre(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour)
	{
		Vec2i span = grc.RenderHQText(fontId, text, { 0,0 }, RGBAb(0,0,0,0));
		Vec2i pos{ rect.left, (rect.top + rect.bottom + span.y) >> 1 };
		grc.RenderHQText(fontId, text, pos, colour);
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

				This->RenderHQText_LeftAligned_VCentre(*grc, fontId, textRect, args.text, fcolour);
			}
		} renderLine;

		renderLine.fontId = ID_FONT(fontIndex);
		renderLine.grc = &grc;
		renderLine.This = this;
		renderLine.cursorPos = metrics.cursorPosition;

		vscrollSpan = sv.Count() * lineHeight;
		pageSize = absRect.bottom - absRect.top;

		EnumerateStringVector(sv, absRect, renderLine);
	}

	struct VScroller
	{
		GuiRect upButton;
		GuiRect back;
		GuiRect slider;
		GuiRect downButton;
	};

	int32 vsWidth = 20;

	void GetVScrollerRect(VScroller& vscroller, const GuiRect& rect)
	{
		vscroller.back = rect;
		vscroller.upButton = rect;
		vscroller.upButton.bottom = rect.top + vsWidth;
		vscroller.upButton.left += 2;
		vscroller.upButton.right -= 2;
		vscroller.upButton.top += 2;
		vscroller.downButton = rect;
		vscroller.downButton.top = rect.bottom - vsWidth;
		vscroller.downButton.left += 2;
		vscroller.downButton.right -= 2;
		vscroller.downButton.bottom -= 2;

		int32 sliderHeight = 0;
		int32 sliderStartPos = 0;
		if (vscrollSpan > 0)
		{
			sliderHeight = (int)((pageSize / (float)vscrollSpan) * (float)(vscroller.downButton.top - vscroller.upButton.bottom));
			sliderStartPos = (int)((vscrollPos / (float)vscrollSpan) * (float)(vscroller.downButton.top - vscroller.upButton.bottom));
		}

		vscroller.slider.top = vscroller.upButton.bottom + 1 + sliderStartPos;
		vscroller.slider.bottom = vscroller.slider.top + sliderHeight;
		vscroller.slider.left = rect.left + 2;
		vscroller.slider.right = rect.right - 2;
	}

	void RenderVScroller(IGuiRenderContext& grc, const VScroller& scroller, Vec2i cursorPos)
	{
		bool isUpLit = IsPointInRect(cursorPos, scroller.upButton);
		Graphics::DrawTriangleFacingUp(grc, scroller.upButton, isUpLit ? RGBAb(255, 255, 255, 255) : RGBAb(224, 224, 224, 255));

		bool isDownLit = IsPointInRect(cursorPos, scroller.downButton);
		Graphics::DrawTriangleFacingDown(grc, scroller.downButton, isDownLit ? RGBAb(255, 255, 255, 255) : RGBAb(224, 224, 224, 255));

		if (vscrollSpan > pageSize)
		{
			bool isSliderLit = IsPointInRect(cursorPos, scroller.slider);
			RGBAb col1 = isSliderLit ? RGBAb(200, 100, 100, 255) : RGBAb(200, 100, 100, 128);
			RGBAb col2 = isSliderLit ? RGBAb(160, 120, 120, 255) : RGBAb(160, 120, 120, 128);
			Graphics::DrawRectangle(grc, scroller.slider, col1, col2);

			RGBAb b1 = isSliderLit ? RGBAb(255, 255, 255, 255) : RGBAb(224, 224, 224, 255);
			RGBAb b2 = isSliderLit ? RGBAb(224, 224, 224, 255) : RGBAb(200, 200, 200, 255);
			Graphics::DrawBorderAround(grc, scroller.slider, { 1,1 }, b1, b2);
		}

		bool isLit = IsPointInRect(cursorPos, scroller.back);
		RGBAb b1 = isLit ? RGBAb(255, 255, 255, 255) : RGBAb(224, 224, 224, 255);
		RGBAb b2 = isLit ? RGBAb(224, 224, 224, 255) : RGBAb(200, 200, 200, 255);
		Graphics::DrawBorderAround(grc, scroller.back, { 1,1 }, b1, b2);
	}

	VScroller vscroller = { };

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		RenderBackground(grc, topLeft, modality);

		auto span = Span(ClientRect());
		absRect = GuiRect { topLeft.x+1, topLeft.y + 1, topLeft.x + span.x - vsWidth - 1, topLeft.y + span.y };

		T2EventArgs<IStringVector*,int32> args;
		args.value1 = nullptr;
		publisher.Publish(args, evPopulate);

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		if (args.value1 != nullptr)
		{
			grc.FlushLayer();
			grc.SetScissorRect(Dequantize(absRect));
			RenderRows(grc, *args.value1);
			grc.FlushLayer();
			grc.SetScissorRect(GuiRectf{ 0, 0, 1000000.0f, 1000000.0f });
		}

		GuiRect scrollerRect = absRect;
		scrollerRect.left = absRect.right;
		scrollerRect.right = topLeft.x + span.x - 1;

		if (vscrollSpan <= pageSize) vscrollPos = 0;

		GetVScrollerRect(vscroller, scrollerRect);

		RenderVScroller(grc, vscroller, metrics.cursorPosition);
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