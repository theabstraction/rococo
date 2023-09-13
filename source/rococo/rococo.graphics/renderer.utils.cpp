#include <rococo.types.h>
#define ROCOCO_GRAPHICS_API ROCOCO_API_EXPORT
#include <rococo.api.h>
#include <rococo.os.h>
#include <rococo.renderer.h>
#include <rococo.fonts.h>
#include <rococo.fonts.hq.h>

#include <limits>
#include <new>

#include <rococo.maths.h>
#include <rococo.time.h>

#ifndef ROCOCO_UTILS_EX_API
#error "ROCOCO_UTILS_EX_API undefined";
#endif

namespace Rococo::Graphics::Impl
{
	using namespace Rococo;
	using namespace Rococo::Graphics;
	using namespace Rococo::Graphics::Fonts;

	Fonts::FontColour FontColourFromRGBAb(RGBAb colour)
	{
		Fonts::FontColour* pCol = (Fonts::FontColour*) & colour;
		return *pCol;
	}

	GuiRectf GuiRectToQuad(const GuiRect& rect)
	{
		return GuiRectf((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom);
	}

	GuiRectf Intersect(const GuiRectf& a, const GuiRectf& b)
	{
		GuiRectf q;
		q.left = max(a.left, b.left);
		q.right = min(a.right, b.right);
		q.top = max(a.top, b.top);
		q.bottom = min(a.bottom, b.bottom);
		return q;
	}

	struct GlyphArgs
	{
		int32 index;
		GuiRectf rect;
	};

	class BasicTextJob : public Fonts::IDrawTextJob
	{
	private:
		cstr text;
		Fonts::FontColour colour;
		int fontIndex;
		int fontHeight;
		IEventCallback<GlyphArgs>* cb;
		int count = 0;
	public:
		GuiRectf target;

		BasicTextJob(int _fontIndex, cstr _text, RGBAb _colour, int _fontHeight, IEventCallback<GlyphArgs>* _cb = nullptr) :
			text(_text), colour((Fonts::FontColour&)_colour), fontIndex(_fontIndex), fontHeight(_fontHeight), cb(_cb)
		{
			float minFloat = std::numeric_limits<float>::min();
			float maxFloat = std::numeric_limits<float>::max();
			target = GuiRectf(maxFloat, maxFloat, minFloat, minFloat);
		}

		void Reset(IEventCallback<GlyphArgs>* cb)
		{
			count = 0;
			this->cb = cb;
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			GuiRectf outputRect;
			builder.AppendChar(c, outputRect);

			if (cb)
			{
				GlyphArgs args;
				args.index = count++;
				args.rect = outputRect;
				cb->OnEvent(args);
			}

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
			builder.SetFontHeight(fontHeight);

			Vec2 firstGlyphPos = builder.GetCursor();

			for (cstr p = text; *p != 0; p++)
			{
				char c = *p;

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
					builder.SetCursor(Vec2{ firstGlyphPos.x, nextGlyphPos.y + fontHeight });
				}
				else
				{
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};

	class LeftAlignedTextJob : public Fonts::IDrawTextJob
	{
	private:
		cstr text;
		Fonts::FontColour colour;
		int fontIndex;
		int fontHeight;
		IEventCallback<GlyphArgs>* cb;
		int count = 0;
		GuiRectf bounds;

		float rightMaxReturnX;
		float rightAlignX;
	public:
		GuiRectf target;

		LeftAlignedTextJob(int _fontIndex, float hardRightEdge, float softRightEdge, cstr _text, RGBAb _colour, const GuiRectf& _bounds, int32 _fontHeight, IEventCallback<GlyphArgs>* _cb = nullptr) :
			text(_text), colour((Fonts::FontColour&)_colour), fontIndex(_fontIndex), fontHeight(_fontHeight), cb(_cb), bounds(_bounds)
		{
			float minFloat = std::numeric_limits<float>::min();
			float maxFloat = std::numeric_limits<float>::max();
			target = GuiRectf(maxFloat, maxFloat, minFloat, minFloat);
			rightMaxReturnX = bounds.right - hardRightEdge;
			rightAlignX = bounds.right - softRightEdge;
		}

		void Reset(IEventCallback<GlyphArgs>* cb)
		{
			count = 0;
			this->cb = cb;
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			GuiRectf outputRect;
			builder.AppendChar(c, outputRect);

			if (cb)
			{
				GlyphArgs args;
				args.index = count++;
				args.rect = outputRect;
				cb->OnEvent(args);
			}

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
			builder.SetFontHeight(fontHeight);

			Vec2 firstGlyphPos = builder.GetCursor();

			for (cstr p = text; *p != 0; p++)
			{
				char c = *p;

				if (c >= 255) c = '?';

				Vec2 pos = builder.GetCursor();

				if (pos.x >= rightAlignX)
				{
					if (c <= 32)
					{
						pos.x = bounds.left;
						pos.y += (float)fontHeight;
						builder.SetCursor(pos);
						continue;
					}
					else if (pos.x >= rightMaxReturnX)
					{
						pos.x = bounds.left;
						pos.y += (float)fontHeight;
						builder.SetCursor(pos);
					}
				}

				if (c == '\t')
				{
					for (int i = 0; i < 4; ++i)
					{
						DrawNextGlyph(' ', builder);
					}

					pos = builder.GetCursor();

					if (pos.x >= rightAlignX)
					{
						pos.x = bounds.left;
						pos.y += (float)fontHeight;
						builder.SetCursor(pos);
					}
				}
				else if (c == '\n')
				{
					pos.x = bounds.left;
					pos.y += (float)fontHeight;
					builder.SetCursor(pos);
				}
				else
				{
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};

	class HorizontalCentredText : public Fonts::IDrawTextJob
	{
	private:
		cstr text;
		Fonts::FontColour colour;
		int fontIndex;
		int fontHeight;
		float lastCellHeight;
		IEventCallback<Rococo::Graphics::GlyphCallbackArgs>* cb;
		int count = 0;
	public:
		GuiRectf target;

		HorizontalCentredText(
			int _fontIndex, 
			int _fontHeight, 
			cstr _text, 
			Fonts::FontColour _colour, 
			IEventCallback<Rococo::Graphics::GlyphCallbackArgs>* _cb = nullptr
		) :
			fontIndex(_fontIndex),
			fontHeight(_fontHeight),
			text(_text),
			colour(_colour),
			lastCellHeight((float)_fontHeight),
			cb(_cb)
		{
			constexpr float minFloat = std::numeric_limits<float>::min();
			constexpr float maxFloat = std::numeric_limits<float>::max();
			target = GuiRectf(maxFloat, maxFloat, minFloat, minFloat);
		}

		void Reset(IEventCallback<Rococo::Graphics::GlyphCallbackArgs>* cb)
		{
			count = 0;
			this->cb = cb;
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			GuiRectf outputRect;

			builder.SetFontIndex(fontIndex);
			builder.SetFontHeight(fontHeight);
			builder.AppendChar(c, outputRect);

			if (cb)
			{
				Rococo::Graphics::GlyphCallbackArgs args;
				args.index = count++;
				args.rect.left = (int32)outputRect.left;
				args.rect.right = (int32)outputRect.right;
				args.rect.top = (int32)outputRect.top;
				args.rect.bottom = (int32)outputRect.bottom;
				cb->OnEvent(args);
			}

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

			Vec2 firstGlyphPos = builder.GetCursor();

			for (cstr p = text; *p != 0; p++)
			{
				char c = *p;

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

	class LeftAlignedText : public Fonts::IDrawTextJob
	{
	private:
		cstr text;
		Fonts::FontColour colour;
		int fontIndex;
		int fontHeight;
		float lastCellHeight;
		GuiRectf targetRect;
		int retzone;
		int hypzone;
	public:
		LeftAlignedText(const GuiRect& _targetRect, int _retzone, int _hypzone, int _fontHeight, int _fontIndex, cstr _text, Fonts::FontColour _colour) :
			targetRect(GuiRectToQuad(_targetRect)), retzone(_retzone), hypzone(_hypzone),
			text(_text), colour(_colour), fontHeight(_fontHeight), fontIndex(_fontIndex), lastCellHeight(10.0f)
		{
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			GuiRectf outputRect;
			builder.AppendChar(c, outputRect);

			lastCellHeight = outputRect.bottom - outputRect.top;
		}

		virtual void OnDraw(Fonts::IGlyphBuilder& builder)
		{
			builder.SetTextColour(colour);
			builder.SetShadow(false);
			builder.SetFontIndex(fontIndex);
			builder.SetFontHeight(fontHeight);
			
			auto outerClipRect = builder.GetClipRect();
			auto innerClipRect = targetRect;
			
			builder.SetClipRect(Intersect(outerClipRect, innerClipRect));

			builder.SetCursor(Vec2{ targetRect.left, targetRect.top });

			const float retX = targetRect.right - retzone;
			const float hypX = targetRect.right - hypzone;

			for (cstr p = text; *p != 0; p++)
			{
				char c = *p;

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
						DrawNextGlyph('-', builder);
					}
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};
}

using namespace Rococo::Graphics::Impl;

namespace Rococo::Graphics
{
	ROCOCO_GRAPHICS_API void EvalTextSpan(IGuiRenderContext& g, const fstring& text, int32 fontIndex, Vec2& pixelSpan)
	{
		BasicTextJob job(fontIndex, text, 0xFFFFFFFF, (int)pixelSpan.y);
		auto iSpan = g.EvalSpan({ 0,0 }, job);
		pixelSpan.x = (float)iSpan.x;
		pixelSpan.y = (float)iSpan.y;
	}

	ROCOCO_GRAPHICS_API Vec2i GetTopLeftPos(const GuiRect& rect, Vec2i span, int32 alignmentFlags)
	{
		Vec2i pos;

		if (IsFlagged(alignmentFlags, Alignment_Left) && !IsFlagged(alignmentFlags, Alignment_Right))
		{
			pos.x = rect.left;
		}
		else if (!IsFlagged(alignmentFlags, Alignment_Left) && IsFlagged(alignmentFlags, Alignment_Right))
		{
			pos.x = rect.right - span.x;
		}
		else
		{
			pos.x = ((rect.left + rect.right - span.x) >> 1);
		}

		if (IsFlagged(alignmentFlags, Alignment_Top) && !IsFlagged(alignmentFlags, Alignment_Bottom))
		{
			pos.y = rect.top;
		}
		else if (!IsFlagged(alignmentFlags, Alignment_Top) && IsFlagged(alignmentFlags, Alignment_Bottom))
		{
			pos.y = rect.bottom - span.y;
		}
		else
		{
			pos.y = ((rect.top + rect.bottom + span.y) >> 1) - span.y;
		}

		return pos;
	}

	ROCOCO_GRAPHICS_API Vec2 GetTopLeftPos(Vec2 pos, Vec2 span, int32 alignmentFlags)
	{
		Vec2 topLeftPos;

		if (HasFlag(alignmentFlags, Alignment_Left) && !HasFlag(alignmentFlags, Alignment_Right))
		{
			topLeftPos.x = pos.x;
		}
		else if (!HasFlag(alignmentFlags, Alignment_Left) && HasFlag(alignmentFlags, Alignment_Right))
		{
			topLeftPos.x = pos.x - span.x;
		}
		else // Centred horizontally
		{
			topLeftPos.x = pos.x - 0.5f * span.x;
		}

		if (HasFlag(alignmentFlags, Alignment_Top) && !HasFlag(alignmentFlags, Alignment_Bottom))
		{
			topLeftPos.y = pos.y;
		}
		else if (!HasFlag(alignmentFlags, Alignment_Top) && HasFlag(alignmentFlags, Alignment_Bottom))
		{
			topLeftPos.y = pos.y - span.y;
		}
		else // Centred vertically
		{
			topLeftPos.y = pos.y - 0.5f * span.y;
		}

		return topLeftPos;
	}

	ROCOCO_GRAPHICS_API void DrawClippedText(IGuiRenderContext& g, const Rococo::GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour, const Rococo::GuiRectf& clipRect)
	{
		GuiRect iRect{ (int32)rect.left, (int32)rect.top, (int32)rect.right, (int32)rect.bottom };
		if (text.length)
		{
			BasicTextJob job(fontIndex, text, colour, Height(iRect));
			Vec2i span = g.EvalSpan({ 0,0 }, job);
			Vec2i topLeft = GetTopLeftPos(iRect, span, alignmentFlags);

			GuiRect clipRecti{ (int32)clipRect.left, (int32)clipRect.top, (int32)clipRect.right, (int32)clipRect.bottom };
			g.RenderText(topLeft, job, &clipRecti);
		}
	}

	ROCOCO_GRAPHICS_API void DrawTextWithCaret(IGuiRenderContext& g, const Rococo::GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour, const Rococo::GuiRectf& clipRect, int32 caretPos)
	{
		GuiRect iRect{ (int32)rect.left, (int32)rect.top, (int32)rect.right, (int32)rect.bottom };

		auto t = Time::TickCount();
		auto quarterSecond = Time::TickHz() >> 2;

		auto counter = t / quarterSecond;

		bool isCaretLit = (counter % 2) == 0;

		struct : IEventCallback<GlyphArgs>
		{
			int caretPos;

			GuiRectf caretGlyphRect = { -1,-1,-1,-1 };
			GuiRectf lastGlyphRect = { -1, -1, -1, -1 };

			void OnEvent(GlyphArgs& args) override
			{
				lastGlyphRect = args.rect;

				if (caretPos == args.index)
				{
					caretGlyphRect = args.rect;
				}
			}
		} onGlyph;

		onGlyph.caretPos = caretPos;

		BasicTextJob job(fontIndex, text, colour, Height(iRect), &onGlyph);
		Vec2i span = g.EvalSpan({ 0,0 }, job);

		job.Reset(&onGlyph);

		Vec2i topLeft = GetTopLeftPos(iRect, span, alignmentFlags);

		GuiRect clipRecti{ (int32)clipRect.left, (int32)clipRect.top, (int32)clipRect.right, (int32)clipRect.bottom };
		g.RenderText(topLeft, job, &clipRecti);

		if (onGlyph.caretGlyphRect.left > -1)
		{
			Vec2i caretStart = { (int32)onGlyph.caretGlyphRect.left,  (int32)onGlyph.caretGlyphRect.bottom };
			Vec2i caretEnd = { (int32)onGlyph.caretGlyphRect.right, (int32)onGlyph.caretGlyphRect.bottom };
			Rococo::Graphics::DrawLine(g, 2, caretStart, caretEnd, isCaretLit ? 0 : 0xFFFFFFFF);
		}
		else if (onGlyph.lastGlyphRect.right > -1)
		{
			auto ds = Span(rect);
			float defaultCaretSpan = 0.75f * ds.y;
			Vec2i caretStart = { (int32)onGlyph.lastGlyphRect.right,  (int32)rect.bottom };
			Vec2i caretEnd = { caretStart.x + (int32)defaultCaretSpan, caretStart.y };
			Rococo::Graphics::DrawLine(g, 2, caretStart, caretEnd, isCaretLit ? 0 : 0xFFFFFFFF);
		}
		else
		{
			auto ds = Span(rect);
			float defaultCaretSpan = 0.75f * ds.y;
			Vec2i caretStart = { (int32)rect.left,  (int32)rect.bottom };
			Vec2i caretEnd = { caretStart.x + (int32)defaultCaretSpan, caretStart.y };
			Rococo::Graphics::DrawLine(g, 2, caretStart, caretEnd, isCaretLit ? 0 : 0xFFFFFFFF);
		}
	}

	ROCOCO_GRAPHICS_API void DrawLeftAligned(IGuiRenderContext& g, const Rococo::GuiRectf& rect, const fstring& text, int32 fontIndex, int32 fontHeight, RGBAb colour, float32 softRightEdge, float32 hardRightEdge)
	{
		GuiRect iRect{ (int32)rect.left, (int32)rect.top, (int32)rect.right, (int32)rect.bottom };
		if (text.length)
		{
			LeftAlignedTextJob job(fontIndex, softRightEdge, hardRightEdge, text, colour, rect, fontHeight);

			Vec2i topLeft = { iRect.left, iRect.top };
			g.RenderText(topLeft, job, &iRect);
		}
	}

	ROCOCO_GRAPHICS_API void DrawText(IGuiRenderContext& g, const GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour)
	{
		GuiRect iRect{ (int32)rect.left, (int32)rect.top, (int32)rect.right, (int32)rect.bottom };
		if (text.length)
		{
			BasicTextJob job(fontIndex, text, colour, (int)Height(rect));
			Vec2i span = g.EvalSpan({ 0,0 }, job);
			Vec2i topLeft = GetTopLeftPos(iRect, span, alignmentFlags);

			g.RenderText(topLeft, job, HasFlag(alignmentFlags, Alignment_Clipped) ? &iRect : nullptr);
		}
	}

	ROCOCO_GRAPHICS_API void DrawTexture(IGuiRenderContext& grc, ID_TEXTURE id, const GuiRect& absRect)
	{
		SpriteVertexData ignore{ 0.0f, 0.0f, 0.0f, 1.0f };
		RGBAb none(0, 0, 0, 0);

		GuiVertex quad[6] =
		{
			{
				{ (float)absRect.left, (float)absRect.top },
				{ { 0, 0 }, 0 },
				ignore,
				none
			},
			{
				{ (float)absRect.right, (float)absRect.top },
				{ { 1, 0 }, 0 },
				ignore,
				none
			},
			{
				{ (float)absRect.right,(float)absRect.bottom },
				{ { 1, 1 }, 0 },
				ignore,
				none
			},
			{
				{ (float)absRect.right, (float)absRect.bottom },
				{ { 1, 1 }, 0 },
				ignore,
				none
			},
			{
				{ (float)absRect.left, (float)absRect.bottom },
				{ { 0, 1 }, 0 },
				ignore,
				none
			},
			{
				{ (float)absRect.left, (float)absRect.top },
				{ { 0, 0 }, 0 },
				ignore,
				none
			}
		};

		grc.DrawCustomTexturedMesh(absRect, id, "!gui.texture.ps", quad, 6);
	}

	ROCOCO_GRAPHICS_API void DrawTriangleFacingLeft(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour)
	{
		BaseVertexData noFont{ { 0, 0 }, 0 };
		SpriteVertexData solid{ 1.0f, 0, 0, 0 };

		GuiVertex triangle[3] =
		{
			{
				{ (float)container.left, (float)((container.top + container.bottom) >> 1) } ,
				noFont,
				solid,
				colour
			},
			{
				{ (float)container.right, (float)container.top },
				noFont,
				solid,
				colour
			},
			{
				{ (float)container.right, (float)container.bottom },
				noFont,
				solid,
				colour
			}
		};
		grc.AddTriangle(triangle);
	}

	ROCOCO_GRAPHICS_API void DrawTriangleFacingUp(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour)
	{
		BaseVertexData noFont{ { 0, 0 }, 0 };
		SpriteVertexData solid{ 1.0f, 0, 0, 0 };

		GuiVertex triangle[3] =
		{
			{
				{ (float)((container.left + container.right) >> 1), (float)container.top } ,
				noFont,
				solid,
				colour
			},
			{
				{ (float)container.right, (float)container.bottom },
				noFont,
				solid,
				colour
			},
			{
				{ (float)container.left, (float)container.bottom },
				noFont,
				solid,
				colour
			}
		};
		grc.AddTriangle(triangle);
	}

	ROCOCO_GRAPHICS_API void DrawTriangleFacingDown(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour)
	{
		BaseVertexData noFont{ { 0, 0 }, 0 };
		SpriteVertexData solid{ 1.0f, 0, 0, 0 };

		GuiVertex triangle[3] =
		{
			{
				{ (float)((container.left + container.right) >> 1), (float)container.bottom } ,
				noFont,
				solid,
				colour
			},
			{
				{ (float)container.left, (float)container.top },
				noFont,
				solid,
				colour
			},
			{
				{ (float)container.right, (float)container.top },
				noFont,
				solid,
				colour
			}
		};
		grc.AddTriangle(triangle);
	}

	ROCOCO_GRAPHICS_API void DrawTriangleFacingRight(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour)
	{
		BaseVertexData noFont{ { 0,0 }, 0 };
		SpriteVertexData solidColour{ 1.0f, 0.0f, 0.0f, 0.0f };

		GuiVertex triangle[3] =
		{
			{
				{ (float)container.left, (float)container.top },
				noFont,
				solidColour,
				colour
			},
			{
				{ (float)container.left, (float)container.bottom },
				noFont,
				solidColour,
				colour
			},
			{
				{ (float)container.right, (float)((container.top + container.bottom) >> 1) },
				noFont,
				solidColour,
				colour
			}
		};
		grc.AddTriangle(triangle);
	}

	ROCOCO_GRAPHICS_API Vec2i GetScreenCentre(const GuiMetrics& metrics)
	{
		return Vec2i{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };
	}

	ROCOCO_GRAPHICS_API Vec2i RenderVerticalCentredTextWithCallback(IGuiRenderContext& gr, int32 cursorPos, IEventCallback<GlyphCallbackArgs>& cb, cstr txt, RGBAb colour, int fontSize, const Vec2i& middleLeft, const GuiRect& clipRect)
	{
		struct : IEventCallback<GlyphCallbackArgs>
		{
			GuiRect rect{ 0,0,0,0 };
			GuiRect lastRect{ 0,0,0,0 };
			int targetPos;
			virtual void OnEvent(GlyphCallbackArgs& args)
			{
				if (args.index == targetPos)
				{
					rect = args.rect;
				}

				lastRect = args.rect;
			}
		} anon;
		anon.targetPos = cursorPos;

		HorizontalCentredText job(0, fontSize, txt, FontColourFromRGBAb(colour), &anon);
		Vec2i span = gr.EvalSpan(Vec2i{ 0,0 }, job);

		int dx = anon.rect.left - Width(clipRect);

		if (anon.rect.left < Width(clipRect))
		{
			dx = 0;
		}

		if (cursorPos > 0 && anon.rect.left == anon.rect.right)
		{
			// All the way to the right

			dx = span.x - Width(clipRect);
		}

		job.Reset(&cb);

		if (span.x <= Width(clipRect) || cursorPos == 0)
		{
			gr.RenderText(Vec2i{ middleLeft.x, middleLeft.y - (span.y >> 1) }, job, &clipRect);
		}
		else
		{
			GuiRect finalClipRect = clipRect;
			finalClipRect.left = max(middleLeft.x, clipRect.left);
			gr.RenderText(Vec2i{ middleLeft.x - dx, middleLeft.y - (span.y >> 1) }, job, &finalClipRect);
		}
		return span;
	}

	ROCOCO_GRAPHICS_API Vec2i RenderVerticalCentredText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& middleLeft, const GuiRect* clipRect)
	{
		HorizontalCentredText job(0, fontSize, text, FontColourFromRGBAb(colour));
		Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
		grc.RenderText(Vec2i{ middleLeft.x, middleLeft.y - (span.y >> 1) }, job, clipRect);
		return span;
	}

	ROCOCO_GRAPHICS_API Vec2i RenderTopLeftAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topLeft)
	{
		HorizontalCentredText job(0, fontSize, text, FontColourFromRGBAb(colour));
		Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
		grc.RenderText(Vec2i{ topLeft.x, topLeft.y }, job);
		return span;
	}

	ROCOCO_GRAPHICS_API Vec2i RenderTopRightAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topRight)
	{
		HorizontalCentredText job(0, fontSize, text, FontColourFromRGBAb(colour));
		Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
		grc.RenderText(Vec2i{ topRight.x - span.x, topRight.y }, job);
		return span;
	}

	ROCOCO_GRAPHICS_API Vec2i RenderRightAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const GuiRect& rect)
	{
		HorizontalCentredText job(0, fontSize, text, FontColourFromRGBAb(colour));
		Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
		Vec2i centre = Centre(rect);
		grc.RenderText(Vec2i{ rect.right - span.x - 2, centre.y - (span.y >> 1) }, job, &rect);
		return span;
	}

	ROCOCO_GRAPHICS_API Vec2i RenderHorizontalCentredText(IGuiRenderContext& grc, cstr txt, RGBAb colour, int fontSize, const Vec2i& topMiddle)
	{
		HorizontalCentredText job(0, fontSize, txt, FontColourFromRGBAb(colour));
		Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
		grc.RenderText(Vec2i{ topMiddle.x - (span.x >> 1), topMiddle.y }, job);
		return span;
	}

	ROCOCO_GRAPHICS_API void DrawLine(IGuiRenderContext& grc, int pixelthickness, Vec2i start, Vec2i end, RGBAb colour)
	{
		Vec2i delta = end - start;

		int dx2 = Sq(delta.x);
		int dy2 = Sq(delta.y);

		Vec2 offset;
		if (dy2 > dx2) // Move vertical than horizontal
		{
			offset = { (float)pixelthickness, 0 };
		}
		else
		{
			offset = { 0, (float)pixelthickness };
		}

		Vec2 bottomLeft = Vec2{ (float)start.x,  (float)start.y };
		Vec2 topLeft = bottomLeft + offset;

		Vec2 bottomRight = Vec2{ (float)end.x,  (float)end.y };
		Vec2 topRight = bottomRight + offset;

		BaseVertexData noFont{ {0,0}, 0 };
		SpriteVertexData solidColour{ 1.0f, 0.0f, 0.0f, 0.0f };

		GuiVertex q[] =
		{
			{ topLeft,      noFont, solidColour, colour },
			{ bottomLeft,   noFont, solidColour, colour },
			{ bottomRight,  noFont, solidColour, colour },
			{ bottomRight,  noFont, solidColour, colour },
			{ topRight,     noFont, solidColour, colour },
			{ topLeft,      noFont, solidColour, colour },
		};

		grc.AddTriangle(q);
		grc.AddTriangle(q + 3);
	}

	ROCOCO_GRAPHICS_API void DrawRectangle(IGuiRenderContext& grc, const GuiRect& grect, RGBAb diag, RGBAb backdiag)
	{
		GuiRectf rect{ (float)grect.left, (float)grect.top, (float)grect.right, (float)grect.bottom };
		BaseVertexData noFont{ {0, 0}, 0 };
		SpriteVertexData solid{ 1.0f, 0, 0, 0 };
		GuiVertex q[] =
		{
			{ {rect.left,  rect.top},    noFont, solid, diag },
			{ {rect.right, rect.top},    noFont, solid, backdiag },
			{ {rect.right, rect.bottom}, noFont, solid, diag },
			{ {rect.right, rect.bottom}, noFont, solid, diag },
			{ {rect.left,  rect.bottom}, noFont, solid, backdiag },
			{ {rect.left,  rect.top},    noFont, solid, diag }
		};

		if (diag.alpha > 0 && backdiag.alpha > 0)
		{
			grc.AddTriangle(q);
			grc.AddTriangle(q + 3);
		}
	}

	ROCOCO_GRAPHICS_API GuiRect RenderHQText(const GuiRect& clipRect, int32 alignment, IGuiRenderContext& grc, ID_FONT fontId, cstr text, RGBAb colour, Vec2i spacing, IEventCallback<GlyphContext>* glyphCallback, int dxShift)
	{
		if (text == nullptr)
		{
			text = "";
			colour = RGBAb(0, 0, 0, 0);
		}

		Vec2 origin = { (float)dxShift, 0 };
		struct : IHQTextJob
		{
			cstr text;
			RGBAb colour;
			Vec2 startPos;
			GuiRectf lastRect = { 0,0,0,0 };
			IEventCallback<GlyphContext>* glyphCallback = nullptr;

			void Render(IHQTextBuilder& builder) override
			{
				builder.SetColour(colour);
				builder.SetCursor(startPos);

				for (const char* p = text; *p != 0; p++)
				{
					builder.Write(*p, &lastRect);

					if (glyphCallback)
					{
						GlyphContext glyph{ Quantize(lastRect), (uint32)*p };
						glyphCallback->OnEvent(glyph);
					}
				}
			}
		} job;

		job.text = text;
		job.colour = colour;
		job.startPos = origin;
		job.glyphCallback = nullptr;

		grc.RenderHQText(fontId, job, IGuiRenderContext::EVALUATE_SPAN_ONLY, clipRect);

		Vec2 span;
		span.x = job.lastRect.right - origin.x;
		span.y = Height(job.lastRect);

		if (HasFlag(Alignment_Left, alignment) && !HasFlag(Alignment_Right, alignment))
		{
			job.startPos.x = (float)clipRect.left + spacing.x;
		}
		else if (HasFlag(Alignment_Right, alignment) && !HasFlag(Alignment_Left, alignment))
		{
			job.startPos.x = (float)(clipRect.right - span.x - spacing.x);
		}
		else
		{
			job.startPos.x = (float)((clipRect.right + clipRect.left) >> 1) - 0.5f * span.x;
		}

		if (HasFlag(Alignment_Top, alignment) && !HasFlag(Alignment_Bottom, alignment))
		{
			job.startPos.y = (float)clipRect.top + span.y + spacing.y;
		}
		else if (HasFlag(Alignment_Bottom, alignment) && !HasFlag(Alignment_Top, alignment))
		{
			job.startPos.y = (float)(clipRect.bottom - spacing.y);
		}
		else
		{
			job.startPos.y = (float)((clipRect.top + clipRect.bottom) >> 1) + 0.5f * span.y;
		}

		job.startPos.x = floorf(job.startPos.x) + dxShift;
		job.startPos.y = floorf(job.startPos.y);
		job.glyphCallback = glyphCallback;

		grc.RenderHQText(fontId, job, colour.alpha == 0 ? IGuiRenderContext::EVALUATE_SPAN_ONLY : IGuiRenderContext::RENDER, clipRect);

		return Quantize(GuiRectf{ job.startPos.x, job.startPos.y, job.startPos.x + span.x, job.startPos.y + span.y });
	}

	ROCOCO_GRAPHICS_API Vec2 RenderHQText_LeftAligned_VCentre(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour)
	{
		if (text == nullptr || *text == 0) return { 0,0 };

		Vec2 origin = { 0, 1000 };
		struct : IHQTextJob
		{
			cstr text;
			RGBAb colour;
			Vec2 startPos;
			GuiRectf lastRect = { 0,0,0,0 };

			void Render(IHQTextBuilder& builder) override
			{
				builder.SetColour(colour);
				builder.SetCursor(startPos);

				for (const char* p = text; *p != 0; p++)
				{
					builder.Write(*p, &lastRect);
				}
			}
		} job;

		job.text = text;
		job.colour = colour;
		job.startPos = origin;

		grc.RenderHQText(fontId, job, IGuiRenderContext::EVALUATE_SPAN_ONLY, rect);

		Vec2 span;
		span.x = job.lastRect.right - origin.x;
		span.y = Height(job.lastRect);

		job.startPos.x = (float)rect.left;
		job.startPos.y = 0.5f * ((float)rect.top + (float)rect.bottom + span.y);

		grc.RenderHQText(fontId, job, IGuiRenderContext::RENDER, rect);

		return span;
	}

	ROCOCO_GRAPHICS_API Vec2 RenderHQParagraph(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour)
	{
		if (text == nullptr || *text == 0) return { 0,0 };

		Vec2 origin = { 0, 1000 };
		struct : IHQTextJob
		{
			cstr text;
			RGBAb colour;
			Vec2 startPos;
			GuiRectf lastRect = { 0,0,0,0 };

			float32 rightBarrier_noMoreWords;
			float32 rightBarrier_noMoreChars;
			float32 lastRowHeight = 0;

			void Render(IHQTextBuilder& builder) override
			{
				builder.SetColour(colour);
				builder.SetCursor(startPos);

				for (const char* p = text; *p != 0; p++)
				{
					if (*p == '\n')
					{
						builder.SetCursor({ startPos.x, lastRect.bottom + lastRowHeight });
						continue;
					}

					builder.Write(*p, &lastRect);

					lastRowHeight = Height(lastRect);

					if (lastRect.right > rightBarrier_noMoreChars)
					{
						if (*p > ' ') // printable character
						{
							if (p[1] > ' ')
							{
								builder.Write('-', &lastRect);
							}
							builder.SetCursor({ startPos.x, lastRect.bottom + lastRowHeight });
							p++;
						}
						else
						{
							builder.SetCursor({ startPos.x, lastRect.bottom + lastRowHeight });
						}
					}
					else if (lastRect.right > rightBarrier_noMoreWords)
					{
						if (*p <= ' ')
						{
							builder.SetCursor({ startPos.x, lastRect.bottom + lastRowHeight });
						}
					}
				}
			}
		} job;

		job.rightBarrier_noMoreWords = rect.right - Width(rect) / 16.0f;
		job.rightBarrier_noMoreChars = rect.right - Width(rect) / 32.0f;

		job.text = text;
		job.colour = colour;
		job.startPos = origin;

		grc.RenderHQText(fontId, job, IGuiRenderContext::EVALUATE_SPAN_ONLY, rect);

		Vec2 span;
		span.x = job.lastRect.right - origin.x;
		span.y = Height(job.lastRect);

		job.startPos.x = (float)rect.left;
		job.startPos.y = (float)rect.top + job.lastRowHeight;

		grc.RenderHQText(fontId, job, IGuiRenderContext::RENDER, rect);

		return span;
	}

	ROCOCO_GRAPHICS_API Vec2 RenderHQText_LeftAligned_VCentre_WithCaret(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour, int caretPos)
	{
		if (text == nullptr || *text == 0) return { 0,0 };

		auto t = Time::TickCount();
		auto quarterSecond = Time::TickHz() >> 2;

		auto counter = t / quarterSecond;

		bool isCaretLit = (counter % 2) == 0;

		Vec2 origin = { 0, 1000 };
		struct : IHQTextJob
		{
			cstr text;
			RGBAb colour;
			Vec2 startPos;
			GuiRectf lastRect = { 0,0,0,0 };
			GuiRectf caretGlyphRect = { -1,-1,-1,-1 };
			int caretPos;

			void Render(IHQTextBuilder& builder) override
			{
				builder.SetColour(colour);
				builder.SetCursor(startPos);


				const char* caretPointer = text + caretPos;

				for (const char* p = text; *p != 0; p++)
				{
					builder.Write(*p, &lastRect);

					if (p == caretPointer)
					{
						caretGlyphRect = lastRect;
					}
				}
			}
		} job;

		job.text = text;
		job.colour = colour;
		job.startPos = origin;
		job.caretPos = caretPos;

		grc.RenderHQText(fontId, job, IGuiRenderContext::EVALUATE_SPAN_ONLY, rect);

		Vec2 span;
		span.x = job.lastRect.right - origin.x;
		span.y = Height(job.lastRect);

		job.startPos.x = (float)rect.left;
		job.startPos.y = 0.5f * ((float)rect.top + (float)rect.bottom + span.y);

		grc.RenderHQText(fontId, job, IGuiRenderContext::RENDER, rect);

		if (job.caretGlyphRect.left > -1)
		{
			Vec2i caretStart = { (int32)job.caretGlyphRect.left,  (int32)job.caretGlyphRect.bottom };
			Vec2i caretEnd = { (int32)job.caretGlyphRect.right, (int32)job.caretGlyphRect.bottom };
			Rococo::Graphics::DrawLine(grc, 2, caretStart, caretEnd, isCaretLit ? 0 : 0xFFFFFFFF);
		}
		else if (job.lastRect.right > -1)
		{
			auto ds = Span(rect);
			float defaultCaretSpan = 0.75f * ds.y;
			Vec2i caretStart = { (int32)job.lastRect.right,  (int32)rect.bottom };
			Vec2i caretEnd = { caretStart.x + (int32)defaultCaretSpan, caretStart.y };
			Rococo::Graphics::DrawLine(grc, 2, caretStart, caretEnd, isCaretLit ? 0 : 0xFFFFFFFF);
		}
		else
		{
			auto ds = Span(rect);
			float defaultCaretSpan = 0.75f * ds.y;
			Vec2i caretStart = { (int32)rect.left,  (int32)rect.bottom };
			Vec2i caretEnd = { caretStart.x + (int32)defaultCaretSpan, caretStart.y };
			Rococo::Graphics::DrawLine(grc, 2, caretStart, caretEnd, isCaretLit ? 0 : 0xFFFFFFFF);
		}

		return span;
	}

	ROCOCO_GRAPHICS_API void RenderCentred(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour)
	{
		Vec2 span = RenderHQText_LeftAligned_VCentre(grc, fontId, rect, text, RGBAb(0, 0, 0, 0));
		int32 left = ((rect.left + rect.right - (int32)(span.x)) >> 1);
		GuiRect finalRect = { left, rect.top, rect.right, rect.bottom };
		RenderHQText_LeftAligned_VCentre(grc, fontId, finalRect, text, colour);
	}

	ROCOCO_GRAPHICS_API void DrawBorderAround(IGuiRenderContext& grc, const GuiRect& rect, const Vec2i& width, RGBAb diag, RGBAb backdiag)
	{
		if (diag.alpha != 0)
		{
			GuiRect topRect{ rect.left, rect.top - width.y, rect.right, rect.top };
			DrawRectangle(grc, topRect, diag, diag);

			GuiRect rightRect{ rect.right, rect.top, rect.right + width.x, rect.bottom };
			DrawRectangle(grc, rightRect, diag, diag);
		}


		if (backdiag.alpha != 0)
		{
			GuiRect bottomRect{ rect.left, rect.bottom, rect.right, rect.bottom + width.y };
			DrawRectangle(grc, bottomRect, backdiag, backdiag);

			GuiRect leftRect{ rect.left - width.x, rect.top, rect.left, rect.bottom };
			DrawRectangle(grc, leftRect, backdiag, backdiag);
		}
	}

	ROCOCO_GRAPHICS_API Fonts::IDrawTextJob& CreateHorizontalCentredText(StackSpaceGraphics& ss, int fontIndex, cstr text, RGBAb colour)
	{
		static_assert(sizeof(ss) > sizeof(HorizontalCentredText), "Increase buffer size");
		HorizontalCentredText* hct = new (&ss) HorizontalCentredText(0, fontIndex, text, FontColourFromRGBAb(colour));
		return *hct;
	}

	ROCOCO_GRAPHICS_API Fonts::IDrawTextJob& CreateLeftAlignedText(StackSpaceGraphics& ss, const GuiRect& targetRect, int retzone, int hypzone, int fontHeight, int fontIndex, cstr text, RGBAb colour)
	{
		static_assert(sizeof(ss) > sizeof(LeftAlignedText), "Increase buffer size");
		LeftAlignedText* lat = new (&ss) LeftAlignedText(targetRect, retzone, hypzone, fontHeight, fontIndex, text, FontColourFromRGBAb(colour));
		return *lat;
	}

	ROCOCO_GRAPHICS_API float GetAspectRatio(const IRenderer& renderer)
	{
		GuiMetrics metrics;
		renderer.GetGuiMetrics(metrics);
		float aspectRatio = metrics.screenSpan.y / float(metrics.screenSpan.x);
		return aspectRatio;
	}
}

