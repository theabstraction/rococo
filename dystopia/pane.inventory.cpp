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
	struct ItemRectEvent
	{
		GuiRect renderRect;
		int inventoryIndex;
	};

	struct MainRectEvent
	{
		GuiRect renderRect;
	};

	void ComputeBoxMatrixGeometry(int rows, int columns, 
		Vec2i cellSpan, Vec2i topLeft, Vec2i cellBorders, IEventCallback<ItemRectEvent>& cbItem, IEventCallback<MainRectEvent>& cbMain)
	{
		int32 width = columns * (cellSpan.x + cellBorders.x) + cellBorders.x;
		int32 height = rows * (cellSpan.y + cellBorders.y) + cellBorders.y;
		GuiRect inventoryRect(topLeft.x, topLeft.y, topLeft.x + width, topLeft.y + height);

		cbMain.OnEvent(MainRectEvent{ inventoryRect });

		int inventoryIndex = 0;

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

	auto RenderInventory(IGuiRenderContext& gc, int rows, int columns, Vec2i cellSpan, Vec2i topLeft, Vec2i cellBorders, IInventory& inv, IBitmapCache& bitmaps) -> int // inventory index
	{
		struct : IEventCallback<MainRectEvent>, IEventCallback<ItemRectEvent>
		{
			Vec2i cursorPos;
			IGuiRenderContext* gc;
			IInventory* inv;
			GuiRect mainRect;
			int selectedIndex;
			IBitmapCache* bitmaps;

			virtual void OnEvent(ItemRectEvent& re)
			{
				if (IsPointInRect(cursorPos, re.renderRect))
				{
					selectedIndex = re.inventoryIndex;
					Graphics::DrawRectangle(*gc, re.renderRect, RGBAb(32, 0, 0), RGBAb(0, 0, 32));
					Graphics::DrawBorderAround(*gc, re.renderRect, Vec2i{ 1,1 }, RGBAb(224, 224, 224), RGBAb(255, 255, 255));
				}
				else
				{
					Graphics::DrawRectangle(*gc, re.renderRect, RGBAb(0, 0, 0), RGBAb(0, 0, 0));
					Graphics::DrawBorderAround(*gc, re.renderRect, Vec2i{ 1,1 }, RGBAb(160, 160, 160), RGBAb(224, 224, 224));
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
					Graphics::DrawBorderAround(*gc, mr.renderRect, Vec2i{ 1,1 }, RGBAb(224, 224, 224), RGBAb(255, 255, 255));
				}
				else
				{
					Graphics::DrawBorderAround(*gc, mr.renderRect, Vec2i{ 1,1 }, RGBAb(160, 160, 160), RGBAb(224, 224, 224));
				}

				mainRect = mr.renderRect;
			}
		} renderBox;
		renderBox.gc = &gc;
		renderBox.inv = &inv;
		renderBox.selectedIndex = -1;
		renderBox.bitmaps = &bitmaps;

		GuiMetrics gm;
		gc.Renderer().GetGuiMetrics(gm);

		renderBox.cursorPos = gm.cursorPosition;

		ComputeBoxMatrixGeometry(rows, columns, cellSpan, topLeft, cellBorders, renderBox, renderBox);

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

	const ControlIcon* RenderControlIcons(IGuiRenderContext& gc, int rows, int columns, Vec2i cellSpan, Vec2i topLeft, Vec2i cellBorders, const Icons& icons, IBitmapCache& cache)
	{
		struct : IEventCallback<MainRectEvent>, IEventCallback<ItemRectEvent>
		{
			Vec2i cursorPos;
			IGuiRenderContext* gc;
			const Icons* icons;
			const ControlIcon* icon;
			IBitmapCache* cache;

			virtual void OnEvent(MainRectEvent& mr)
			{
				if (IsPointInRect(cursorPos, mr.renderRect))
				{
					Graphics::DrawBorderAround(*gc, mr.renderRect, Vec2i{ 1,1 }, RGBAb(224, 224, 224), RGBAb(255, 255, 255));
				}
				else
				{
					Graphics::DrawBorderAround(*gc, mr.renderRect, Vec2i{ 1,1 }, RGBAb(160, 160, 160), RGBAb(224, 224, 224));
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
						Graphics::DrawBorderAround(*gc, re.renderRect, Vec2i{ 1,1 }, RGBAb(224, 224, 224), RGBAb(255, 255, 255));
					}
					else
					{	
						Graphics::DrawRectangle(*gc, re.renderRect, RGBAb(0, 0, 0), RGBAb(0, 0, 0));
						cache->DrawBitmap(*gc, re.renderRect, currentIcon->bitmapIdUp);
						Graphics::DrawBorderAround(*gc, re.renderRect, Vec2i{ 1,1 }, RGBAb(160, 160, 160), RGBAb(224, 224, 224));
					}
				}
				else
				{
					Graphics::DrawRectangle(*gc, re.renderRect, RGBAb(0, 0, 0), RGBAb(0, 0, 0));
					Graphics::DrawBorderAround(*gc, re.renderRect, Vec2i{ 1,1 }, RGBAb(160, 160, 160), RGBAb(224, 224, 224));
				}
			}
		} renderBox;
		renderBox.gc = &gc;
		renderBox.icons = &icons;
		renderBox.icon = nullptr;
		renderBox.cache = &cache;

		GuiMetrics gm;
		gc.Renderer().GetGuiMetrics(gm);

		renderBox.cursorPos = gm.cursorPosition;

		ComputeBoxMatrixGeometry(rows, columns, cellSpan, topLeft, cellBorders, renderBox, renderBox);

		return renderBox.icon;
	}

	class InventoryPane : public IUIPaneSupervisor, public IEventCallback<ActionMap>
	{
		Environment& e;

		Icons icons;
		const ControlIcon* lastSelectedIcon;
		int lastInventoryIndex;

		CONTROL_ICON activeIcon;
	public:
		InventoryPane(Environment& _e) :
			e(_e), activeIcon(CONTROL_ICON_NONE)
		{
			icons.push_back(
				{
					CONTROL_ICON_EXAMINE,
					L"!icons/examine.up.tif",
					L"!icons/examine.down.tif",
					L"Examine",
					0,
					0,
					Vec2i { -32, -28 }
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
							e.bitmapCache.SetCursorBitmap(item->BitmapId(), Vec2i{ -32, -28 });
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
					else
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

				lastSelectedIcon = RenderControlIcons(gc, span.rows, 2, Vec2i{ 64, 56 }, Vec2i { 10, 10 }, Vec2i{ 5, 4 }, icons, e.bitmapCache);
				lastInventoryIndex = RenderInventory(gc, span.rows, span.columns, Vec2i{ 64, 56 }, topLeft, Vec2i{ 5, 4 }, *inv, e.bitmapCache);
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