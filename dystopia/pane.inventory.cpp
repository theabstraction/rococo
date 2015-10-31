#include "dystopia.h"
#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "rococo.fonts.h"
#include "dystopia.post.h"

#include <string>

#include <vector>

#include "human.types.h"

#include "dystopia.ui.h"

using namespace Rococo;
using namespace Dystopia;

namespace
{
	const RGBAb dollBlack(0, 0, 0, 64);
	const RGBAb opaqueBlack(0, 0, 0, 255);
	const RGBAb border1(160, 160, 160);
	const RGBAb border2(192, 192, 192);
	const RGBAb hiBorder1(224, 224, 224);
	const RGBAb hiBorder2(255, 255, 255);
	const Vec2i borderSpan{ 1,1 };
	const Vec2i slotDimensions{ 64, 56 };
	const Vec2i slotHotSpotOffset{ -32, -28 };

	struct ItemRectEvent
	{
		GuiRect renderRect;
		int inventoryIndex;
	};

	struct MainRectEvent
	{
		GuiRect renderRect;
	};

	void ComputeBoxMatrixGeometry(uint32 startIndex, int rows, int columns, 
		Vec2i cellSpan, Vec2i topLeft, Vec2i cellBorders, IEventCallback<ItemRectEvent>& cbItem, IEventCallback<MainRectEvent>& cbMain)
	{
		int32 width = columns * (cellSpan.x + cellBorders.x) + cellBorders.x;
		int32 height = rows * (cellSpan.y + cellBorders.y) + cellBorders.y;
		GuiRect inventoryRect(topLeft.x, topLeft.y, topLeft.x + width, topLeft.y + height);

		cbMain.OnEvent(MainRectEvent{ inventoryRect });

		int inventoryIndex = startIndex;

		for (int j = 0; j < rows; j++)
		{
			int y = j * (cellSpan.y + cellBorders.y) + cellBorders.y + topLeft.y;
			for (int i = 0; i < columns; ++i)
			{
				int x = i * (cellSpan.x + cellBorders.x) + cellBorders.x + topLeft.x;
				GuiRect box(x, y, x + cellSpan.x, y + cellSpan.y);

				cbItem.OnEvent(ItemRectEvent{ box, inventoryIndex++ });
			}
		}
	}

	auto RenderBackpack(uint32 startIndex, IGuiRenderContext& gc, int rows, int columns, Vec2i cellSpan, Vec2i topLeft, Vec2i cellBorders, IInventory& inv, IBitmapCache& bitmaps, bool& isGuiUnderSlot) -> int // inventory index
	{
		struct : IEventCallback<MainRectEvent>, IEventCallback<ItemRectEvent>
		{
			Vec2i cursorPos;
			IGuiRenderContext* gc;
			IInventory* inv;
			GuiRect mainRect;
			int selectedIndex;
			IBitmapCache* bitmaps;
			bool isGuiUnderSlot;

			virtual void OnEvent(ItemRectEvent& re)
			{
				if (IsPointInRect(cursorPos, re.renderRect))
				{
					selectedIndex = re.inventoryIndex;
					Graphics::DrawRectangle(*gc, re.renderRect, RGBAb(32, 0, 0), RGBAb(0, 0, 32));
					Graphics::DrawBorderAround(*gc, re.renderRect, borderSpan, border2, hiBorder2);
				}
				else
				{
					Graphics::DrawRectangle(*gc, re.renderRect, opaqueBlack, opaqueBlack);
					Graphics::DrawBorderAround(*gc, re.renderRect, borderSpan, border1, hiBorder1);
				}

				auto* item = inv->GetItem(re.inventoryIndex);
				if (item != nullptr)
				{
					ID_BITMAP bitmapId = item->BitmapId();
					bitmaps->DrawBitmap(*gc, re.renderRect, bitmapId);
				}
			}

			virtual void OnEvent(MainRectEvent& mr)
			{
				if (IsPointInRect(cursorPos, mr.renderRect))
				{
					Graphics::DrawBorderAround(*gc, mr.renderRect, borderSpan, border2, hiBorder2);
					isGuiUnderSlot = true;
				}
				else
				{
					Graphics::DrawBorderAround(*gc, mr.renderRect, borderSpan,border1, hiBorder1);
				}

				mainRect = mr.renderRect;
			}
		} renderBox;
		renderBox.gc = &gc;
		renderBox.inv = &inv;
		renderBox.selectedIndex = -1;
		renderBox.bitmaps = &bitmaps;
		renderBox.isGuiUnderSlot = false;

		GuiMetrics gm;
		gc.Renderer().GetGuiMetrics(gm);

		renderBox.cursorPos = gm.cursorPosition;

		ComputeBoxMatrixGeometry(startIndex, rows, columns, cellSpan, topLeft, cellBorders, renderBox, renderBox);

		if (renderBox.isGuiUnderSlot)
		{
			isGuiUnderSlot = true;
		}

		return renderBox.selectedIndex;
	}

	enum CONTROL_ICON
	{
		CONTROL_ICON_NONE,
		CONTROL_ICON_EXAMINE
	};

	struct ControlIcon
	{
		CONTROL_ICON id;
		std::wstring resourceName1;
		std::wstring resourceName2;
		std::wstring verb;
		ID_BITMAP bitmapIdUp;
		ID_BITMAP bitmapIdDown;
		Vec2i hotspotOffset;
	};

	typedef std::vector<ControlIcon> Icons;

	const ControlIcon* RenderControlIcons(IGuiRenderContext& gc, int rows, int columns, Vec2i cellSpan, Vec2i topLeft, Vec2i cellBorders, const Icons& icons, IBitmapCache& cache, bool& isCursorUnderGui)
	{
		struct : IEventCallback<MainRectEvent>, IEventCallback<ItemRectEvent>
		{
			Vec2i cursorPos;
			IGuiRenderContext* gc;
			const Icons* icons;
			const ControlIcon* icon;
			IBitmapCache* cache;
			bool isCursorUnderGui;

			virtual void OnEvent(MainRectEvent& mr)
			{
				if (IsPointInRect(cursorPos, mr.renderRect))
				{
					Graphics::DrawBorderAround(*gc, mr.renderRect, borderSpan, border2, hiBorder2);
					isCursorUnderGui = true;
				}
				else
				{
					Graphics::DrawBorderAround(*gc, mr.renderRect, borderSpan, border1, hiBorder1);
				}
			}

			virtual void OnEvent(ItemRectEvent& re)
			{
				const ControlIcon* currentIcon = (re.inventoryIndex >= 0 && re.inventoryIndex < icons->size()) ? &(*icons)[re.inventoryIndex] : nullptr;

				if (currentIcon)
				{
					if (IsPointInRect(cursorPos, re.renderRect))
					{
						icon = currentIcon;	
						Graphics::DrawRectangle(*gc, re.renderRect, RGBAb(32, 0, 0), RGBAb(0, 0, 32));
						cache->DrawBitmap(*gc, re.renderRect, currentIcon->bitmapIdDown);
						Graphics::DrawBorderAround(*gc, re.renderRect, borderSpan, border2, hiBorder2);
					}
					else
					{	
						Graphics::DrawRectangle(*gc, re.renderRect, opaqueBlack, opaqueBlack);
						cache->DrawBitmap(*gc, re.renderRect, currentIcon->bitmapIdUp);
						Graphics::DrawBorderAround(*gc, re.renderRect, borderSpan, border1, hiBorder1);
					}
				}
				else
				{
					Graphics::DrawRectangle(*gc, re.renderRect, opaqueBlack, opaqueBlack);
					Graphics::DrawBorderAround(*gc, re.renderRect, borderSpan, border1, hiBorder1);
				}
			}
		} renderBox;
		renderBox.gc = &gc;
		renderBox.icons = &icons;
		renderBox.icon = nullptr;
		renderBox.cache = &cache;
		renderBox.isCursorUnderGui = false;

		GuiMetrics gm;
		gc.Renderer().GetGuiMetrics(gm);

		renderBox.cursorPos = gm.cursorPosition;

		ComputeBoxMatrixGeometry(0, rows, columns, cellSpan, topLeft, cellBorders, renderBox, renderBox);

		if (renderBox.isCursorUnderGui)
		{
			isCursorUnderGui = true;
		}

		return renderBox.icon;
	}

	enum InventoryBoxType
	{
		InventoryBoxType_None,
		InventoryBoxType_Border,
		InventoryBoxType_ColourTile,
		InventoryBoxType_Bitmap,
		InventoryBoxType_Slot
	};

	struct InventoryBox
	{
		PAPER_DOLL_SLOT slotIndex;
		InventoryBoxType type;
		GuiRect rect;
		ID_BITMAP bitmapId;
		ID_BITMAP highlightBitmapId;
		Vec2i borderSpan;
		RGBAb colour1;
		RGBAb colour2;
		RGBAb highlightColour1;
		RGBAb highlightColour2;
	};

	typedef std::vector<InventoryBox> InventoryList;

	bool DoesItemFitSlot(PAPER_DOLL_SLOT itemSlot, PAPER_DOLL_SLOT dollSlot)
	{
		if (itemSlot == PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO)
		{
			// PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO means no item
			return true;
		}
		else if (itemSlot == PAPER_DOLL_SLOT_EITHER_HAND)
		{
			return dollSlot == PAPER_DOLL_SLOT_LEFT_HAND || dollSlot == PAPER_DOLL_SLOT_RIGHT_HAND;
		}
		else
		{
			return dollSlot == itemSlot;
		}
	}

	auto RenderInventoryList(IGuiRenderContext& gc, Vec2i topLeft, IInventory& inv, const InventoryList& items, IBitmapCache& bitmaps, bool& isCursorUnderGui)->int32 // yields slot index under cursor or negative
	{
		GuiMetrics metrics;
		gc.Renderer().GetGuiMetrics(metrics);

		auto* cursorItem = inv.GetItem(inv.GetCursorIndex());
		PAPER_DOLL_SLOT itemSlot = cursorItem ? cursorItem->DollSlot() : PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO;
		int slotIndex = -1;

		for (auto& i : items)
		{
			GuiRect targetRect = i.rect + topLeft;
			bool isPointInRect = IsPointInRect(metrics.cursorPosition, targetRect);
			if (isPointInRect) isCursorUnderGui = true;
			bool isHighlighted = isPointInRect && DoesItemFitSlot(itemSlot, i.slotIndex);

			switch (i.type)
			{
			case InventoryBoxType_Border:
				Graphics::DrawBorderAround(gc, i.rect + topLeft, i.borderSpan,
					isHighlighted ? i.highlightColour1 : i.colour1,
					isHighlighted ? i.highlightColour2 : i.colour2);
				break;
			case InventoryBoxType_ColourTile:
				Graphics::DrawRectangle(gc, targetRect,
					isHighlighted ? i.highlightColour1 : i.colour1,
					isHighlighted ? i.highlightColour2 : i.colour2);
				break;
			case InventoryBoxType_Bitmap:
				bitmaps.DrawBitmap(gc, targetRect, isHighlighted ? i.highlightBitmapId : i.bitmapId);
				break;
			case InventoryBoxType_Slot:
				{
					auto* item = inv.GetItem(i.slotIndex);
					if (item != nullptr)
					{
						ID_BITMAP bitmapId = item->BitmapId();
						bitmaps.DrawBitmap(gc, targetRect, bitmapId);
					}

					if (isHighlighted)
					{
						slotIndex = i.slotIndex;
					}
				}
				break;
			}
		}

		return slotIndex;
	}

	void AddBitmap(InventoryList& items, IBitmapCache& bitmaps, const wchar_t* imageFile, const wchar_t* highlightImageFile, const GuiRect& rect, PAPER_DOLL_SLOT slot)
	{
		InventoryBox box;
		box.bitmapId = bitmaps.Cache(imageFile);
		box.highlightBitmapId = bitmaps.Cache(imageFile);
		box.type = InventoryBoxType_Bitmap;
		box.rect = rect;
		box.slotIndex = slot;

		items.push_back(box);
	}

	void AddBorder(InventoryList& items, RGBAb b1, RGBAb b2, RGBAb hiB1, RGBAb hiB2, const GuiRect& rect, Vec2i borderSpan, PAPER_DOLL_SLOT slot)
	{
		InventoryBox box;
		box.colour1 = b1;
		box.colour2 = b2;
		box.highlightColour1 = hiB1;
		box.highlightColour2 = hiB2;
		box.borderSpan = borderSpan;
		box.rect = rect;
		box.type = InventoryBoxType_Border;
		box.slotIndex = slot;

		items.push_back(box);
	}

	void AddColourTile(InventoryList& items, RGBAb b1, RGBAb b2, RGBAb hiB1, RGBAb hiB2, const GuiRect& rect, PAPER_DOLL_SLOT slot)
	{
		InventoryBox box;
		box.colour1 = b1;
		box.colour2 = b2;
		box.highlightColour1 = hiB1;
		box.highlightColour2 = hiB2;
		box.rect = rect;
		box.type = InventoryBoxType_ColourTile;
		box.slotIndex = slot;

		items.push_back(box);
	}

	GuiRect MakeSlotRect(Vec2i topLeft)
	{
		return GuiRect(topLeft.x, topLeft.y, topLeft.x + slotDimensions.x, topLeft.y + slotDimensions.y);
	}

	void AddInventorySlotDefault(InventoryList& doll, PAPER_DOLL_SLOT slotIndex, const GuiRect& rect, bool blackOut = true)
	{
		if (blackOut) AddColourTile(doll, dollBlack, dollBlack, RGBAb(64, 0, 0, 64), RGBAb(0, 0, 64, 64), rect, slotIndex);
		AddBorder(doll, border1, border2, hiBorder1, hiBorder2, rect, borderSpan, slotIndex);

		InventoryBox box;
		box.type = InventoryBoxType_Slot;
		box.slotIndex = slotIndex;
		box.rect = rect;

		doll.push_back(box);
	}

	void PopulatePaperDoll(InventoryList& doll, IBitmapCache& bitmaps)
	{
		AddBitmap(doll, bitmaps, L"!inventory/paperdoll.tif", L"!inventory/paperdoll.tif", GuiRect(0, 0, 320, 320), PAPER_DOLL_SLOT_EITHER_HAND );
		AddBorder(doll, border1, border2, hiBorder1, hiBorder2, GuiRect(0, 0, 320, 320), { 2, 2 }, PAPER_DOLL_SLOT_EITHER_HAND);
		AddInventorySlotDefault(doll, PAPER_DOLL_SLOT_LEFT_HAND, MakeSlotRect({4,4}));
		AddInventorySlotDefault(doll, PAPER_DOLL_SLOT_RIGHT_HAND, MakeSlotRect({ 252,4 }));
		AddInventorySlotDefault(doll, PAPER_DOLL_SLOT_FEET, MakeSlotRect({ 128,260 }));
		AddInventorySlotDefault(doll, PAPER_DOLL_SLOT_TROUSERS, MakeSlotRect({ 128,196 }));
		AddInventorySlotDefault(doll, PAPER_DOLL_SLOT_UNDERWEAR, MakeSlotRect({ 128,132 }));
		AddInventorySlotDefault(doll, PAPER_DOLL_SLOT_CHEST, MakeSlotRect({ 128,68 }));
		AddInventorySlotDefault(doll, PAPER_DOLL_SLOT_HELMET, MakeSlotRect({ 128,4 }));
	}

	class InventoryPane : public IUIPaneSupervisor, public IEventCallback<ActionMap>
	{
		Environment& e;

		Icons icons;
		const ControlIcon* lastSelectedIcon;
		int lastInventoryIndex;

		CONTROL_ICON activeIcon;

		InventoryList paperDoll;

		bool isCursorUnderGui;
	public:
		InventoryPane(Environment& _e) :
			e(_e), activeIcon(CONTROL_ICON_NONE), isCursorUnderGui(false)
		{
			icons.push_back(
				{
					CONTROL_ICON_EXAMINE,
					L"!icons/examine.up.tif",
					L"!icons/examine.down.tif",
					L"Examine",
					0,
					0,
					slotHotSpotOffset
				}
			);

			for (auto& i : icons)
			{
				i.bitmapIdDown = e.bitmapCache.Cache(i.resourceName1.c_str());
				i.bitmapIdUp = e.bitmapCache.Cache(i.resourceName2.c_str());
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual Relay OnTimestep(const TimestepEvent& clock)
		{
			return Relay_Next;
		}

		void OnIconSelected(const ControlIcon& icon)
		{
			e.bitmapCache.SetCursorBitmap(icon.bitmapIdUp, icon.hotspotOffset);
		}

		virtual void OnEvent(ActionMap& map)
		{
			switch (map.type)
			{
			case ActionMapTypeInventory:	
				if (map.isActive)
				{
					e.uiStack.PopTop();
					e.bitmapCache.SetCursorBitmap(0, Vec2i{ 0,0 });
					activeIcon = CONTROL_ICON_NONE;
				}
				break;
			case ActionMapTypeSelect:
				if (map.isActive)
				{
					auto* inv = e.level.GetInventory(e.level.GetPlayerId());
					if (!inv) break;

					if (lastSelectedIcon != nullptr)
					{
						activeIcon = lastSelectedIcon->id;

						if (activeIcon == CONTROL_ICON_EXAMINE)
						{
							auto* item = inv->GetItem(inv->GetCursorIndex());
							if (item)
							{
								VerbExamine examine{ e.level.GetPlayerId(), inv->GetCursorIndex() };
								e.postbox.PostForLater(examine, false);
							}
						}
					}
					else if (lastInventoryIndex >= 0)
					{
						auto* item = inv->GetItem(lastInventoryIndex);
						if (item)
						{
							e.bitmapCache.SetCursorBitmap(item->BitmapId(), slotHotSpotOffset);
							auto* oldCursorItem = inv->Swap(inv->GetCursorIndex(), item);
							inv->Swap(lastInventoryIndex, oldCursorItem);
						}
						else
						{
							e.bitmapCache.SetCursorBitmap(0, Vec2i{ 0,0 });
							auto* oldCursorItem = inv->Swap(inv->GetCursorIndex(), nullptr);
							inv->Swap(lastInventoryIndex, oldCursorItem);
						}
					}
					else if (!isCursorUnderGui)
					{
						auto* item = inv->GetItem(inv->GetCursorIndex());
						if (item)
						{
							GuiMetrics metrics;
							e.renderer.GetGuiMetrics(metrics);
							VerbDropAtCursor drop{ e.level.GetPlayerId(), inv->GetCursorIndex(), metrics.cursorPosition };
							e.postbox.PostForLater(drop, false);
							e.bitmapCache.SetCursorBitmap(0, { 0,0 });
						}
					}
				}
				break;
			}
		}

		virtual Relay OnKeyboardEvent(const KeyboardEvent& ke)
		{
			e.controls.MapKeyboardEvent(ke, *this);
			return Relay_None;
		}

		virtual Relay OnMouseEvent(const MouseEvent& me)
		{
			e.controls.MapMouseEvent(me, *this);
			return Relay_None;
		}

		virtual void OnTop()
		{
			if (paperDoll.empty())
			{
				PopulatePaperDoll(paperDoll, e.bitmapCache);
			}

			auto* inv = e.level.GetInventory(e.level.GetPlayerId());
			if (inv)
			{
				auto* item = inv->GetItem(inv->GetCursorIndex());
				if (item)
				{
					e.bitmapCache.SetCursorBitmap(item->BitmapId(), Vec2i{ -32, -28 });
				}
			}
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
		}

		virtual void RenderGui(IGuiRenderContext& gc)
		{
			auto* inv = e.level.GetInventory(e.level.GetPlayerId());
			if (inv)
			{
				auto span = inv->Span();

				Vec2i topLeft{ 164, 10 };

				isCursorUnderGui = false;

				lastSelectedIcon = RenderControlIcons(gc, span.rows, 2, slotDimensions, Vec2i { 10, 10 }, Vec2i{ 5, 4 }, icons, e.bitmapCache, isCursorUnderGui);

				uint32 startIndex = inv->HasPaperDoll() ? PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO : 0;
				lastInventoryIndex = RenderBackpack(startIndex, gc, span.rows, span.columns, slotDimensions, topLeft, Vec2i{ 5, 4 }, *inv, e.bitmapCache, isCursorUnderGui);

				if (inv->HasPaperDoll())
				{
					int32 lastDollIndex = RenderInventoryList(gc, { 740, 10 }, *inv, paperDoll, e.bitmapCache, isCursorUnderGui);
					if (lastInventoryIndex < 0)
					{
						lastInventoryIndex = lastDollIndex;
					}
				}
			}
			else
			{
				e.uiStack.PopTop(); // player vanished or something
			}
		}
	};
}

namespace Dystopia
{
	IUIPaneSupervisor* CreateInventoryPane(Environment& e)
	{
		return new InventoryPane(e);
	}
}