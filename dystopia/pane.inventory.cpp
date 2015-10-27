#include "dystopia.h"
#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "rococo.fonts.h"

#include <string>

#include <vector>

#include "human.types.h"

using namespace Rococo;
using namespace Dystopia;

namespace
{
	struct RectEvent
	{
		GuiRect renderRect;
		bool isBoxRect;
		int inventoryIndex;
	};

	void ComputeBoxMatrixGeometry(int rows, int columns, Vec2i cellSpan, Vec2i topLeft, Vec2i cellBorders, IEventCallback<RectEvent>& cb)
	{
		int32 width = columns * (cellSpan.x + cellBorders.x) + cellBorders.x;
		int32 height = rows * (cellSpan.y + cellBorders.y) + cellBorders.y;
		GuiRect inventoryRect(topLeft.x, topLeft.y, topLeft.x + width, topLeft.y + height);

		cb.OnEvent(RectEvent{ inventoryRect, false, -1 });

		int inventoryIndex = 0;

		for (int j = 0; j < rows; j++)
		{
			int y = j * (cellSpan.y + cellBorders.y) + cellBorders.y + topLeft.y;
			for (int i = 0; i < columns; ++i)
			{
				int x = i * (cellSpan.x + cellBorders.x) + cellBorders.x + topLeft.x;
				GuiRect box(x, y, x + cellSpan.x, y + cellSpan.y);

				cb.OnEvent(RectEvent{ box, true, inventoryIndex++ });
			}
		}
	}

	GuiRect RenderBoxMatrix(IGuiRenderContext& gc, int rows, int columns, Vec2i cellSpan, Vec2i topLeft, Vec2i cellBorders, IInventory& inv)
	{
		struct : IEventCallback<RectEvent>
		{
			Vec2i cursorPos;
			IGuiRenderContext* gc;
			IInventory* inv;
			GuiRect mainRect;

			virtual void OnEvent(RectEvent& re)
			{
				if (IsPointInRect(cursorPos, re.renderRect))
				{	
					if (re.isBoxRect) Graphics::DrawRectangle(*gc, re.renderRect, RGBAb(32, 0, 0), RGBAb(0, 0, 32));
					Graphics::DrawBorderAround(*gc, re.renderRect, Vec2i{ 1,1 }, RGBAb(224, 224, 224), RGBAb(255, 255, 255));
				}
				else
				{
					if (re.isBoxRect) Graphics::DrawRectangle(*gc, re.renderRect, RGBAb(0, 0, 0), RGBAb(0, 0, 0));
					Graphics::DrawBorderAround(*gc, re.renderRect, Vec2i{ 1,1 }, RGBAb(160, 160, 160), RGBAb(224, 224, 224));
				}

				auto* item = inv->GetItem(re.inventoryIndex);
				if (item != nullptr)
				{
					Vec2i p = Centre(re.renderRect);
					GuiRect highlight(p.x - 4, p.y - 4, p.x + 4, p.y + 4);
					Graphics::DrawRectangle(*gc, highlight, RGBAb(255, 0, 0), RGBAb(255, 255, 0));
				}

				if (re.isBoxRect)
				{
					mainRect = re.renderRect;
				}
			}
		} renderBox;
		renderBox.gc = &gc;
		renderBox.inv = &inv;

		GuiMetrics gm;
		gc.Renderer().GetGuiMetrics(gm);

		renderBox.cursorPos = gm.cursorPosition;

		ComputeBoxMatrixGeometry(rows, columns, cellSpan, topLeft, cellBorders, renderBox);

		return renderBox.mainRect;
	}

	enum ID_CONTROL_ICON
	{
		CONTROL_ICON_EXAMINE
	};

	struct ControlIcon
	{
		ID_CONTROL_ICON id;
		std::wstring resourceName1;
		std::wstring resourceName2;
		std::wstring verb;
	};

	typedef std::vector<ControlIcon> Icons;

	const ControlIcon* RenderControlIcons(IGuiRenderContext& gc, int rows, int columns, Vec2i cellSpan, Vec2i topLeft, Vec2i cellBorders, const Icons& icons)
	{
		struct : IEventCallback<RectEvent>
		{
			Vec2i cursorPos;
			IGuiRenderContext* gc;
			const Icons* icons;
			const ControlIcon* icon;

			virtual void OnEvent(RectEvent& re)
			{
				if (IsPointInRect(cursorPos, re.renderRect))
				{
					if (re.inventoryIndex >= 0 && re.inventoryIndex < icons->size())
					{
						icon = &(*icons)[re.inventoryIndex];
					}
					if (re.isBoxRect) Graphics::DrawRectangle(*gc, re.renderRect, RGBAb(32, 0, 0), RGBAb(0, 0, 32));
					Graphics::DrawBorderAround(*gc, re.renderRect, Vec2i{ 1,1 }, RGBAb(224, 224, 224), RGBAb(255, 255, 255));
				}
				else
				{
					if (re.isBoxRect) Graphics::DrawRectangle(*gc, re.renderRect, RGBAb(0, 0, 0), RGBAb(0, 0, 0));
					Graphics::DrawBorderAround(*gc, re.renderRect, Vec2i{ 1,1 }, RGBAb(160, 160, 160), RGBAb(224, 224, 224));
				}

				if (re.inventoryIndex >= 0 && re.inventoryIndex < icons->size())
				{
					Vec2i p = Centre(re.renderRect);
					GuiRect highlight(p.x - 4, p.y - 4, p.x + 4, p.y + 4);
					Graphics::DrawRectangle(*gc, highlight, RGBAb(255, 0, 0), RGBAb(255, 255, 0));
				}
			}
		} renderBox;
		renderBox.gc = &gc;
		renderBox.icons = &icons;
		renderBox.icon = nullptr;

		GuiMetrics gm;
		gc.Renderer().GetGuiMetrics(gm);

		renderBox.cursorPos = gm.cursorPosition;

		ComputeBoxMatrixGeometry(rows, columns, cellSpan, topLeft, cellBorders, renderBox);

		return renderBox.icon;
	}

	class InventoryPane : public IUIPaneSupervisor, public IEventCallback<ActionMap>
	{
		Environment& e;
		ILevel& level;

		Icons icons;
		const ControlIcon* lastSelectedIcon;
	public:
		InventoryPane(Environment& _e, ILevel& _level) :
			e(_e), level(_level)
		{
			icons.push_back(
				{
					CONTROL_ICON_EXAMINE,
					L"!icons/examine.up.tiff",
					L"!icons/examine.down.tiff",
					L"Examine"
				}
			);
		}

		virtual void Free()
		{
			delete this;
		}

		virtual PaneModality OnTimestep(const TimestepEvent& clock)
		{
			return PaneModality_Modeless;
		}

		virtual void OnEvent(ActionMap& map)
		{
			switch (map.type)
			{
			case ActionMapTypeInventory:
				if (map.isActive) e.uiStack.PopTop();
				break;
			}
		}

		virtual PaneModality OnKeyboardEvent(const KeyboardEvent& ke)
		{
			e.controls.MapKeyboardEvent(ke, *this);
			return PaneModality_Modal;
		}

		virtual PaneModality OnMouseEvent(const MouseEvent& me)
		{
			e.controls.MapMouseEvent(me, *this);
			return PaneModality_Modal;
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
		}

		virtual void RenderGui(IGuiRenderContext& gc)
		{
			auto* inv = level.GetInventory(level.GetPlayerId());
			if (inv)
			{
				auto span = inv->Span();

				Vec2i topLeft{ 164, 10 };

				lastSelectedIcon = RenderControlIcons(gc, span.rows, 2, Vec2i{ 64, 56 }, Vec2i { 10, 10 }, Vec2i{ 5, 4 }, icons);
				auto rect = RenderBoxMatrix(gc, span.rows, span.columns, Vec2i{ 64, 56 }, topLeft, Vec2i{ 5, 4 }, *inv);
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
	IUIPaneSupervisor* CreateInventoryPane(Environment& e, ILevel& level)
	{
		return new InventoryPane(e, level);
	}
}