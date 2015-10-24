#include "dystopia.h"
#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "rococo.fonts.h"

#include <string>

#include <vector>

using namespace Rococo;
using namespace Dystopia;

namespace
{
	Fonts::FontColour FontColourFromRGBAb(RGBAb colour)
	{
		Fonts::FontColour* pCol = (Fonts::FontColour*) &colour;
		return *pCol;
	}

	Quad GuiRectToQuad(const GuiRect& rect)
	{
		return Quad((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom);
	}

	class LeftAlignedText : public Fonts::IDrawTextJob
	{
	private:
		const wchar_t* text;
		Fonts::FontColour colour;
		int fontIndex;
		float lastCellHeight;
		Quad targetRect;
		int retzone;
		int hypzone;
	public:
		LeftAlignedText(const GuiRect& _targetRect, int _retzone, int _hypzone, int _fontIndex, const wchar_t* _text, Fonts::FontColour _colour) :
			targetRect(GuiRectToQuad(_targetRect)), retzone(_retzone), hypzone(_hypzone),
			text(_text), colour(_colour), fontIndex(_fontIndex), lastCellHeight(10.0f)
		{
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			Quad outputRect;
			builder.AppendChar(c, outputRect);

			lastCellHeight = outputRect.bottom - outputRect.top;
		}

		virtual void OnDraw(Fonts::IGlyphBuilder& builder)
		{
			builder.SetTextColour(colour);
			builder.SetShadow(false);
			builder.SetFontIndex(fontIndex);

			builder.SetClipRect(Quad((float)targetRect.left, (float)targetRect.top, (float)targetRect.right, (float)targetRect.bottom));

			builder.SetCursor(Vec2{ targetRect.left, targetRect.top });

			const float retX = targetRect.right - retzone;
			const float hypX = targetRect.right - hypzone;

			for (const wchar_t* p = text; *p != 0; p++)
			{
				wchar_t c = *p;

				Vec2 nextGlyphPos = builder.GetCursor();

				if (c >= 255) c = '?';

				if (c == '\t')
				{
					if (nextGlyphPos.x > retX)
					{
						builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
						continue;
					}

					for (int i = 0; i < 4; ++i)
					{
						DrawNextGlyph(' ', builder);
					}
				}
				else if (c == ' ')
				{
					if (nextGlyphPos.x > retX)
					{
						builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
						continue;
					}

					DrawNextGlyph(' ', builder);
				}
				else if (c == '\n')
				{
					builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
				}
				else
				{
					if (nextGlyphPos.x > hypX)
					{
						DrawNextGlyph('-', builder);
						builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
						DrawNextGlyph(' ', builder);
						DrawNextGlyph('-', builder);
					}
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};

	class HorizontalCentredText : public Fonts::IDrawTextJob
	{
	private:
		const wchar_t* text;
		Fonts::FontColour colour;
		int fontIndex;
		float lastCellHeight;
	public:
		Quad target;

		HorizontalCentredText(int _fontIndex, const wchar_t* _text, Fonts::FontColour _colour) :
			text(_text), colour(_colour), fontIndex(_fontIndex), lastCellHeight(10.0f)
		{
			float minFloat = std::numeric_limits<float>::min();
			float maxFloat = std::numeric_limits<float>::max();
			target = Quad(maxFloat, maxFloat, minFloat, minFloat);
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			Quad outputRect;
			builder.AppendChar(c, outputRect);

			lastCellHeight = outputRect.bottom - outputRect.top;

			if (outputRect.left < target.left) target.left = outputRect.left;
			if (outputRect.right > target.right) target.right = outputRect.right;
			if (outputRect.bottom > target.bottom) target.bottom = outputRect.bottom;
			if (outputRect.top < target.top) target.top = outputRect.top;
		}

		virtual void OnDraw(Fonts::IGlyphBuilder& builder)
		{
			builder.SetTextColour(colour);
			builder.SetShadow(false);
			builder.SetFontIndex(fontIndex);

			float fMin = -1000000.0f;
			float fMax = 1000000.0f;
			builder.SetClipRect(Quad(fMin, fMin, fMax, fMax));

			Vec2 firstGlyphPos = builder.GetCursor();

			for (const wchar_t* p = text; *p != 0; p++)
			{
				wchar_t c = *p;

				if (c >= 255) c = '?';

				if (c == '\t')
				{
					for (int i = 0; i < 4; ++i)
					{
						DrawNextGlyph(' ', builder);
					}
				}
				else if (c == '\n')
				{
					Vec2 nextGlyphPos = builder.GetCursor();
					builder.SetCursor(Vec2{ firstGlyphPos.x, nextGlyphPos.y + lastCellHeight });
				}
				else
				{
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};

	void DrawRectangle(IGuiRenderContext& grc, const GuiRect& grect, RGBAb diag, RGBAb backdiag)
	{
		Quad rect{ (float)grect.left, (float)grect.top, (float)grect.right, (float)grect.bottom };
		GuiVertex q[] =
		{
			{ rect.left,  rect.top,    1.0f, 0, diag,     0, 0, 0 },
			{ rect.right, rect.top,    1.0f, 0, backdiag, 0, 0, 0 },
			{ rect.right, rect.bottom, 1.0f, 0, diag,     0, 0, 0 },
			{ rect.right, rect.bottom, 1.0f, 0, diag,     0, 0, 0 },
			{ rect.left,  rect.bottom, 1.0f, 0, backdiag, 0, 0, 0 },
			{ rect.left,  rect.top,    1.0f, 0, diag,     0, 0, 0 },
		};

		grc.AddTriangle(q);
		grc.AddTriangle(q + 3);
	}

	Vec2i TopLeft(const GuiRect& rect) { return Vec2i{ rect.left, rect.top }; }
	Vec2i Span(const GuiRect& rect) { return Vec2i{ rect.right - rect.left, rect.bottom - rect.top }; }

	void DrawBorderAround(IGuiRenderContext& grc, const GuiRect& rect, const Vec2i& width, RGBAb diag, RGBAb backdiag)
	{
		GuiRect topRect{ rect.left - width.x, rect.top - width.y, rect.right, rect.top };
		DrawRectangle(grc, topRect, diag, diag);

		GuiRect bottomRect{ rect.left - width.x, rect.bottom, rect.right, rect.bottom + width.y };
		DrawRectangle(grc, bottomRect, backdiag, backdiag);

		GuiRect leftRect{ rect.left - width.x, rect.top, rect.left, rect.bottom };
		DrawRectangle(grc, leftRect, backdiag, backdiag);

		GuiRect rightRect{ rect.right, rect.top - width.y, rect.right + width.x, rect.bottom + width.y };
		DrawRectangle(grc, rightRect, diag, diag);
	}

	Vec2i GetScreenCentre(const GuiMetrics& metrics)
	{
		return Vec2i{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };
	}

	Vec2i RenderTextVCentred(IGuiRenderContext& grc, int32 x, int32 top, RGBAb colour, const wchar_t* text, int fontIndex)
	{
		HorizontalCentredText job(fontIndex, text, FontColourFromRGBAb(colour));
		Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
		grc.RenderText(Vec2i{ x - (span.x >> 1), top }, job);
		return span;
	}

	Vec2i TopCentre(const GuiRect& rect)
	{
		return Vec2i{ (rect.left + rect.right) >> 1, rect.top };
	}

	bool IsPointInRect(const Vec2i& p, const GuiRect& rect)
	{
		return (p.x > rect.left && p.x < rect.right && p.y > rect.top && p.y < rect.bottom);
	}

	struct ContextMenuItemImpl
	{
		std::wstring buttonName;
		int32 commandId;
		int64 context;
		GuiRect lastRenderedRect;
		bool isActive;
	};

	class ContextMenu : public IUIPaneSupervisor
	{
		Environment& e;
		std::vector<ContextMenuItemImpl> items;
		Vec2i topLeft;
		GuiRect lastRenderedRect;
		ContextMenuItemImpl lastHighlight;
		IEventCallback<ContextMenuItem>& onClick;
	
	public:
		ContextMenu(Environment& _e, Vec2i _topLeft, ContextMenuItem newMenu[], IEventCallback<ContextMenuItem>& _onClick): 
			e(_e), onClick(_onClick), topLeft(_topLeft)
		{
			for (auto cmi = newMenu; cmi->buttonName != nullptr; cmi++)
			{
				items.push_back({ cmi->buttonName, cmi->commandId, cmi->context,{ 0,0,0,0 }, cmi->isActive });
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual PaneModality OnFrameUpdated(const IUltraClock& clock)
		{
			return PaneModality_Modeless;
		}

		virtual PaneModality OnKeyboardEvent(const KeyboardEvent& ke)
		{
			return PaneModality_Modeless;
		}

		virtual PaneModality OnMouseEvent(const MouseEvent& me)
		{
			if (me.HasFlag(MouseEvent::LUp))
			{
				if (!lastHighlight.buttonName.empty())
				{
					ContextMenuItem value{ lastHighlight.buttonName.c_str(), lastHighlight.commandId, lastHighlight.context };
					e.uiStack.PopTop();
					onClick.OnEvent(value);
					Free();
				}	
			}

			return PaneModality_Modal;
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
		}

		virtual void RenderGui(IGuiRenderContext& gc)
		{
			const int X_BORDER = 6;
			const int Y_BORDER = 4;

			Vec2i totalSpan = { 0,Y_BORDER };

			int fontHeight = 0;

			for (auto i : items)
			{
				HorizontalCentredText item(3, i.buttonName.c_str(), 0xFFFFFFFF);
				auto span = gc.EvalSpan({ 0,0 }, item);
				totalSpan.y += span.y + Y_BORDER;
				totalSpan.x = max(span.x, totalSpan.x);
				fontHeight = span.y;
			}

			totalSpan.x += 2 * X_BORDER;

			lastRenderedRect = GuiRect(topLeft.x, topLeft.y, topLeft.x + totalSpan.x, topLeft.y + totalSpan.y);

			DrawRectangle(gc, lastRenderedRect, RGBAb(64, 0, 0), RGBAb(0, 64, 0));
			DrawBorderAround(gc, lastRenderedRect, { 2,2 }, RGBAb(128, 128, 128), RGBAb(192, 192, 192));

			Vec2i p = topLeft + Vec2i{ X_BORDER, Y_BORDER };

			GuiMetrics metrics;
			gc.Renderer().GetGuiMetrics(metrics);

			lastHighlight.buttonName.clear();

			for (auto i : items)
			{
				RGBAb fontColour(192, 192, 192);
				i.lastRenderedRect = GuiRect(p.x - X_BORDER, p.y, p.x + totalSpan.x - X_BORDER, p.y + fontHeight);

				if (IsPointInRect(metrics.cursorPosition, i.lastRenderedRect) && i.isActive)
				{
					lastHighlight = i;
					DrawRectangle(gc, i.lastRenderedRect, RGBAb(128, 0, 0), RGBAb(0, 0, 128));
					fontColour = RGBAb(255, 255, 255);
				}

				if (!i.isActive) fontColour = RGBAb(64, 64, 64);

				HorizontalCentredText item(3, i.buttonName.c_str(), FontColourFromRGBAb(fontColour));
				gc.RenderText(p, item);
				p.y += fontHeight + Y_BORDER;
			}
		}
	};
}

namespace Dystopia
{
	IUIPaneSupervisor* CreateContextMenu(Environment& e, Vec2i topLeft, ContextMenuItem newMenu[], IEventCallback<ContextMenuItem>& onClick)
	{
		return new ContextMenu(e, topLeft, newMenu, onClick);
	}
}