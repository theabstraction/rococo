#include <rococo.mplat.h>
#include <vector>

using namespace Rococo;

struct InventoryLayoutRules
{
	int32 rows;
	int32 columns;
	Vec2 cellSpan;
	Vec2 borders;
	Vec2 topLeft;
	boolean32 rowByRow;
	int32 startIndex;
	int32 endIndex;
};

struct InventoryCell
{
	GuiRectf guiRect = { 0,0,0,0 };
	int64 itemCount = 0;
	uint64 id = 0;
	int64 flags = 0;
};

ROCOCOAPI IInventoryRenderScheme
{
	virtual void Render(IGuiRenderContext& g, const InventoryCell & cell) = 0;
};

ROCOCOAPI IInventoryArray
{
	virtual void GetRect(int32 index, GuiRectf& rect) = 0;
	virtual int64 Flags(int32 index) = 0;
	virtual int64 Id(int32 index) = 0;
	virtual int64 ItemCount(int32 index) = 0;
	virtual void SetFlags(int32 index, int64 flags) = 0;
	virtual void SetId(int32 index, int64 count) = 0;
	virtual void SetItemCount(int32 index, int64 count) = 0;
	virtual void SetRect(int32 index, const GuiRectf& rect) = 0;
	virtual void LayoutAsRect(const InventoryLayoutRules& rules) = 0;
};

ROCOCOAPI IInventoryArraySupervisor : IInventoryArray
{
	virtual void Render(IInventoryRenderScheme& scheme, IGuiRenderContext& g) = 0;
	virtual void Free() = 0;
};

namespace
{
	// The value is chosen such that the capacity squared never overflows the int32
	const int32 MAX_CAPACITY = 32767;

	struct InventoryArray : IInventoryArraySupervisor
	{
		std::vector<InventoryCell> cells;

		InventoryCell& At(int32 index)
		{
			size_t bigIndex = (size_t)index;
			if (bigIndex >= cells.size())
			{
				Throw(0, "Bad cell index: %d. Max value is %llu", index, bigIndex - 1);
			}

			return cells[bigIndex];
		}

		InventoryArray(size_t capacity): cells(capacity)
		{
		}

		void GetRect(int32 index, GuiRectf& rect) override
		{
			rect = At(index).guiRect;
		}

		void SetFlags (int32 index, int64 flags) override
		{
			At(index).flags = flags;
		}

		void SetId(int32 index, int64 id) override
		{
			At(index).id = id;
		}

		void SetItemCount(int32 index, int64 count) override
		{
			At(index).itemCount = count;
		}

		void SetRect(int32 index, const GuiRectf& rect) override
		{
			At(index).guiRect = rect;
		}

		int64 Flags(int32 index) override
		{
			return At(index).flags;
		}

		int64 ItemCount(int32 index) override
		{
			return At(index).itemCount;
		}

		int64 Id(int32 index) override
		{
			return At(index).id;
		}

		void Free() override
		{
			delete this;
		}

		void Render(IInventoryRenderScheme& scheme, IGuiRenderContext& g)
		{
			for (auto& c : cells)
			{
				scheme.Render(g, c);
			}
		}

		void LayoutAsRect(const InventoryLayoutRules& L) override
		{
			if (L.rows < 1 || L.rows > MAX_CAPACITY)
			{
				Throw(0, "rows range is [1, %d]", MAX_CAPACITY);
			}

			if (L.columns < 1 || L.columns > MAX_CAPACITY)
			{
				Throw(0, "columns range is [1, %d]", MAX_CAPACITY);
			}

			if (L.startIndex < 0 || L.startIndex > MAX_CAPACITY)
			{
				Throw(0, "startIndex range is [1, %d]", MAX_CAPACITY);
			}

			if (L.endIndex < L.startIndex || L.endIndex > MAX_CAPACITY)
			{
				Throw(0, "startIndex range is [%d, %d]", L.startIndex, MAX_CAPACITY);
			}

			int32 nCells = L.endIndex - L.startIndex;

			if ((L.rows * L.columns) != nCells)
			{
				Throw(0, "The product of rows * columns (%d x %d) must be %d", L.rows, L.columns, nCells);
			}

			float x0 = L.topLeft.x;
			float y0 = L.topLeft.y;

			int32 index = L.startIndex;

			if (L.rowByRow)
			{
				for (int j = 0; j < L.rows; ++j)
				{
					x0 = L.topLeft.x;

					for (int i = 0; i < L.columns; ++i)
					{
						auto& c = At(index++);
						c.guiRect.left = x0;
						c.guiRect.top = y0;
						c.guiRect.right = x0 + L.cellSpan.x;
						c.guiRect.bottom = y0 + L.cellSpan.y;

						x0 += L.cellSpan.x + L.borders.x;
					}

					y0 += L.cellSpan.y + L.borders.y;
				}
			}
			else
			{
				for (int j = 0; j < L.columns; ++j)
				{
					y0 = L.topLeft.y;

					for (int i = 0; i < L.rows; ++i)
					{
						auto& c = At(index++);
						c.guiRect.left = x0;
						c.guiRect.top = y0;
						c.guiRect.right = x0 + L.cellSpan.x;
						c.guiRect.bottom = y0 + L.cellSpan.y;

						y0 += L.cellSpan.y + L.borders.y;
					}

					x0 += L.cellSpan.x + L.borders.x;
				}
			}
		}
	};
}

namespace Rococo
{
	IInventoryArraySupervisor* CreateInventoryArray(int32 capacity)
	{
		if (capacity < 1 || capacity > MAX_CAPACITY)
		{
			Throw(0, "%s: Bad capacity %d. The value must be between 1 and %d", __FUNCTION__, capacity, MAX_CAPACITY);
		}

		return new InventoryArray((size_t) capacity);
	}
}