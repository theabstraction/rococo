#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>
#include <rococo.hashtable.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

class EnumListPane : public BasePane, public IEnumListPane, public IEventCallback<ScrollEvent>
{
	Platform& platform; // make platform first member to ensure correct compilation of constructor
	int32 fontIndex = 1;
	HString populateId;
	int32 horzAlign = 0;
	int32 vertAlign = 0;
	Vec2i padding{ 0,0 };
	EventIdRef evPopulate;
	stringmap<int32> categoriesByName;
	std::unordered_map<int32,HString> categoriesById;
	int32 minCatId = 0x7FFFFFFF;
	int32 maxCatId = 0x80000000;

	RGBAb normalArrowColour{ 224,224,224,255 };
	RGBAb hilightArrowColour{ 255,255,255,255 };
	RGBAb disabledArrowColour{ 224,224,224, 128 };

	RGBAb focusColour1{ 0,0,64,255 };
	RGBAb focusColour2{ 0,0,96,255 };

	EventIdRef evScrollPopulate;
	AutoFree<IScrollbar> vscroll;

	int32 vPos = 0;
public:
	EnumListPane(Platform& _platform, int _fontIndex, cstr _populateId) :
		fontIndex(_fontIndex), platform(_platform), populateId(_populateId),
		evPopulate(platform.publisher.CreateEventIdFromVolatileString(_populateId)),
		vscroll(platform.utilities.CreateScrollbar(true))
	{
		char scrollPopulateText[256];
		SafeFormat(scrollPopulateText, sizeof scrollPopulateText, "%s.vscroll.populate", _populateId);
		evScrollPopulate = platform.publisher.CreateEventIdFromVolatileString(scrollPopulateText);
	}

	void Free() override
	{
		delete this;
	}

	void DecrementEnum(int32 id, int32 enumValue)
	{
		int32 newValue = enumValue-1;

		for (auto& i : categoriesById)
		{
			int32 catValue = i.first;
			if (catValue < enumValue)
			{
				newValue = max(newValue, catValue);
			}
		}

		SetValue(id, newValue);
	}

	void IncrementEnum(int32 id, int32 enumValue)
	{
		int32 newValue = enumValue+1;

		for (auto& i : categoriesById)
		{
			int32 catValue = i.first;
			if (catValue > enumValue)
			{
				newValue = min(newValue, catValue);
			}
		}

		SetValue(id, newValue);
	}

	void SetValue(int32 id, int32 value)
	{
		if (categoriesById.find(value) != categoriesById.end())
		{
			TEventArgs<IEnumVector*> args;
			args.value = nullptr;
			platform.publisher.Publish(args, evPopulate);
			if (args.value)
			{
				args.value->SetValue(id, value);
			}
		}
	}

	void OnEvent(ScrollEvent& ev) override
	{
		this->vPos = ev.logicalValue;
	}

	void MakeActive(int32 id)
	{
		TEventArgs<IEnumVector*> args;
		args.value = nullptr;
		platform.publisher.Publish(args, evPopulate);
		if (args.value)
		{
			args.value->SetActiveIndex(id);
		}
	}

	void SetArrowColours(RGBAb normal, RGBAb hilighted, RGBAb disabled) override
	{
		normalArrowColour = normal;
		hilightArrowColour = hilighted;
		disabledArrowColour = disabled;
	}

	void SetFocusColours(RGBAb col1, RGBAb col2) override
	{
		focusColour1 = col1;
		focusColour2 = col2;
	}

	GuiRect lastAbsRect{ -1, -1, -1, -1 };

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		ScrollEvent se;
		if (vscroll->AppendEvent(me, TopLeft(lastAbsRect), se))
		{
			vPos = se.logicalValue;
			return;
		}

		if (me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
		{
			struct ANON : public IEventCallback<const ItemCallbackArgs>
			{
				EnumListPane* This;
				Vec2i pos;

				void OnEvent(const ItemCallbackArgs& args) override
				{
					if (IsPointInRect(pos, args.lButtonRect))
					{
						This->DecrementEnum(args.id, args.value);
					}
					else if (IsPointInRect(pos, args.rButtonRect))
					{
						This->IncrementEnum(args.id, args.value);
					}
					else if (IsPointInRect(pos, args.itemRect))
					{
						This->MakeActive(args.id);
					}
				}
			} routeClick;
			routeClick.This = this;
			routeClick.pos = me.cursorPos;
			ForEachItem(routeClick, lastEnumRect);
		}
		else if (me.HasFlag(me.LDown) || me.HasFlag(me.RDown))
		{
		}
	}

	bool AppendEvent(const KeyboardEvent& ke, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		ScrollEvent se;
		if (vscroll->AppendEvent(ke, se))
		{
			vPos = se.logicalValue;
			return true;
		}
		else
		{
			return false;
		}
	}

	void AddEnumCategory(const fstring& key, int32 value) override
	{
		categoriesByName[(cstr)key] = value;
		categoriesById[value] = key;
		minCatId = min(minCatId, value);
		maxCatId = max(maxCatId, value);
	}

	struct ItemCallbackArgs
	{
		GuiRect itemRect;
		GuiRectf textRect;
		GuiRect lButtonRect;
		GuiRect rButtonRect;
		int32 value;
		int32 id;
		int32 activeId;
	};

	void DrawLabel(IGuiRenderContext& grc, const ItemCallbackArgs& args, cstr text)
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		if (args.activeId == args.id)
		{
			Graphics::DrawRectangle(grc, args.itemRect, focusColour1, focusColour2);
		}

		bool isLArrowLit = IsPointInRect(metrics.cursorPosition, args.lButtonRect);
		RGBAb arrowColourL = isLArrowLit ? hilightArrowColour : normalArrowColour;
		if (args.value <= minCatId) arrowColourL = disabledArrowColour;	
		Graphics::DrawTriangleFacingLeft(grc, args.lButtonRect, arrowColourL);

		bool isRArrayLit = IsPointInRect(metrics.cursorPosition, args.rButtonRect);
		RGBAb arrowColourR = isRArrayLit ? hilightArrowColour : normalArrowColour;
		if (args.value >= maxCatId) arrowColourR = disabledArrowColour;
		Graphics::DrawTriangleFacingRight(grc, args.rButtonRect, arrowColourR);

		bool isLit = IsPointInRect(metrics.cursorPosition, args.itemRect);
		RGBAb fontColour = isLit ? Scheme().hi_fontColour : Scheme().fontColour;
		Graphics::DrawText(grc, args.textRect, Graphics::Alignment_Left, to_fstring(text), fontIndex, fontColour);
	}

	GuiRect lastEnumRect = { -1,-1,-1,-1};

	enum { CELL_HEIGHT = 24 };

	void ForEachItem(IEventCallback<const ItemCallbackArgs>& cb, const GuiRect& enumRect)
	{
		lastEnumRect = enumRect;

		TEventArgs<IEnumVector*> args;
		args.value = nullptr;
		platform.publisher.Publish(args, evPopulate);

		if (args.value == nullptr) return;

		auto& enums = *args.value;

		ItemCallbackArgs rects;
		rects.itemRect = { enumRect.left, enumRect.top, enumRect.right, enumRect.top - vPos };

		rects.activeId = args.value->GetActiveIndex();

		enum 
		{ 
			LBORDER = 20, 
			RBORDER = 20, 
			ARROW_SPAN = 16,
			BUTTON_BORDER_TOP = 4,
			BUTTON_BORDER_BOTTOM = 4
		};

		for (int32 i = 0; i < enums.Count(); ++i)
		{
			int32 value = enums[i];
			rects.itemRect.top = rects.itemRect.bottom;
			rects.itemRect.bottom += CELL_HEIGHT;

			if (rects.itemRect.bottom > enumRect.top && rects.itemRect.top < enumRect.bottom)
			{
				rects.id = i;

				rects.textRect =
				{
					(float)rects.itemRect.left + LBORDER,
					(float)rects.itemRect.top,
					(float)rects.itemRect.right - RBORDER,
					(float)rects.itemRect.bottom,
				};

				GuiRect lButton = rects.itemRect;
				lButton.right = lButton.left + ARROW_SPAN;
				lButton.top = rects.itemRect.top + BUTTON_BORDER_TOP;
				lButton.bottom = rects.itemRect.bottom - BUTTON_BORDER_BOTTOM;

				GuiRect rButton = rects.itemRect;
				rButton.right = rects.itemRect.right;
				rButton.left = rects.itemRect.right - ARROW_SPAN;
				lButton.top = rects.itemRect.top + BUTTON_BORDER_TOP;
				lButton.bottom = rects.itemRect.bottom - BUTTON_BORDER_BOTTOM;

				rects.lButtonRect = lButton;
				rects.rButtonRect = rButton;

				rects.value = value;

				cb.OnEvent(rects);
			}
		}
	}

	int32 lastPageSize = -1;
	int lastCount = -1;

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		auto span = Span(ClientRect());
		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

		lastAbsRect = absRect;

		TEventArgs<IEnumVector*> args;
		args.value = nullptr;
		platform.publisher.Publish(args, evPopulate);

		int32 pageSize = Height(absRect);

		if (args.value)
		{
			if (lastCount != args.value->Count())
			{
				lastCount = args.value->Count();
				lastPageSize = -1;
			}
		}

		if (args.value && lastPageSize != pageSize)
		{
			lastPageSize = pageSize;

			auto& enums = *args.value;

			ScrollEvent state;
			state.fromScrollbar = false;
			state.logicalMinValue = 0;
			state.logicalMaxValue = max(0, enums.Count() * CELL_HEIGHT);
			state.logicalPageSize = pageSize;
			state.logicalValue = 0;
			state.rowSize = CELL_HEIGHT;
			vscroll->SetScrollState(state);
		}

		bool isLit = IsPointInRect(metrics.cursorPosition, absRect);

		auto& s = Scheme();

		RGBAb bkColour1 = !isLit ? s.bottomRight : s.hi_bottomRight;
		RGBAb bkColour2 = !isLit ? s.topLeft : s.hi_topLeft;

		int32 vscrollLeft = absRect.right - 24;

		GuiRect enumRect = absRect;
		enumRect.right = vscrollLeft - 2;

		Graphics::DrawRectangle(grc, absRect, bkColour1, bkColour2);

		grc.FlushLayer();
		grc.SetScissorRect(Dequantize(enumRect));

		struct ANON : IEventCallback<const ItemCallbackArgs>
		{
			EnumListPane* This;
			IGuiRenderContext* grc;
			void OnEvent(const ItemCallbackArgs& args) override
			{
				auto itCat = This->categoriesById.find(args.value);
				if (itCat == This->categoriesById.end())
				{
					char buf[64];
					SafeFormat(buf, sizeof buf, "Unknown enum [%d]", args.value);
					This->DrawLabel(*grc, args, buf);
				}
				else
				{
					This->DrawLabel(*grc, args, itCat->second.c_str());
				}
			}
		} renderAll;

		renderAll.grc = &grc;
		renderAll.This = this;

		ForEachItem(renderAll, enumRect);

		grc.FlushLayer();
		grc.SetScissorRect(GuiRectf{ 0, 0, (float) metrics.screenSpan.x, (float) metrics.screenSpan.y });

		GuiRect scrollRect =
		{
			vscrollLeft,
			absRect.top + 1,
			absRect.right - 1,
			absRect.bottom - 1
		};

		vscroll->Render(
			grc,
			scrollRect,
			modality,
			RGBAb(128, 128, 128,255),
			RGBAb(96,96,96,255),
			RGBAb(160, 160, 160, 255),
			RGBAb(140, 140, 140, 255),
			s.hi_topLeftEdge,
			s.topLeftEdge,
			*this,
			evScrollPopulate
		);

		RGBAb edgeColour1 = isLit ? s.bottomRightEdge : s.hi_bottomRightEdge;
		RGBAb edgeColour2 = isLit ? s.topLeftEdge : s.hi_topLeftEdge;
		Graphics::DrawBorderAround(grc, absRect, { 1,1 }, edgeColour1, edgeColour2);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		Rococo::IEnumListPane* AddEnumList(Platform& platform, BasePane& panel, int32 fontIndex, const fstring& populateId, const GuiRect& rect)
		{
			auto* enumList = new EnumListPane(platform, fontIndex, populateId);
			panel.AddChild(enumList);
			enumList->SetRect(rect);
			return enumList;
		}
	}
}