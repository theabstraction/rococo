#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRCell
	{
		IGRWidgetDivision* div = nullptr;
	};

	struct GRColumn
	{
		HString name;
		int minWidth;
		int defaultWidth;
		int maxWidth;
		int width;
	};

	struct GRTableRow
	{
		std::vector<GRCell> cellsInThisRow;
		int32 rowHeight;
	};

	struct GRTable : IGRWidgetTable, IGRWidget
	{
		IGRPanel& panel;

		std::vector<GRColumn> columnHeaders;
		std::vector<GRTableRow> rows;

		GRTable(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		int32 AddColumn(const GRColumnSpec& spec) override
		{
			columnHeaders.push_back(GRColumn{ spec.name, spec.minWidth, spec.defaultWidth, spec.maxWidth, spec.defaultWidth });

			for (auto& row : rows)
			{
				row.cellsInThisRow.push_back(GRCell{ &CreateDivision(*this) });
			}

			panel.InvalidateLayout(true);

			return (int32) columnHeaders.size() - 1;
		}

		void ExpandToFit()
		{
			int height = 0;

			for (auto& row : rows)
			{
				height += row.rowHeight;
			}

			panel.Resize({ panel.Span().x, height });
		}

		// Adds a new row and returns the row index of the new row
		int32 AddRow(const GRRowSpec& spec) override
		{
			rows.push_back(GRTableRow{});
			rows.back().cellsInThisRow.resize(columnHeaders.size());
			rows.back().rowHeight = spec.rowHeight;

			for (auto& cell : rows.back().cellsInThisRow)
			{
				cell.div = &CreateDivision(*this);
			}

			panel.InvalidateLayout(true);

			ExpandToFit();

			return (int32) rows.size() - 1;
		}

		// Returns a reference to the cell at the given location. If the location indices are out of bounds, the method returns nullptr
		IGRWidgetDivision* GetCell(int32 column, int32 row) override
		{
			if (column < 0 || column >= (int32)columnHeaders.size())
			{
				return nullptr;
			}

			if (row < 0 || row >= (int32)rows.size())
			{
				return nullptr;
			}

			auto& cell = rows[row].cellsInThisRow[column];
			return cell.div;
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			int x = 0;

			for (size_t colIndex = 0; colIndex < columnHeaders.size(); ++colIndex)
			{
				const GRColumn& columnSpec = columnHeaders[colIndex];

				int columnWidth = colIndex < columnHeaders.size() - 1 ? columnSpec.width : max(columnSpec.width, Width(panelDimensions) - x);

				int y = 0;

				for (auto& row : rows)
				{
					auto& cell = row.cellsInThisRow[colIndex];

					cell.div->Panel().SetParentOffset({ x, y }).Resize({ columnWidth, row.rowHeight });

					y += row.rowHeight;
				}

				x += columnWidth;
			}
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			return EventRouting::NextHandler;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return { 0,0 };
		}

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetTable>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
		}
	};

	struct GRTableFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRTable(panel);
		}
	} s_TableFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetTable::InterfaceId()
	{
		return "IGRWidgetTable";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetTable& CreateTable(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto* table = Cast<IGRWidgetTable>(gr.AddWidget(parent.Panel(), GRANON::s_TableFactory));
		return *table;
	}
}