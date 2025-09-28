#include <rococo.gui.retained.h>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct GRWidgetRenderStateMiniScheme
	{
		struct MiniFocusScheme
		{
			RGBAb focusedColour;
			RGBAb notFocusedColour;
		};

		struct HoverAndFocus
		{
			MiniFocusScheme hovered;
			MiniFocusScheme notHovered;
		};

		HoverAndFocus pressed;
		HoverAndFocus notPressed;
	};

	struct Scheme : IGRSchemeSupervisor
	{
		std::unordered_map<EGRSchemeColourSurface, GRWidgetRenderStateMiniScheme> mapSurfaceToColour;

		void Free() override
		{
			delete this;
		}

		RGBAb GetColour(EGRSchemeColourSurface surface, GRWidgetRenderState rs) const override
		{
			auto i = mapSurfaceToColour.find(surface);
			if (i == mapSurfaceToColour.end())
			{
				return RGBAb(255, 255, 0, 64); // Semi transparent magenta is the colour that indicates undefined scheme entry
			}

			const GRWidgetRenderStateMiniScheme::HoverAndFocus& hf = rs.value.bitValues.pressed ? i->second.pressed : i->second.notPressed;
			const GRWidgetRenderStateMiniScheme::MiniFocusScheme f = rs.value.bitValues.hovered ? hf.hovered : hf.notHovered;
			return rs.value.bitValues.focused ? f.focusedColour : f.notFocusedColour;
		}

		void SetColour(EGRSchemeColourSurface surface, RGBAb colour, EGRColourSpec spec) override
		{
			switch(spec)
			{
			case EGRColourSpec::ForAllRenderStates:
				SetUniformColourForAllRenderStates(*this, surface, colour);
				break;
			case EGRColourSpec::ForAllPressedStates:
				SetColour(surface, colour, GRWidgetRenderState(true, false, false));
				SetColour(surface, colour, GRWidgetRenderState(true, false, true));
				SetColour(surface, colour, GRWidgetRenderState(true, true, false));
				SetColour(surface, colour, GRWidgetRenderState(true, true, true));
				break;
			case EGRColourSpec::ForAllFocusedStates:
				SetColour(surface, colour, GRWidgetRenderState(false, false, true));
				SetColour(surface, colour, GRWidgetRenderState(false, true, true));
				SetColour(surface, colour, GRWidgetRenderState(true, false, true));
				SetColour(surface, colour, GRWidgetRenderState(true, true, true));
				break;
			case EGRColourSpec::ForAllHoveredStates:
				SetColour(surface, colour, GRWidgetRenderState(false, true, false));
				SetColour(surface, colour, GRWidgetRenderState(false, true, true));
				SetColour(surface, colour, GRWidgetRenderState(true, true, false));
				SetColour(surface, colour, GRWidgetRenderState(true, true, true));
				break;
			}
		}

		void SetColour(EGRSchemeColourSurface surface, RGBAb colour, GRWidgetRenderState rs) override
		{
			auto i = mapSurfaceToColour.find(surface);
			if (i != mapSurfaceToColour.end())
			{
				GRWidgetRenderStateMiniScheme::HoverAndFocus& hf = rs.value.bitValues.pressed ? i->second.pressed : i->second.notPressed;
				GRWidgetRenderStateMiniScheme::MiniFocusScheme& f = rs.value.bitValues.hovered ? hf.hovered : hf.notHovered;
				RGBAb& target = rs.value.bitValues.focused ? f.focusedColour : f.notFocusedColour;
				target = colour;
			}
			else
			{						
				GRWidgetRenderStateMiniScheme newScheme;

				GRWidgetRenderStateMiniScheme::MiniFocusScheme mfs;
				mfs.focusedColour = colour;
				mfs.notFocusedColour = colour;

				GRWidgetRenderStateMiniScheme::HoverAndFocus hf;
				hf.hovered = mfs;
				hf.notHovered = mfs;

				newScheme.pressed = hf;
				newScheme.notPressed = hf;

				mapSurfaceToColour.emplace(surface, newScheme);
			}
		}

		bool TryGetColour(EGRSchemeColourSurface surface, RGBAb& colour, GRWidgetRenderState rs) const override
		{
			auto i = mapSurfaceToColour.find(surface);
			if (i == mapSurfaceToColour.end())
			{
				return false;
			}

			const GRWidgetRenderStateMiniScheme::HoverAndFocus& hf = rs.value.bitValues.pressed ? i->second.pressed : i->second.notPressed;
			const GRWidgetRenderStateMiniScheme::MiniFocusScheme f = rs.value.bitValues.hovered ? hf.hovered : hf.notHovered;
			colour = rs.value.bitValues.focused ? f.focusedColour : f.notFocusedColour;
			return true;
		}
	};
}

namespace Rococo::Gui
{
	IGRSchemeSupervisor* CreateGRScheme()
	{
		return new ANON::Scheme();
	}

	void SetAllHoverStates(IGRScheme& scheme, EGRSchemeColourSurface surface, RGBAb colour)
	{
		scheme.SetColour(surface, colour, EGRColourSpec::ForAllHoveredStates);
	}

	ROCOCO_GUI_RETAINED_API void SetSchemeColours_ThemeGrey(IGRScheme& scheme)
	{
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(64, 64, 64, 192));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::CONTAINER_TOP_LEFT, RGBAb(64, 64, 64, 192));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::BACKGROUND, RGBAb(64, 64, 64, 192));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::MENU_BUTTON, RGBAb(96, 96, 96, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(64, 64, 64, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(64, 64, 64, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::BUTTON, RGBAb(96, 96, 96, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT, RGBAb(64, 64, 64, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(64, 64, 64, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::BUTTON_SHADOW, RGBAb(0, 0, 0, 0));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::BUTTON_TEXT, RGBAb(255, 255, 255, 255));

		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::EDIT_TEXT, RGBAb(224, 224, 224, 255));

		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::TEXT, RGBAb(224, 224, 224, 255));
		scheme.SetColour(EGRSchemeColourSurface::TEXT, RGBAb(255, 255, 255, 255), EGRColourSpec::ForAllHoveredStates);

		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::EDIT_TEXT, RGBAb(224, 224, 224, 255));
		scheme.SetColour(EGRSchemeColourSurface::EDIT_TEXT, RGBAb(255, 255, 255, 255), EGRColourSpec::ForAllHoveredStates);

		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(0, 0, 0, 0));
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(0, 0, 0,   128), GRWidgetRenderState(0, 0, 0));
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(64, 64, 64, 64), GRWidgetRenderState(0, 0, 1));
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(0, 0, 0,    64), GRWidgetRenderState(0, 1, 0));
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(64, 64, 64, 32), GRWidgetRenderState(0, 1, 1));
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(64, 64, 64, 64), GRWidgetRenderState(0, 0, 1));

		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::SLIDER_BACKGROUND, RGBAb(64, 64, 64, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::GAME_OPTION_BACKGROUND, RGBAb(64, 64, 64, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::SLIDER_SLOT_BACKGROUND, RGBAb(128, 128, 128, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::SLIDER_SLOT_EDGE_1, RGBAb(255, 255, 255, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::SLIDER_SLOT_EDGE_2, RGBAb(192, 192, 192, 255));

		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::GAME_OPTION_TOP_LEFT, RGBAb(128, 128, 128, 255));
		SetAllHoverStates(scheme, EGRSchemeColourSurface::GAME_OPTION_TOP_LEFT, RGBAb(255, 255, 255, 255));

		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::GAME_OPTION_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255));
		SetAllHoverStates(scheme, EGRSchemeColourSurface::GAME_OPTION_BOTTOM_RIGHT, RGBAb(128, 128, 128, 255));

		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::GAME_OPTION_TEXT, RGBAb(192, 192, 192, 255));
		SetAllHoverStates(scheme, EGRSchemeColourSurface::GAME_OPTION_TEXT, RGBAb(255, 255, 255, 255));

		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::GAME_OPTION_DISABLED_TEXT, RGBAb(96, 96, 96, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::GAME_OPTION_DISABLED_BACKGROUND, RGBAb(48, 48, 48, 255));

		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::LABEL_SHADOW, RGBAb(0, 0, 0, 255));
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::FOCUS_RECTANGLE, RGBAb(255, 64, 64, 255));
	}

	ROCOCO_GUI_RETAINED_API void SetPropertyEditorColours_PastelScheme(IGRPanel& framePanel)
	{
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(160, 160, 160, 255), EGRColourSpec::ForAllRenderStates);
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(192, 192, 192, 255), GRWRS());
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllRenderStates);
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT, RGBAb(32, 32, 32, 255), EGRColourSpec::ForAllRenderStates);
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_BACKGROUND, RGBAb(225, 225, 225, 255), EGRColourSpec::ForAllRenderStates);
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_TOP_LEFT, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllRenderStates);
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT, RGBAb(32, 32, 32, 255), EGRColourSpec::ForAllRenderStates);
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_BACKGROUND, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllRenderStates);
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT, RGBAb(128, 128, 128, 255), EGRColourSpec::ForAllRenderStates);
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), EGRColourSpec::ForAllRenderStates);
		framePanel.Set(EGRSchemeColourSurface::SCROLLER_TRIANGLE_NORMAL, RGBAb(224, 224, 224, 255), EGRColourSpec::ForAllRenderStates);
		framePanel.Set(EGRSchemeColourSurface::READ_ONLY_TEXT, RGBAb(128, 128, 128, 255), EGRColourSpec::ForAllRenderStates);
		SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::ROW_COLOUR_EVEN, RGBAb(240, 240, 240));
		SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::ROW_COLOUR_ODD, RGBAb(255, 255, 255));
		SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::NAME_TEXT, RGBAb(0, 0, 0, 255));
		SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::LABEL_BACKGROUND, RGBAb(255, 255, 255, 0));
		MakeTransparent(framePanel, EGRSchemeColourSurface::LABEL_SHADOW);
		SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::EDITOR, RGBAb(192, 192, 192));
		SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::EDIT_TEXT, RGBAb(0, 0, 0));
		SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_EVEN, RGBAb(255, 240, 240));
		SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_ODD, RGBAb(240, 255, 240));
		framePanel.Set(EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_EVEN, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllFocusedStates);
		framePanel.Set(EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_ODD, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllFocusedStates);
		SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_TEXT, RGBAb(0, 0, 0, 255));
		MakeTransparent(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_SHADOW);
		SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::VALUE_TEXT, RGBAb(0, 0, 0, 255));
		MakeTransparent(framePanel, EGRSchemeColourSurface::BUTTON_IMAGE_FOG);
		framePanel.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 32), GRWidgetRenderState(0, 1, 0));
		framePanel.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 48), GRWidgetRenderState(0, 0, 1));
		framePanel.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 64), GRWidgetRenderState(0, 1, 1));
		framePanel.Set(EGRSchemeColourSurface::BACKGROUND, RGBAb(255, 0, 0, 0), EGRColourSpec::ForAllRenderStates);
	}

	ROCOCO_GUI_RETAINED_API void MakeTransparent(IGRPanel& panel, EGRSchemeColourSurface surface)
	{
		SetUniformColourForAllRenderStates(panel, surface, RGBAb(255, 255, 0, 0));
	}

	ROCOCO_GUI_RETAINED_API void SetUniformColourForAllRenderStates(IGRScheme& scheme, EGRSchemeColourSurface surface, RGBAb colour)
	{
		GRWidgetRenderState::ForEachPermutation([&scheme, surface, colour](GRWidgetRenderState rs)
			{
				scheme.SetColour(surface, colour, rs);
			}
		);
	}

	ROCOCO_GUI_RETAINED_API void SetUniformColourForAllRenderStates(IGRPanel& panel, EGRSchemeColourSurface surface, RGBAb colour)
	{
		GRWidgetRenderState::ForEachPermutation([&panel, surface, colour](GRWidgetRenderState rs)
			{
				panel.Set(surface, colour, rs);
			}
		);
	}
}