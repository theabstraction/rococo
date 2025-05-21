#include "rococo.GR.UE5.h"
#include "RococoGuiAPI.h"
#include <rococo.gui.retained.ex.h>
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.maths.i32.h>
#include <rococo.hashtable.h>
#include <rococo.ui.h>
#include <rococo.vector.ex.h>
#include "SlateRenderContext.h"

namespace Rococo::OS
{
	ROCOCO_API Windows::IWindow& WindowOwner();
}

namespace Rococo::Gui
{
	ROCOCO_INTERFACE IUE5_GR_ImageSupervisor : IGRImageSupervisor
	{
	};

	ROCOCO_INTERFACE IUE5_GR_CustodianSupervisor
	{
		virtual IGRCustodian& Custodian() = 0;
		virtual void Render(IGRSystem& gr) = 0;
		virtual void RouteKeyboardEvent(const KeyboardEvent& key, IGRSystem& gr) = 0;
		virtual void RouteMouseEvent(const MouseEvent& me, IGRSystem& gr) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_GUI_RETAINED_API EGREventRouting TranslateToEditor(
		Windows::IWindow& ownerWindow,
		const GRKeyEvent& keyEvent,
		IGREditorMicromanager& manager,
		Strings::ICharBuilder& builder);
}

namespace Rococo::Gui::UE5::Implementation
{
	enum class ERenderTaskType
	{
		Edge
	};

	struct RenderTask
	{
		ERenderTaskType type;
		GuiRect target;
		RGBAb colour1;
		RGBAb colour2;
	};

	struct UE5_GR_Custodian;

	struct UE5_GR_Renderer : IGRRenderContext
	{
		GuiRect lastScreenDimensions;
		Vec2i cursorPos{ -1000,-1000 };
		std::vector<RenderTask> lastTasks;
		SlateRenderContext& rc;
		UE5_GR_Custodian& custodian;

		UE5_GR_Renderer(SlateRenderContext& _rc, UE5_GR_Custodian& _custodian) : rc(_rc), custodian(_custodian)
		{

		}

		virtual ~UE5_GR_Renderer()
		{

		}

		IGRFonts& Fonts() override;
		IGRImages& Images() override;

		void DrawLastItems()
		{
			for (auto& task : lastTasks)
			{
				switch (task.type)
				{
				case ERenderTaskType::Edge:
					DrawRectEdge(task.target, task.colour1, task.colour2, EGRRectStyle::SHARP, 4);
					break;
				}
			}

			lastTasks.clear();
		}

		void DrawDirectionArrow(const GuiRect& absRect, RGBAb colour, Degrees heading) override
		{
			bool needsClipping = lastScissorRect.IsNormalized() && Span(absRect) != Span(IntersectNormalizedRects(absRect, lastScissorRect));
		}

		void DrawImageStretched(IGRImage& image, const GuiRect& absRect) override
		{
		}

		void DrawImageUnstretched(IGRImage& image, const GuiRect& absRect, GRAlignmentFlags alignment)  override
		{
		}

		void DrawRect(const GuiRect& absRect, RGBAb colour, EGRRectStyle rectStyle, int cornerRadius) override
		{
			if (!lastScissorRect.IsNormalized())
			{
				return;
			}

			GuiRect visibleRect = IntersectNormalizedRects(absRect, lastScissorRect);

			/*
			rc->SetScissorRect(visibleRect);
			switch (rectStyle)
			{
			case EGRRectStyle::SHARP:
				Rococo::Graphics::DrawRectangle(*rc, absRect, colour, colour);
				break;
			case EGRRectStyle::ROUNDED_WITH_BLUR:
			case EGRRectStyle::ROUNDED:
				Rococo::Graphics::DrawRoundedRectangle(*rc, absRect, colour, cornerRadius);
			}
			*/
		}

		void DrawLine(Vec2i start, Vec2i end, RGBAb colour)
		{
		}

		void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2, EGRRectStyle rectStyle, int cornerRadius) override
		{
			UNUSED(rectStyle);
			UNUSED(cornerRadius);

			if (!lastScissorRect.IsNormalized())
			{
				return;
			}

			GuiRect visibleRect = IntersectNormalizedRects(absRect, lastScissorRect);

			/*
			rc->SetScissorRect(visibleRect);

			switch (rectStyle)
			{
			case EGRRectStyle::SHARP:
			{
				Rococo::Graphics::DrawBorderAround(*rc, absRect, Vec2i{ 1,1 }, colour1, colour2);
				break;
			}
			case EGRRectStyle::ROUNDED:
			case EGRRectStyle::ROUNDED_WITH_BLUR:
				Rococo::Graphics::DrawRoundedEdge(*rc, absRect, colour1, cornerRadius);
				break;
			}

			rc->ClearScissorRect();
			*/
		}

		void DrawRectEdgeLast(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			if (!lastScissorRect.IsNormalized())
			{
				return;
			}

			GuiRect visibleRect = IntersectNormalizedRects(absRect, lastScissorRect);
			RenderTask task{ ERenderTaskType::Edge, visibleRect, colour1, colour2 };
			lastTasks.push_back(task);
		}

		void DrawEditableText(GRFontId fontId, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, const CaretSpec& caret) override
		{
			if (!lastScissorRect.IsNormalized())
			{
				return;
			}


			/*
			rc->FlushLayer();
			rc->SetScissorRect(lastScissorRect);

			// If there is nothing to display, then render a space character, which will give the caret a rectangle to work with
			fstring editText = text.length > 0 ? text : " "_fstring;

			alignment.Remove(EGRAlignment::Right).Add(EGRAlignment::Left);
			int32 iAlignment = GRAlignment_To_RococoAlignment(alignment);

			auto& metrics = rc->Resources().HQFontsResources().GetFontMetrics(To_ID_FONT(fontId));

			struct : IEventCallback<GlyphContext>
			{
				int32 charPos = 0;
				int32 caretPos = 0;
				int32 caretWidth = 0;
				Vec2i caretStart = { 0,0 };
				Vec2i caretEnd = { 0,0 };
				Vec2i lastBottomRight = { 0,0 };

				void OnEvent(GlyphContext& glyph) override
				{
					if (charPos == caretPos)
					{
						caretStart = BottomLeft(glyph.outputRect);
						caretEnd = BottomRight(glyph.outputRect);
					}

					lastBottomRight = BottomRight(glyph.outputRect);

					charPos++;
				}
			} glyphCallback;

			glyphCallback.caretPos = caret.CaretPos;
			glyphCallback.caretWidth = metrics.imgWidth;

			RGBAb transparent(0, 0, 0, 0);

			Rococo::Graphics::RenderHQText(clipRect, iAlignment, *rc, To_ID_FONT(fontId), editText, transparent, spacing, &glyphCallback);

			int32 dxShift = 0;

			if (glyphCallback.caretEnd.x <= glyphCallback.caretStart.x)
			{
				glyphCallback.caretStart = glyphCallback.lastBottomRight;
				glyphCallback.caretEnd = glyphCallback.caretStart + Vec2i{ metrics.imgWidth, 0 };
			}

			if (glyphCallback.caretEnd.x >= clipRect.right)
			{
				dxShift = (clipRect.right - glyphCallback.caretEnd.x);
			}

			glyphCallback.charPos = 0;
			glyphCallback.caretStart = glyphCallback.caretEnd = { 0,0 };

			Rococo::Graphics::RenderHQText(clipRect, iAlignment, *rc, To_ID_FONT(fontId), editText, colour, spacing, &glyphCallback, dxShift);

			if (glyphCallback.caretEnd.x <= glyphCallback.caretStart.x)
			{
				glyphCallback.caretStart = glyphCallback.lastBottomRight;
				glyphCallback.caretEnd = glyphCallback.caretStart + Vec2i{ metrics.imgWidth, 0 };
			}

			auto ticks = Rococo::Time::TickCount();
			auto dticks = ticks % Rococo::Time::TickHz();

			float dt = dticks / (float)Rococo::Time::TickHz();

			RGBAb blinkColour = dt > 0.5f ? caret.CaretColour1 : caret.CaretColour2;
			Rococo::Graphics::DrawLine(*rc, 1, glyphCallback.caretStart, glyphCallback.caretEnd, blinkColour);

			rc->FlushLayer();
			rc->ClearScissorRect();

			*/
		}

		void DrawTriangles(const GRTriangle* triangles, size_t nTriangles) override
		{
			/*
			SpriteVertexData useColour{ 1, 0,0,0 };
			BaseVertexData noTextures{ {0,0}, 0 };

			for (size_t i = 0; i < nTriangles; i++)
			{
				auto& t = triangles[i];

				GuiVertex v[3];
				v[0].pos = Vec2{ (float)t.a.position.x, (float)t.a.position.y };
				v[1].pos = Vec2{ (float)t.b.position.x, (float)t.b.position.y };
				v[2].pos = Vec2{ (float)t.c.position.x, (float)t.c.position.y };

				v[0].colour = t.a.colour;
				v[1].colour = t.b.colour;
				v[2].colour = t.c.colour;

				v[0].sd = useColour;
				v[1].sd = useColour;
				v[2].sd = useColour;

				v[0].vd = noTextures;
				v[1].vd = noTextures;
				v[2].vd = noTextures;

				rc->AddTriangle(v);
			}

			*/
		}

		void DrawParagraph(GRFontId fontId, const GuiRect& targetRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) override
		{
			DrawText(fontId, targetRect, alignment, spacing, text, colour);
		}

		void DrawText(GRFontId fontId, const GuiRect& targetRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) override
		{
			if (!lastScissorRect.IsNormalized())
			{
				return;
			}

			if (!AreRectsOverlapped(lastScissorRect, targetRect))
			{
				return;
			}

			/*

			rc->FlushLayer();
			rc->SetScissorRect(lastScissorRect);

			int32 iAlignment = GRAlignment_To_RococoAlignment(alignment);

			if (alignment.HasSomeFlags(EGRAlignment::AutoFonts))
			{
				ID_FONT autoFontId = To_ID_FONT(fontId);

				for (;;)
				{
					Vec2i pixelSpan = rc->Resources().HQFontsResources().EvalSpan(To_ID_FONT(fontId), text);;

					if (pixelSpan.x == 0 || pixelSpan.y == 0)
					{
						break;
					}

					if (Width(targetRect) > pixelSpan.x)
					{
						Rococo::Graphics::RenderHQText(targetRect, iAlignment, *rc, To_ID_FONT(fontId), text, colour, spacing);
						break;
					}
					else
					{
						ID_FONT smallerFont = rc->Resources().HQFontsResources().FindBestSmallerFont(autoFontId);
						if (!smallerFont)
						{
							ID_FONT smallestFont = rc->Resources().HQFontsResources().FindSmallestFont();
							Rococo::Graphics::RenderHQText(targetRect, iAlignment, *rc, smallestFont, text, colour, spacing);
							break;
						}

						autoFontId = smallerFont;
					}
				}
			}
			else
			{
				Rococo::Graphics::RenderHQText(targetRect, iAlignment, *rc, To_ID_FONT(fontId), text, colour, spacing);
			}

			rc->FlushLayer();
			rc->ClearScissorRect();

			*/
		}

		/*
		bool TryFindFontJustSmallerThanHeight(Rococo::Gui::FontSpec&, int) const
		{
			return false;
		}
		*/

		void Flush() override
		{
			//	rc->FlushLayer();
		}

		Vec2i CursorHoverPoint() const override
		{
			return cursorPos;
		}

		bool IsHovered(IGRPanel& panel) const override
		{
			return IsPointInRect(cursorPos, panel.AbsRect());
		}

		GuiRect ScreenDimensions() const override
		{
			return lastScreenDimensions;
		}

		GuiRect lastScissorRect;

		void EnableScissors(const GuiRect& scissorRect) override
		{
			lastScissorRect = scissorRect;
		}

		void DisableScissors() override
		{
			lastScissorRect = GuiRect{ 0,0,0,0 };
		}

		bool TryGetScissorRect(GuiRect& scissorRect) const override
		{
			scissorRect = lastScissorRect;
			return lastScissorRect.IsNormalized();
		}

		/*
		bool Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, bool isStretched, const BitmapLocation& sprite)
		{
			UNUSED(isStretched);

			if (!rc || sprite.pixelSpan.x <= 0 || sprite.pixelSpan.y <= 0 || sprite.textureIndex < 0)
			{
				return false;
			}

			bool isLeftAligned = alignment.HasSomeFlags(EGRAlignment::Left) && !alignment.HasSomeFlags(EGRAlignment::Right);
			bool isRightAligned = !alignment.HasSomeFlags(EGRAlignment::Left) && alignment.HasSomeFlags(EGRAlignment::Right);

			int32 x = 0;

			GuiRect rect = panel.AbsRect();

			Vec2i ds = Quantize(sprite.pixelSpan);

			if (isLeftAligned)
			{
				x = rect.left + spacing.x;
			}
			else if (isRightAligned)
			{
				x = rect.right - spacing.x - ds.x;
			}
			else
			{
				x = (rect.left + rect.right - ds.x) >> 1;
			}

			int y = 0;
			bool isTopAligned = alignment.HasSomeFlags(EGRAlignment::Top) && !alignment.HasSomeFlags(EGRAlignment::Bottom);
			bool isBottomAligned = !alignment.HasSomeFlags(EGRAlignment::Top) && alignment.HasSomeFlags(EGRAlignment::Bottom);

			if (isTopAligned)
			{
				y = rect.top + spacing.y;
			}
			else if (isBottomAligned)
			{
				y = rect.bottom - spacing.x - ds.y;
			}
			else
			{
				y = (rect.top + rect.bottom - ds.y) >> 1;
			}

			Graphics::DrawSprite({ x, y }, sprite, *rc);

			return true;
		}
	*/
	};


	struct UE5_GR_Image : IUE5_GR_ImageSupervisor
	{
		Vec2i span{ 8, 8 };

		UE5_GR_Image(cstr hint, cstr imagePath)
		{

		}

		bool Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, bool isStretched, IGRRenderContext& g) override
		{
			return false;
			// return static_cast<UE5_GR_Renderer&>(g).Render(panel, alignment, spacing, isStretched, sprite);
		}

		void Free() override
		{
			delete this;
		}

		Vec2i Span() const override
		{
			return { 32, 32 };
			// return Quantize(sprite.pixelSpan);
		}

		/*
		const BitmapLocation& Sprite() const override
		{
			return sprite;
		}
		*/
	};

	const stringmap<cstr> macroToPingPath =
	{
		{ "$(COLLAPSER_EXPAND)", "!textures/toolbars/MAT/expanded.tif" },
		{ "$(COLLAPSER_COLLAPSE)", "!textures/toolbars/MAT/collapsed.tif" },
		{ "$(COLLAPSER_ELEMENT_EXPAND)", "!textures/toolbars/MAT/expanded.tif" },
		{ "$(COLLAPSER_ELEMENT_INLINE)",  "!textures/toolbars/MAT/collapsed.tif" },
	};

	struct UE5_GR_Custodian : IUE5_GRCustodianSupervisor, IGREventHistory, IGRFonts, IGRImages
	{
		// Debugging materials:
		std::vector<IGRWidget*> history;
		EGREventRouting lastRoutingStatus = EGREventRouting::Terminate;
		int64 eventCount = 0;

		UE5_GR_Custodian()
		{

		}

		virtual ~UE5_GR_Custodian()
		{

		}

		Windows::IWindow& Owner()
		{
			return Rococo::OS::WindowOwner();
		}

		float zoomLevel = 1.0f;

		void SetUIZoom(float _zoomLevel) override
		{
			float newZoomLevel = clamp(1.0f, _zoomLevel, 100.0f);
			if (newZoomLevel != this->zoomLevel)
			{
				this->zoomLevel = newZoomLevel;
				//renderer.utils.GetHQFonts().SetZoomLevel(newZoomLevel);
			}
		}

		float ZoomLevel() const override
		{
			return this->zoomLevel;
		}

		void AlertNoActionForKey() override
		{

		}

		IGRFonts& Fonts() override
		{
			return *this;
		}

		cstr GetLastKnownControlType() const override
		{
			return "XBOX";
		}

		GRFontId BindFontId(const FontSpec& spec) override
		{
			return GRFontId::NONE;
		}

		int GetFontHeight(GRFontId id) const override
		{
			return 11;
		}

		IGRImageSupervisor* CreateImageFromPath(cstr debugHint, cstr codedImagePath) override
		{
			return nullptr;
		}

		Vec2i EvaluateMinimalSpan(GRFontId fontId, const fstring& text) const override
		{
			return { 32,32 };
		}

		void RecordWidget(IGRWidget& widget) override
		{
			history.push_back(&widget);
		}

		void RouteKeyboardEvent(const KeyboardEvent& key, IGRSystem& gr) override
		{
			GRKeyEvent keyEvent{ *this, eventCount, key };
			lastRoutingStatus = gr.RouteKeyEvent(keyEvent);
			if (lastRoutingStatus == EGREventRouting::NextHandler)
			{
				gr.ApplyKeyGlobally(keyEvent);
			}
		}

		EGRCursorIcon currentIcon = EGRCursorIcon::Arrow;

		void RouteMouseEvent(const MouseEvent& me, IGRSystem& gr) override
		{
			static_assert(sizeof GRCursorClick == sizeof uint16);

			history.clear();
			if (me.buttonFlags != 0)
			{
				GRCursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(GRCursorClick*)&me.buttonFlags, EGRCursorIcon::Unspecified, (int)(int16)me.buttonData };
				lastRoutingStatus = gr.RouteCursorClickEvent(cursorEvent);
			}
			else
			{
				GRCursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(GRCursorClick*)&me.buttonFlags, EGRCursorIcon::Arrow, 0 };
				lastRoutingStatus = gr.RouteCursorMoveEvent(cursorEvent);

				if (currentIcon != cursorEvent.nextIcon)
				{
					currentIcon = cursorEvent.nextIcon;

					switch (currentIcon)
					{
					case EGRCursorIcon::Arrow:
						//sysRenderer.GuiResources().SetSysCursor(EWindowCursor_Default);
						break;
					case EGRCursorIcon::LeftAndRightDragger:
						//sysRenderer.GuiResources().SetSysCursor(EWindowCursor_HDrag);
						break;
					}
				}
			}
			eventCount++;
		}

		void Free() override
		{
			delete this;
		}

		void RaiseError(const Sex::ISExpression* associatedSExpression, EGRErrorCode, cstr function, cstr format, ...) override
		{
			char message[1024];
			va_list args;
			va_start(args, format);
			Strings::SafeVFormat(message, sizeof message, format, args);
			va_end(args);

			if (associatedSExpression)
			{
				Rococo::Sex::Throw(*associatedSExpression, "%s: %s", function, message);
			}
			else
			{
				Throw(0, "%s: %s", function, message);
			}
		}

		void Render(SlateRenderContext& rc, IGRSystem& gr) override
		{
			UE5_GR_Renderer renderer(rc, *this);
			gr.RenderAllFrames(renderer);
			renderer.DrawLastItems();
		}

		std::vector<char> copyAndPasteBuffer;

		EGREventRouting TranslateToEditor(const GRKeyEvent& keyEvent, IGREditorMicromanager& manager) override
		{
			Strings::CharBuilder builder(copyAndPasteBuffer);
			return Gui::TranslateToEditor(Rococo::OS::WindowOwner(), keyEvent, manager, builder);
		}
	};

	IGRFonts& UE5_GR_Renderer::Fonts()
	{
		return custodian;
	}

	IGRImages& UE5_GR_Renderer::Images()
	{
		return custodian;
	}
}

namespace Rococo::Gui
{
	DLLEXPORT IUE5_GRCustodianSupervisor* Create_UE5_GRCustodian()
	{
		return new Rococo::Gui::UE5::Implementation::UE5_GR_Custodian();
	}
}
