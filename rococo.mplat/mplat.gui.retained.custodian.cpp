#include <rococo.gui.retained.ex.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.maths.h>
#include <rococo.maths.i32.h>
#include <rococo.ui.h>
#include <rococo.fonts.hq.h>
#include <rococo.textures.h>
#include <rococo.os.h>
#include <vector>
#include <rococo.vkeys.win32.h>
#include <rococo.os.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Graphics;
using namespace Rococo::Textures;

namespace ANON
{
	int32 GRAlignment_To_RococoAlignment(GRAlignmentFlags flags)
	{
		int32 iAlignment = 0;
		if (flags.HasSomeFlags(GRAlignment::Left))
		{
			iAlignment |= Alignment_Left;
		}

		if (flags.HasSomeFlags(GRAlignment::Right))
		{
			iAlignment |= Alignment_Right;
		}

		if (flags.HasSomeFlags(GRAlignment::Top))
		{
			iAlignment |= Alignment_Top;
		}

		if (flags.HasSomeFlags(GRAlignment::Bottom))
		{
			iAlignment |= Alignment_Bottom;
		}

		return iAlignment;
	}

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

	struct MPlatGR_Renderer : IGRRenderContext
	{
		IUtilities& utils;
		IGuiRenderContext* rc = nullptr;
		GuiRect lastScreenDimensions;
		Vec2i cursorPos{ -1000,-1000 };

		std::vector<RenderTask> lastTasks;

		MPlatGR_Renderer(IUtilities& _utils) : utils(_utils)
		{

		}

		void DrawLastItems()
		{
			for (auto& task : lastTasks)
			{
				switch (task.type)
				{
				case ERenderTaskType::Edge:
					DrawRectEdge(task.target, task.colour1, task.colour2);
					break;
				}
			}

			lastTasks.clear();
		}

		void DrawDirectionArrow(const GuiRect& absRect, RGBAb colour, Degrees heading) override
		{
			bool needsClipping = lastScissorRect.IsNormalized() && Span(absRect) != Span(IntersectNormalizedRects(absRect, lastScissorRect));
				
			if (needsClipping)
			{
				rc->FlushLayer();
				rc->SetScissorRect(lastScissorRect);
			}

			if (heading.degrees == 0)
			{
				Rococo::DrawTriangleFacingUp(*rc, absRect, colour);
			}
			else
			{
				Rococo::DrawTriangleFacingDown(*rc, absRect, colour);
			}

			if (needsClipping)
			{
				rc->FlushLayer();
				rc->ClearScissorRect();
			}
		}

		void DrawRect(const GuiRect& absRect, RGBAb colour) override
		{
			GuiRect visibleRect = lastScissorRect.IsNormalized() ? IntersectNormalizedRects(absRect, lastScissorRect) : absRect;
			Rococo::Graphics::DrawRectangle(*rc, visibleRect, colour, colour);
		}

		void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			GuiRect visibleRect = lastScissorRect.IsNormalized() ? IntersectNormalizedRects(absRect, lastScissorRect) : absRect;
			Rococo::Graphics::DrawBorderAround(*rc, visibleRect, Vec2i{ 1,1 }, colour1, colour2);
		}

		void DrawRectEdgeLast(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			GuiRect visibleRect = lastScissorRect.IsNormalized() ? IntersectNormalizedRects(absRect, lastScissorRect) : absRect;
			RenderTask task{ ERenderTaskType::Edge, visibleRect, colour1, colour2 };
			lastTasks.push_back(task);
		}

		void DrawEditableText(GRFontId fontId, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, int32 caretPos, RGBAb colour) override
		{
			if (lastScissorRect.IsNormalized())
			{
				rc->FlushLayer();
				rc->SetScissorRect(lastScissorRect);
			}

			// If there is nothing to display, then render a space character, which will give the caret a rectangle to work with
			fstring editText = text.length > 0 ? text : " "_fstring;

			alignment.Remove(GRAlignment::Right).Add(GRAlignment::Left);
			int32 iAlignment = GRAlignment_To_RococoAlignment(alignment);

			ID_FONT hqFontId;

			switch (fontId)
			{
			case GRFontId::MENU_FONT:
			default:
				hqFontId = utils.GetHQFonts().GetSysFont(HQFont::MenuFont);
				break;
			}

			auto& metrics = rc->Gui().HQFontsResources().GetFontMetrics(hqFontId);
			
			struct : IEventCallback<GlyphContext>
			{
				int32 charPos = 0;
				int32 caretPos = 0;
				int32 caretWidth = 0;
				Vec2i caretStart = {0,0};
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

			glyphCallback.caretPos = caretPos;
			glyphCallback.caretWidth = metrics.imgWidth;

			RGBAb transparent(0, 0, 0, 0);

			Rococo::Graphics::RenderHQText(clipRect, iAlignment, *rc, hqFontId, editText, transparent, spacing, &glyphCallback);

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

			Rococo::Graphics::RenderHQText(clipRect, iAlignment, *rc, hqFontId, editText, colour, spacing, &glyphCallback, dxShift);

			if (glyphCallback.caretEnd.x <= glyphCallback.caretStart.x)
			{
				glyphCallback.caretStart = glyphCallback.lastBottomRight;
				glyphCallback.caretEnd = glyphCallback.caretStart + Vec2i{ metrics.imgWidth, 0 };
			}

			auto ticks = Rococo::OS::CpuTicks();
			auto dticks = ticks % Rococo::OS::CpuHz();

			float dt = dticks / (float)Rococo::OS::CpuHz();

			RGBAb blinkColour = colour;
			if (dt > 0.5f)
			{
				blinkColour.alpha = colour.alpha / 2;
			}
			Rococo::Graphics::DrawLine(*rc, 1, glyphCallback.caretStart, glyphCallback.caretEnd, blinkColour);

			if (lastScissorRect.IsNormalized())
			{
				rc->FlushLayer();
				rc->ClearScissorRect();
			}
		}

		void DrawText(GRFontId fontId, const GuiRect& targetRect, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) override
		{
			if (lastScissorRect.IsNormalized() && IsRectClipped(lastScissorRect, clipRect))
			{
				if (!AreRectsOverlapped(lastScissorRect, clipRect))
				{
					return;
				}

				rc->FlushLayer();
				rc->SetScissorRect(lastScissorRect);
			}

			int32 iAlignment = GRAlignment_To_RococoAlignment(alignment);

			ID_FONT hqFontId;

			switch (fontId)
			{
			case GRFontId::MENU_FONT:
			default:
				hqFontId = utils.GetHQFonts().GetSysFont(HQFont::MenuFont);
				break;
			}
	
			Rococo::Graphics::RenderHQText(clipRect, iAlignment, *rc, hqFontId, text, colour, spacing);

			if (lastScissorRect.IsNormalized() && IsRectClipped(lastScissorRect, clipRect))
			{
				rc->FlushLayer();
				rc->ClearScissorRect();
			}
		}

		void Flush() override
		{
			rc->FlushLayer();
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

		void SetContext(IGuiRenderContext* rc)
		{
			this->rc = rc;
			if (rc)
			{
				GuiMetrics metrics;
				rc->Renderer().GetGuiMetrics(metrics);
				cursorPos = metrics.cursorPosition;
				lastScreenDimensions = { 0, 0, metrics.screenSpan.x, metrics.screenSpan.y };
			}
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

		bool Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, const BitmapLocation& sprite)
		{
			if (!rc || sprite.pixelSpan.x <= 0 || sprite.pixelSpan.y <= 0 || sprite.textureIndex < 0) 
			{
				return false;
			}

			GuiRect rect = panel.AbsRect();
			Graphics::DrawSprite(TopLeft(rect) + Vec2i { 1,1,}, sprite, *rc);

			return true;
		}
	};

	struct MPlatImageMemento : IImageMemento
	{
		Vec2i span{ 8, 8 };
		BitmapLocation sprite = BitmapLocation::None();

		ITextureArrayBuilder& sprites;

		MPlatImageMemento(cstr imagePath, ITextureArrayBuilder& _sprites): sprites(_sprites)
		{
			if (!sprites.TryGetBitmapLocation(imagePath, sprite))
			{
				Throw(0, "Could not find bitmap: %s", imagePath);
			}
		}

		bool Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, IGRRenderContext& g) override
		{
			return static_cast<MPlatGR_Renderer&>(g).Render(panel, alignment, spacing, sprite);
		}

		void Free() override
		{
			delete this;
		}

		Vec2i Span() const override
		{
			return Quantize(sprite.pixelSpan);
		}
	};

	struct MPlatCustodian : IMPlatGuiCustodianSupervisor, IGRCustodian, IGREventHistory
	{
		MPlatGR_Renderer renderer;
		IRenderer& sysRenderer;

		// Debugging materials:
		std::vector<IGRWidget*> history;
		EventRouting lastRoutingStatus = EventRouting::Terminate;
		int64 eventCount = 0;

		MPlatCustodian(IUtilities& utils, IRenderer& _sysRenderer): renderer(utils), sysRenderer(_sysRenderer)
		{
			
		}

		IImageMemento* CreateImageMemento(cstr codedImagePath) override
		{
			cstr imagePath;
			if (strstr(codedImagePath, "$(COLLAPSER_COLLAPSE)"))
			{
				imagePath = "!textures/toolbars/3rd-party/www.aha-soft.com/Forward.tiff";
			}
			else if (strstr(codedImagePath, "$(COLLAPSER_EXPAND)"))
			{
				imagePath = "!textures/toolbars/3rd-party/www.aha-soft.com/Down.tiff";
			}
			else
			{
				imagePath = codedImagePath;
			}

			return new MPlatImageMemento(imagePath, sysRenderer.Gui().SpriteBuilder());
		}

		Vec2i EvaluateMinimalSpan(GRFontId fontId, const fstring& text) const override
		{
			ID_FONT idSysFont;

			switch (fontId)
			{
			case GRFontId::MENU_FONT:
			default:
				idSysFont = renderer.utils.GetHQFonts().GetSysFont(HQFont::MenuFont);
				break;
			}

			return sysRenderer.Gui().HQFontsResources().EvalSpan(idSysFont, text);
		}

		void RecordWidget(IGRWidget& widget) override
		{
			history.push_back(&widget);
		}

		void RouteKeyboardEvent(const KeyboardEvent& key, IGuiRetained& gr) override
		{
			KeyEvent keyEvent{ *this, eventCount, key };
			lastRoutingStatus = gr.RouteKeyEvent(keyEvent);
		}

		ECursorIcon currentIcon = ECursorIcon::Arrow;

		void RouteMouseEvent(const MouseEvent& me, IGuiRetained& gr) override
		{
			static_assert(sizeof CursorClick == sizeof uint16);

			history.clear();
			if (me.buttonFlags != 0)
			{
				CursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(CursorClick*)&me.buttonFlags, ECursorIcon::Unspecified };
				lastRoutingStatus = gr.RouteCursorClickEvent(cursorEvent);
			}
			else
			{
				CursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(CursorClick*)&me.buttonFlags, ECursorIcon::Arrow };
				lastRoutingStatus = gr.RouteCursorMoveEvent(cursorEvent);

				if (currentIcon != cursorEvent.nextIcon)
				{
					currentIcon = cursorEvent.nextIcon;

					switch (currentIcon)
					{
					case ECursorIcon::Arrow:
						sysRenderer.Gui().SetSysCursor(EWindowCursor_Default);
						break;
					case ECursorIcon::LeftAndRightDragger:
						sysRenderer.Gui().SetSysCursor(EWindowCursor_HDrag);
						break;
					}
				}
			}
			eventCount++;
		}

		IGRCustodian& Custodian() override
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}

		void RaiseError(GRErrorCode code, cstr function, cstr message)
		{
			Throw(0, "%s: %s", function, message);
		}

		void Render(IGuiRenderContext& rc, IGuiRetained& gr) override
		{
			renderer.SetContext(&rc);
			
			if (renderer.lastScreenDimensions.right > 0 && renderer.lastScreenDimensions.bottom > 0)
			{
				gr.RenderGui(renderer);
			}

			renderer.DrawLastItems();

			renderer.SetContext(nullptr);
		}

		std::vector<char> copyAndPasteBuffer;

		void TranslateToEditor(const KeyEvent& keyEvent, IGREditorMicromanager& manager) override
		{
			if (!keyEvent.osKeyEvent.IsUp())
			{
				switch (keyEvent.osKeyEvent.VKey)
				{
				case IO::VKCode_BACKSPACE:
					manager.BackspaceAtCaret();
					return;
				case IO::VKCode_DELETE:
					manager.DeleteAtCaret();
					return;
				case IO::VKCode_ENTER:
					manager.Return();
					return;
				case IO::VKCode_LEFT:
					manager.AddToCaretPos(-1);
					return;
				case IO::VKCode_RIGHT:
					manager.AddToCaretPos(1);
					return;
				case IO::VKCode_HOME:
					manager.AddToCaretPos(-100'000'000);
					return;
				case IO::VKCode_END:
					manager.AddToCaretPos(100'000'000);
					return;
				case IO::VKCode_C:
					if (IO::IsKeyPressed(IO::VKCode_CTRL))
					{
						// Note that GetTextAndLength is guaranteed to be at least one character, and if so, the one character is the nul terminating the string
						copyAndPasteBuffer.resize(manager.GetTextAndLength(nullptr, 0));
						manager.GetTextAndLength(copyAndPasteBuffer.data(), (int32) copyAndPasteBuffer.size());
						Rococo::OS::CopyStringToClipboard(copyAndPasteBuffer.data());
						copyAndPasteBuffer.clear();
						return;
					}
					else
					{
						break;
					}
				case IO::VKCode_V:
					if (IO::IsKeyPressed(IO::VKCode_CTRL))
					{
						manager.GetTextAndLength(copyAndPasteBuffer.data(), (int32)copyAndPasteBuffer.size());

						struct : IEventCallback<cstr>
						{
							IGREditorMicromanager* manager = nullptr;
							void OnEvent(cstr text) override
							{
								for (cstr p = text; *p != 0; p++)
								{
									manager->AppendCharAtCaret(*p);
								}
							}
						} cb;
						cb.manager = &manager;
						Rococo::OS::PasteStringFromClipboard(cb);
						return;
					}
					else
					{
						break;
					}
				}

				if (keyEvent.osKeyEvent.unicode >= 32 && keyEvent.osKeyEvent.unicode <= 127)
				{
					manager.AppendCharAtCaret((char)keyEvent.osKeyEvent.unicode);
				}
			}
		}
	};
}

namespace Rococo::Gui
{
	IMPlatGuiCustodianSupervisor* CreateMPlatCustodian(IUtilities& utils, IRenderer& sysRenderer)
	{
		return new ANON::MPlatCustodian(utils, sysRenderer);
	}
}