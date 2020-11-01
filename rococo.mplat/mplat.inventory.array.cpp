#include <rococo.mplat.h>
#include <vector>
#include "rococo.script.types.h"
#include <rococo.textures.h>
#include <rococo.strings.h>

using namespace Rococo;

struct InventoryCell
{
	GuiRect guiRect = { 0,0,0,0 };
	int64 itemCount = 0;
	uint64 id = 0;
	int64 flags = 0;
};

namespace
{
	// The value is chosen such that the capacity squared never overflows the int32
	const int32 MAX_CAPACITY = 32767;

	struct PaperDoll
	{
		GuiRect rect;
		Textures::BitmapLocation bitmap;
		HString pingPath;
	};

	struct InventoryArray : IInventoryArraySupervisor
	{
		std::vector<InventoryCell> cells;
		std::vector<PaperDoll> dolls;

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

		void AddPaperDoll(const GuiRect& rect, const fstring& pingPathToImage) override
		{
			if (pingPathToImage.length < 2) Throw(0, "Expecting at least 2 characters in [pingPathToImage]");
			Textures::BitmapLocation bitmap{ {0,0,0,0}, 0, {0,0} };
			dolls.push_back(PaperDoll { rect, bitmap, pingPathToImage });
		}

		void ClearDolls() override
		{
			dolls.clear();
		}

		boolean32 GetDoll(int32 index, GuiRect& rect, IStringPopulator& sb, Rococo::Textures::BitmapLocation& bitmap) override
		{
			if (dolls.empty())
			{
				Throw(0, "No dolls");
			}

			if (index < 0 || index >= (int32)dolls.size())
			{
				Throw(0, "Bad doll index %d. Range is [0, %llu]", index, dolls.size());
			}

			const auto& d = dolls[index];
			rect = d.rect;
			sb.Populate(d.pingPath);
			bitmap = d.bitmap;

			return true;
		}

		void SetDollBitmap(int32 index, const Textures::BitmapLocation& bitmap) override
		{
			if (dolls.empty())
			{
				Throw(0, "No dolls");
			}

			if (index < 0 || index >= (int32)dolls.size())
			{
				Throw(0, "Bad doll index %d. Range is [0, %llu]", index, dolls.size());
			}

			dolls[index].bitmap = bitmap;
		}

		int32 DollCount()
		{
			return (int32) dolls.size();
		}

		int32 GetIndexAt(Vec2i& pos) override
		{
			for (int i = 0; i < (int32)cells.size(); i++)
			{
				if (IsPointInRect(pos, cells[i].guiRect))
				{
					return i;
				}
			}

			return -1;
		}

		void GetRect(int32 index, GuiRect& rect) override
		{
			rect = At(index).guiRect;
		}

		int32 NumberOfItems()
		{
			return (int32) cells.size();
		}

		void SetFlags(int32 index, int64 flags) override
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

		void SetRect(int32 index, const GuiRect& rect) override
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

		void ComputeSpan(const Rococo::InventoryLayoutRules& L, Vec2i& span) override
		{
			span.x = L.columns * (L.borders.x + L.cellSpan.x);
			span.y = L.rows * (L.borders.y + L.cellSpan.y);
		}

		void Swap(int32 i, int32 j) override
		{
			if (i < 0 || i >(int32) cells.size())
			{
				Throw(0, "%s: Bad i", __FUNCTION__);
			}

			if (j < 0 || j >(int32) cells.size())
			{
				Throw(0, "%s: Bad j", __FUNCTION__);
			}

			std::swap(cells[i].flags, cells[j].flags);
			std::swap(cells[i].id, cells[j].id);
			std::swap(cells[i].itemCount, cells[j].itemCount);
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

			int32 x0 = L.topLeft.x;
			int32 y0 = L.topLeft.y;

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