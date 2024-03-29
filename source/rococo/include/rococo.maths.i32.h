#pragma once

#include <rococo.types.h>

namespace Rococo
{
	Vec2i TopCentre(const GuiRect& rect);
	bool IsPointInRect(Vec2i p, const GuiRect& rect);

	inline bool operator == (const Vec2i& a, const Vec2i& b)
	{
		return a.x == b.x && a.y == b.y; 
	}

	inline bool operator != (const Vec2i& a, const Vec2i& b)
	{
		return !(a == b); 
	}

	inline bool operator != (const GuiRect& a, const GuiRect& b)
	{
		return a.left != b.left || a.right != b.right || a.bottom != b.bottom || a.top != b.top;
	}

	inline bool operator == (const GuiRect& a, const GuiRect& b)
	{
		return !(a != b);
	}

	bool IsOdd(int32 i);

	inline Vec2i Quantize(const Vec2& v)
	{
		return Vec2i{ (int32)v.x, (int32)v.y };
	}

	inline GuiRect Quantize(const GuiRectf& r)
	{
		return GuiRect{ (int32)r.left, (int32)r.top, (int32)r.right, (int32)r.bottom };
	}

	inline Vec2i operator - (const Vec2i& a, const Vec2i& b)
	{
		return Vec2i{ a.x - b.x, a.y - b.y };
	}

	inline Vec2i operator + (const Vec2i& a, const Vec2i& b)
	{
		return Vec2i{ a.x + b.x, a.y + b.y };
	}

	inline GuiRect Expand(const GuiRect& rect, int32 pixels)
	{
		return GuiRect(rect.left - pixels, rect.top - pixels, rect.right + pixels, rect.bottom + pixels);
	}

	inline Vec2i Centre(const GuiRect& q) 
	{
		return Vec2i{ (q.left + q.right) >> 1, (q.top + q.bottom) >> 1 };
	}

	inline int32 Width(const GuiRect& q) 
	{
		return q.right - q.left;
	}

	inline int32 Height(const GuiRect& q) 
	{ 
		return q.bottom - q.top;
	}

	inline Vec2i Span(const GuiRect& q) 
	{
		return Vec2i{ Width(q), Height(q) };
	}

	inline Vec2i TopLeft(const GuiRect& q)
	{ 
		return Vec2i{ q.left, q.top };
	}

	inline Vec2i BottomRight(const GuiRect& q) 
	{
		return Vec2i{ q.right, q.bottom };
	}

	inline Vec2i TopRight(const GuiRect& q) 
	{ 
		return Vec2i{ q.right, q.top };
	}

	inline Vec2i BottomLeft(const GuiRect& q) 
	{ 
		return Vec2i{ q.left, q.bottom };
	}

	inline GuiRect operator + (const GuiRect& rect, const Vec2i& delta)
	{
		return GuiRect(rect.left + delta.x, rect.top + delta.y, rect.right + delta.x, rect.bottom + delta.y);
	}

	// Intersect normalized rectangles. If either rectangle is not normalized, or if there is no intersection returns {0,0,0,0}. 
	inline GuiRect IntersectNormalizedRects(const GuiRect& a, const GuiRect& b)
	{
		const GuiRect GuiRectNull = { 0,0,0,0 };

		if (!a.IsNormalized() || !b.IsNormalized())
		{
			return GuiRectNull;
		}

		if (a.left >= b.right || a.right <= b.left)
		{
			return GuiRectNull;
		}

		if (a.top >= b.bottom || a.bottom <= b.top)
		{
			return GuiRectNull;
		}

		return GuiRect{ max(a.left, b.left), max(a.top, b.top), min(a.right, b.right), min(a.bottom, b.bottom) };
	}
}