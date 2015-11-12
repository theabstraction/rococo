#include "dystopia.h"
#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "rococo.fonts.h"
#include "rococo.strings.h"
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
	const Vec2i cellBorders{ 5, 4 };
	enum { FOREIGN_INV_FLAG = 0x40000000, CONTAINER_INV_FLAG = 0x20000000 };
	struct ItemRectEvent
	{
		GuiRect renderRect;
		int inventoryIndex;
	};

	struct MainRectEvent
	{
		GuiRect renderRect;
	};

	void DrawItemBitmap(IItem* item, GuiRect& rect, IGuiRenderContext& rc, IBitmapCache& b)
	{
		if (!item) return;

		b.DrawBitmap(rc, rect, item->Data().bitmapId);

		auto* ammo = item->GetAmmo();
		if (ammo)
		{
			wchar_t scount[8];
			SafeFormat(scount, _TRUNCATE, L"%u", ammo->count);
			
			Graphics::StackSpaceGraphics ss;
			{
				auto& shadowjob = Graphics::CreateHorizontalCentredText(ss, 7, scount, RGBAb(0, 0, 0));
				rc.RenderText(TopLeft(rect) - Vec2i{ 1,1 }, shadowjob);
			}
			
			{
				auto& job = Graphics::CreateHorizontalCentredText(ss, 7, scount, RGBAb(255, 255, 255));
				rc.RenderText(TopLeft(rect), job);
			}
		}
	}

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

	auto RenderContainer(uint32 startIndex, IGuiRenderContext& gc, int rows, int columns, Vec2i cellSpan, Vec2i topLeft, Vec2i cellBorders, IInventory& inv, IBitmapCache& bitmaps, bool& isGuiUnderSlot) -> int // inventory index
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
					DrawItemBitmap(item, re.renderRect, *gc, *bitmaps);
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
		CONTROL_ICON_EXAMINE,
		CONTROL_ICON_OPEN
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
		PAPER_DOLL_SLOT itemSlot = cursorItem ? cursorItem->Data().slot : PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO;
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
						DrawItemBitmap(item, targetRect, gc, bitmaps);
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

	class InventoryPane : public IUIPaneSupervisor, public IEventCallback<ActionMap>, public Post::IRecipient
	{
		Environment& e;

		Icons icons;
		const ControlIcon* lastSelectedIcon;
		int lastInventoryIndex;

		CONTROL_ICON activeIcon;

		InventoryList paperDoll;

		bool isCursorUnderGui;

		ID_ENTITY foreignContainerId;
		int32 containerSlotIndex;

		bool isBackpackVisible;
	public:
		InventoryPane(Environment& _e) :
			e(_e), activeIcon(CONTROL_ICON_NONE), isCursorUnderGui(false) , containerSlotIndex(-1), isBackpackVisible(false)
		{
			icons.push_back(
			{
				CONTROL_ICON_EXAMINE,
				L"!icons/examine.up.tif",
				L"!icons/examine.up.tif",
				L"Examine",
				ID_BITMAP::Invalid(),
				ID_BITMAP::Invalid(),
				slotHotSpotOffset
			}
			);

			icons.push_back(
			{
				CONTROL_ICON_OPEN,
				L"!icons/open.tif",
				L"!icons/open.tif",
				L"Open",
				ID_BITMAP::Invalid(),
				ID_BITMAP::Invalid(),
				slotHotSpotOffset
			}
			);

			for (auto& i : icons)
			{
				i.bitmapIdDown = e.bitmapCache.Cache(i.resourceName1.c_str());
				i.bitmapIdUp = e.bitmapCache.Cache(i.resourceName2.c_str());
			}

			e.postbox.Subscribe<VerbOpenInventory>(this);
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void OnPost(const Mail& mail)
		{
			auto* openInventory = Post::InterpretAs<VerbOpenInventory>(mail);
			if (openInventory)
			{
				if (e.uiStack.Top().id != ID_PANE_INVENTORY_SELF)
				{
					e.uiStack.PushTop(ID_PANE_INVENTORY_SELF);
				}

				foreignContainerId = openInventory->containerId;
			}
		}

		virtual Relay OnTimestep(const TimestepEvent& clock)
		{
			return Relay_Next;
		}

		void OnIconSelected(const ControlIcon& icon)
		{
			activeIcon = icon.id;

			auto* inv = e.level.GetInventory(e.level.GetPlayerId());
			if (!inv) return;

			if (activeIcon == CONTROL_ICON_EXAMINE)
			{
				auto* item = inv->GetItem(inv->GetCursorIndex());
				if (item)
				{
					VerbExamine examine{ e.level.GetPlayerId(), inv->GetCursorIndex() };
					e.postbox.PostForLater(examine, false);
				}
			}
			else if (activeIcon == CONTROL_ICON_OPEN)
			{
				isBackpackVisible = !isBackpackVisible;
			}
		}

		virtual void OnEvent(ActionMap& map)
		{
			switch (map.type)
			{
			case ActionMapTypeInventory:
				if (map.isActive)
				{
					e.uiStack.PopTop();
					e.bitmapCache.SetCursorBitmap(ID_BITMAP::Invalid(), Vec2i{ 0,0 });
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
						OnIconSelected(*lastSelectedIcon);
					}
					else if (lastInventoryIndex >= 0)
					{
						if ((lastInventoryIndex & FOREIGN_INV_FLAG) == 0)
						{
							if ((lastInventoryIndex & CONTAINER_INV_FLAG) != 0)
							{
								auto* rightItem = inv->GetItem(PAPER_DOLL_SLOT_RIGHT_HAND);

								auto* wep = rightItem->GetRangedWeaponData();
								if (wep != nullptr)
								{
									auto* magazine = inv->GetItem(inv->GetCursorIndex());
									if (magazine)
									{
										auto* ammo = magazine->GetAmmo();
										if (ammo == nullptr || ammo->ammoType != wep->ammunitionIndex)
										{
											return; // wrong ammo
										}
									}
								}

								auto* container = rightItem ? rightItem->GetContents() : nullptr;
								if (!container) return;

								auto* item = container->GetItem(lastInventoryIndex & ~CONTAINER_INV_FLAG);
								if (item)
								{
									e.bitmapCache.SetCursorBitmap(item->Data().bitmapId, slotHotSpotOffset);
								}
								else
								{
									e.bitmapCache.SetCursorBitmap(ID_BITMAP::Invalid(), Vec2i{ 0,0 });		
								}

								auto* oldCursorItem = inv->Swap(inv->GetCursorIndex(), item);
								container->Swap(lastInventoryIndex & ~CONTAINER_INV_FLAG, oldCursorItem);
							}
							else
							{
								auto* item = inv->GetItem(lastInventoryIndex);
								if (item)
								{
									e.bitmapCache.SetCursorBitmap(item->Data().bitmapId, slotHotSpotOffset);
								}
								else
								{
									e.bitmapCache.SetCursorBitmap(ID_BITMAP::Invalid(), Vec2i{ 0,0 });
									
								}
								
								auto* oldCursorItem = inv->Swap(inv->GetCursorIndex(), item);
								inv->Swap(lastInventoryIndex, oldCursorItem);
							}
						}
						else
						{
							auto* foreignInv = e.level.GetInventory(foreignContainerId);
							if (foreignInv)
							{
								auto* item = foreignInv->GetItem(lastInventoryIndex & ~FOREIGN_INV_FLAG);
								if (item)
								{
									e.bitmapCache.SetCursorBitmap(item->Data().bitmapId, slotHotSpotOffset);
								}
								else
								{
									e.bitmapCache.SetCursorBitmap(ID_BITMAP::Invalid(), Vec2i{ 0,0 });
								}

								auto* oldCursorItem = inv->Swap(inv->GetCursorIndex(), item);
								foreignInv->Swap(lastInventoryIndex & ~FOREIGN_INV_FLAG, oldCursorItem);

								VerbInventoryChanged changed;
								changed.containerId = foreignContainerId;
								e.postbox.PostForLater(changed, false);
							}
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
							e.bitmapCache.SetCursorBitmap(ID_BITMAP::Invalid(), { 0,0 });
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

		virtual void OnPop()
		{
			foreignContainerId = ID_ENTITY::Invalid();
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
					e.bitmapCache.SetCursorBitmap(item->Data().bitmapId, Vec2i{ -32, -28 });
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
				isCursorUnderGui = false;
				lastInventoryIndex = -1;

				int nIconCols = 1;
				Vec2i topLeft {2, 2}; 
				lastSelectedIcon = RenderControlIcons(gc, 4, nIconCols, slotDimensions, topLeft, cellBorders, icons, e.bitmapCache, isCursorUnderGui);

				int32 iconWidth = nIconCols * (slotDimensions.y + cellBorders.y);
				
				Vec2i bptopLeft{ iconWidth + topLeft.x + 24, 2 };
				Vec2i ammoTopLeft = isBackpackVisible ? Vec2i{ 328, 252 } : (bptopLeft + Vec2i{ 320, 0 });

				if (isBackpackVisible)
				{
					auto span = inv->Span();
					
					uint32 startIndex = inv->HasPaperDoll() ? PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO : 0;
					lastInventoryIndex = RenderContainer(startIndex, gc, span.rows, span.columns, slotDimensions, bptopLeft, cellBorders, *inv, e.bitmapCache, isCursorUnderGui);
				}

				if (inv->HasPaperDoll())
				{
					Vec2i topLeft = isBackpackVisible ? Vec2i { 2, 252 } : bptopLeft;
					int32 lastDollIndex = RenderInventoryList(gc, topLeft, *inv, paperDoll, e.bitmapCache, isCursorUnderGui);
					if (lastInventoryIndex < 0)
					{
						lastInventoryIndex = lastDollIndex;
					}

					auto* equippedItem = inv->GetItem(PAPER_DOLL_SLOT_RIGHT_HAND);
					if (equippedItem)
					{
						auto* contents = equippedItem->GetContents();
						if (contents)
						{
							auto span = contents->Span();

							int32 containerHeight = span.rows * (slotDimensions.y + cellBorders.y) + 70;
							int32 containerWidth = span.columns * (slotDimensions.x + cellBorders.x) + 14;

							Graphics::DrawRectangle(gc, GuiRect(0, 0, containerWidth, containerHeight) + ammoTopLeft, opaqueBlack, opaqueBlack);
							Graphics::DrawBorderAround(gc, GuiRect(0, 0, containerWidth, containerHeight) + ammoTopLeft, borderSpan, border1, border2);

							GuiRect bitmapRect = GuiRect(0, 0, slotDimensions.x + 2, slotDimensions.y) + ammoTopLeft + Vec2i{ 2,2 };
							e.bitmapCache.DrawBitmap(gc, bitmapRect, equippedItem->Data().bitmapId);
							uint32 startIndex = contents->HasPaperDoll() ? PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO : 0;

							Vec2i containerTopLeft{ bitmapRect.left + 2, bitmapRect.bottom + 2 };
							int32 contentsIndex = RenderContainer(startIndex, gc, span.rows, span.columns, slotDimensions, containerTopLeft, cellBorders, *contents, e.bitmapCache, isCursorUnderGui);
							if (contentsIndex >= 0)
							{
								lastInventoryIndex = contentsIndex | CONTAINER_INV_FLAG;
							}
						}
					}
				}

				auto* foreignInv = e.level.GetInventory(foreignContainerId);
				if (foreignInv)
				{
					uint32 foreignInvStart = foreignInv->HasPaperDoll() ? PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO : 0;
					auto foreignSpan = foreignInv->Span();

					GuiMetrics metrics;
					e.renderer.GetGuiMetrics(metrics);

					auto span = foreignInv->Span();
					int32 containerWidth = span.columns * (slotDimensions.x + cellBorders.x) + 14;

					Vec2i forTopLeft { metrics.screenSpan.x - containerWidth, 252 };

					int32 foreignIndex = RenderContainer(foreignInvStart, gc, foreignSpan.rows, foreignSpan.columns, slotDimensions, forTopLeft, cellBorders, *foreignInv, e.bitmapCache, isCursorUnderGui);
					if (foreignIndex >= 0) lastInventoryIndex = foreignIndex | FOREIGN_INV_FLAG;
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