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

	struct GRTable : IGRWidgetTable, IGRWidgetSupervisor, IGRNavigator
	{
		IGRPanel& panel;

		std::vector<GRColumn> columnHeaders;
		std::vector<GRTableRow> rows;

		GRTable(IGRPanel& owningPanel) : panel(owningPanel)
		{
			panel.PreventInvalidationFromChildren();
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

		void SetColumnWidth(int columnIndex, int pixelWidth) override
		{
			if (columnIndex < 0 || columnIndex > (int) columnHeaders.size())
			{
				RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "column index %d out of bounds. Array size is %llu", columnIndex, columnHeaders.size());
			}

			int oldWidth = columnHeaders[columnIndex].width;
			if (oldWidth != pixelWidth)
			{
				columnHeaders[columnIndex].width = pixelWidth;
				panel.InvalidateLayout(true);
			}

			for (auto& row : rows)
			{
				auto& cell = row.cellsInThisRow[columnIndex];
				cell.div->Panel().SetConstantWidth(pixelWidth);
			}
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

			auto& row = CreateDivision(*this);
			row.Panel().SetConstantHeight(spec.rowHeight);
			row.Panel().SetExpandToParentHorizontally();
			row.Panel().SetLayoutDirection(ELayoutDirection::LeftToRight);

			for (auto& cell : rows.back().cellsInThisRow)
			{
				cell.div = &CreateDivision(row.InnerWidget());
				cell.div->Panel().SetExpandToParentHorizontally();
				cell.div->Panel().SetConstantHeight(spec.rowHeight);
				cell.div->Panel().SetLayoutDirection(ELayoutDirection::LeftToRight);
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

		int EstimateHeight() const override
		{
			int totalHeight = 0;

			for (auto& row : rows)
			{
				totalHeight += row.rowHeight;
			}

			return totalHeight;
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

					auto& cellpanel = cell.div->Panel();
					cellpanel.SetParentOffset({ x, y });
					cellpanel.SetConstantWidth(columnWidth);
					cellpanel.SetParentOffset({ x, y }).Resize({ columnWidth, row.rowHeight });
					cellpanel.InvalidateLayout(false);

					y += row.rowHeight;
				}

				x += columnWidth;
			}

			x = 0;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
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

		void Render(IGRRenderContext&) override
		{
			// g.DrawRectEdge(panel.AbsRect(), RGBAb(255, 0, 0, 255), RGBAb(255, 0, 0, 255));
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&)
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			if (ppOutputArg) *ppOutputArg = nullptr;
			if (!interfaceId || *interfaceId == 0) return EGRQueryInterfaceResult::INVALID_ID;

			if (DoInterfaceNamesMatch(interfaceId, IGRWidgetTable::InterfaceId()))
			{
				if (ppOutputArg)
				{
					*ppOutputArg = static_cast<IGRWidgetTable*>(this);					
				}

				return EGRQueryInterfaceResult::SUCCESS;
			}
			else if (DoInterfaceNamesMatch(interfaceId, IGRNavigator::InterfaceId()))
			{
				if (ppOutputArg)
				{
					*ppOutputArg = static_cast<IGRNavigator*>(this);
				}

				return EGRQueryInterfaceResult::SUCCESS;
			}

			return EGRQueryInterfaceResult::NOT_IMPLEMENTED;
		}

		struct RowAndColumn
		{
			int row;
			int column;

			bool operator == (RowAndColumn other) const
			{
				return row == other.row && column == other.column;
			}

			bool operator !=  (RowAndColumn other) const
			{
				return !(*this == other);
			}
		};

		RowAndColumn FindRowAndColumnOfChild(IGRPanel& childlId)
		{
			int y = 0;
			for (auto& row : rows)
			{
				int x = 0;
				for (auto& cell : row.cellsInThisRow)
				{
					if (cell.div && IsCandidateDescendantOfParent(cell.div->Panel(), childlId))
					{
						return { y, x };
					}
					x++;
				}
				y++;
			}

			return { -1,-1 };
		}

		RowAndColumn GetNextCell(RowAndColumn cellId, bool cycleToFirstRow)
		{
			int32 nColumns = (int32)columnHeaders.size();
			int32 nRows = (int32)rows.size();

			if (nColumns == 0 || nRows == 0 || cellId.column < 0 || cellId.row < 0)
			{
				return { -1, -1 };
			}

			int nextColumn = cellId.column + 1;
			int nextRow = cellId.row;

			if (nextColumn >= columnHeaders.size())
			{
				nextColumn = 0;
				nextRow++;
			}

			if (nextRow >= nRows)
			{
				if (cycleToFirstRow)
				{
					nextRow = 0;
				}
				else
				{
					return { -1, -1 };
				}
			}

			return { nextRow, nextColumn };
		}

		EGREventRouting OnNavigate(EGRNavigationDirective directive) override
		{
			auto focusId = panel.Root().GR().GetFocusId();
			auto* child = panel.Root().GR().FindWidget(focusId);
			if (!child)
			{
				return EGREventRouting::Terminate;
			}

			RowAndColumn cellId = FindRowAndColumnOfChild(child->Panel());

			bool cycleToFirstRow = panel.HasFlag(EGRPanelFlags::CycleTabsEndlessly);

			if (directive == EGRNavigationDirective::Tab)
			{
				for (RowAndColumn nextCellId = GetNextCell(cellId, cycleToFirstRow); nextCellId != cellId; nextCellId = GetNextCell(nextCellId, cycleToFirstRow))
				{
					if (nextCellId.column < 0)
					{
						break;
					}

					auto* div = rows[nextCellId.row].cellsInThisRow[nextCellId.column].div;
					if (div)
					{
						if (TrySetDeepFocus(div->Panel()))
						{
							return EGREventRouting::Terminate;
						}
					}
				}
			}

			return EGREventRouting::NextHandler;
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRTable";
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