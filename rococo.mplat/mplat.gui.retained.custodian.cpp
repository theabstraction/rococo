#include <rococo.gui.retained.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.maths.h>
#include <rococo.ui.h>
#include <rococo.textures.h>
#include <vector>

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

		if (flags.HasSomeFlags(GRAlignment::Top))
		{
			iAlignment |= Alignment_Bottom;
		}

		return iAlignment;
	}

	struct MPlatGR_Renderer : IGRRenderContext
	{
		IUtilities& utils;
		IGuiRenderContext* rc = nullptr;
		GuiRect lastScreenDimensions;
		Vec2i cursorPos{ -1000,-1000 };

		MPlatGR_Renderer(IUtilities& _utils) : utils(_utils)
		{

		}

		void DrawRect(const GuiRect& absRect, RGBAb colour) override
		{
			Rococo::Graphics::DrawRectangle(*rc, absRect, colour, colour);
		}

		void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			Rococo::Graphics::DrawBorderAround(*rc, absRect, Vec2i{ 1,1 }, colour1, colour2);
		}

		void DrawText(GRFontId fontId, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) override
		{
			int32 iAlignment = GRAlignment_To_RococoAlignment(alignment);

			ID_FONT hqFontId;

			switch (fontId)
			{
			case GRFontId::MENU_FONT:
			default:
				hqFontId = utils.GetHQFonts().GetSysFont(HQFont::MenuFont);
				break;
			}
	
			Rococo::Graphics::RenderHQText(clipRect, iAlignment, *rc, hqFontId, text, colour);
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

		void EnableScissors(const GuiRect& scissorRect) override
		{
			Throw(0, "Not implemented");
		}

		void DisableScissors() override
		{
			Throw(0, "Not implemented");
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

		EventRouting OnGREvent(WidgetEvent& widgetEvent) override
		{
			return EventRouting::Terminate;
		}

		void RecordWidget(IGRWidget& widget) override
		{
			history.push_back(&widget);
		}

		void RouteMouseEvent(const MouseEvent& me, IGuiRetained& gr) override
		{
			static_assert(sizeof CursorClick == sizeof uint16);

			history.clear();
			if (me.buttonFlags != 0)
			{
				CursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(CursorClick*)&me.buttonFlags };
				lastRoutingStatus = gr.RouteCursorClickEvent(cursorEvent);
			}
			else
			{
				CursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(CursorClick*)&me.buttonFlags };
				lastRoutingStatus = gr.RouteCursorMoveEvent(cursorEvent);
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

			renderer.SetContext(nullptr);
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