#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>
#include <unordered_map>
#include <string>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

class EnumListPane : public BasePane, public IEnumListPane
{
	int32 fontIndex = 1;
	std::string populateId;
	int32 horzAlign = 0;
	int32 vertAlign = 0;
	Vec2i padding{ 0,0 };
	IPublisher& publisher;
	EventIdRef evPopulate;
	std::unordered_map<std::string, int32> categoriesByName;
	std::unordered_map<int32, std::string> categoriesById;
	int32 minCatId = 0x7FFFFFFF;
	int32 maxCatId = 0x80000000;

	RGBAb normalArrowColour{ 224,224,224,255 };
	RGBAb hilightArrowColour{ 255,255,255,255 };
	RGBAb disabledArrowColour{ 224,224,224, 128 };

	RGBAb focusColour1{ 0,0,64,255 };
	RGBAb focusColour2{ 0,0,96,255 };
public:
	EnumListPane(IPublisher& _publisher, int _fontIndex, cstr _populateId) : 
		fontIndex(_fontIndex), publisher(_publisher), populateId(_populateId),
		evPopulate(publisher.CreateEventIdFromVolatileString(_populateId))
	{
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
			args.data = nullptr;
			publisher.Publish(args, evPopulate);
			if (args.data)
			{
				args.data->SetValue(id, value);
			}
		}
	}

	void MakeActive(int32 id)
	{
		TEventArgs<IEnumVector*> args;
		args.data = nullptr;
		publisher.Publish(args, evPopulate);
		if (args.data)
		{
			args.data->SetActiveIndex(id);
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

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		if (me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
		{
			struct ANON : public IEventCallback<const ItemCallbackArgs>
			{
				EnumListPane* This;
				Vec2i pos;
				int32 id = 0;
				void OnEvent(const ItemCallbackArgs& args) override
				{
					if (IsPointInRect(pos, args.lButtonRect))
					{
						This->DecrementEnum(id, args.value);
					}
					else if (IsPointInRect(pos, args.rButtonRect))
					{
						This->IncrementEnum(id, args.value);
					}
					else if (IsPointInRect(pos, args.itemRect))
					{
						This->MakeActive(id);
					}

					id++;
				}
			} routeClick;
			routeClick.This = this;
			routeClick.pos = me.cursorPos;
			ForEachItem(routeClick, lastPos);
		}
		else if (me.HasFlag(me.LDown) || me.HasFlag(me.RDown))
		{
		}
	}

	bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		return false;
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

	Vec2i lastPos{ 0,0 };

	void ForEachItem(IEventCallback<const ItemCallbackArgs>& cb, Vec2i topLeft)
	{
		lastPos = topLeft;

		auto span = Span(ClientRect());
		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

		TEventArgs<IEnumVector*> args;
		args.data = nullptr;
		publisher.Publish(args, evPopulate);

		if (args.data == nullptr) return;

		auto& enums = *args.data;

		enum { CELL_HEIGHT = 24 };

		ItemCallbackArgs rects;
		rects.itemRect = { absRect.left, absRect.top, absRect.right, absRect.top + CELL_HEIGHT };

		rects.activeId = args.data->GetActiveIndex();

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

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		auto span = Span(ClientRect());
		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

		bool isLit = IsPointInRect(metrics.cursorPosition, absRect);

		auto& s = Scheme();

		RGBAb bkColour1 = !isLit ? s.bottomRight : s.hi_bottomRight;
		RGBAb bkColour2 = !isLit ? s.topLeft : s.hi_topLeft;

		Graphics::DrawRectangle(grc, absRect, bkColour1, bkColour2);

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

		ForEachItem(renderAll, topLeft);

		RGBAb edgeColour1 = isLit ? s.bottomRightEdge : s.hi_bottomRightEdge;
		RGBAb edgeColour2 = isLit ? s.topLeftEdge : s.hi_topLeftEdge;
		Graphics::DrawBorderAround(grc, absRect, { 1,1 }, edgeColour1, edgeColour2);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		Rococo::IEnumListPane* AddEnumList(IPublisher& publisher, BasePane& panel, int32 fontIndex, const fstring& populateId, const GuiRect& rect)
		{
			auto* enumList = new EnumListPane(publisher, fontIndex, populateId);
			panel.AddChild(enumList);
			enumList->SetRect(rect);
			return enumList;
		}
	}
}