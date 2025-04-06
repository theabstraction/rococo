#include <rococo.gui.retained.h>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct GRRenderStateMiniScheme
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
		std::unordered_map<EGRSchemeColourSurface, GRRenderStateMiniScheme> mapSurfaceToColour;

		void Free() override
		{
			delete this;
		}

		RGBAb GetColour(EGRSchemeColourSurface surface, GRRenderState rs) const override
		{
			auto i = mapSurfaceToColour.find(surface);
			if (i == mapSurfaceToColour.end())
			{
				return RGBAb(255, 255, 0, 64); // Semi transparent magenta is the colour that indicates undefined scheme entry
			}

			const GRRenderStateMiniScheme::HoverAndFocus& hf = rs.value.bitValues.pressed ? i->second.pressed : i->second.notPressed;
			const GRRenderStateMiniScheme::MiniFocusScheme f = rs.value.bitValues.hovered ? hf.hovered : hf.notHovered;
			return rs.value.bitValues.focused ? f.focusedColour : f.notFocusedColour;
		}

		static uint8 Intensify(uint8 component, float componentScale)
		{
			float fNewComponent = clamp(componentScale * (float)component, 0.0f, 255.0f);
			return (uint8)fNewComponent;
		}

		static void Intensify(RGBAb& colour, float componentScale)
		{
			if (colour.alpha <= 128)
			{
				// We can just intensify the alpha for the desired highlight
				colour.alpha = Intensify(colour.alpha, componentScale);
			}
			else if (colour.red <= 128 && colour.green <= 128 && colour.blue <= 128)
			{
				// Intensify the colour components instead
				colour.red = Intensify(colour.red, componentScale);
				colour.green = Intensify(colour.green, componentScale);
				colour.blue = Intensify(colour.blue, componentScale);
			}
			else
			{
				// We don't know how to scale colours and alpha for the general case, it is up to the application developer to assign the intensified colours
			}
		}

		void SetColour(EGRSchemeColourSurface surface, RGBAb colour, GRRenderState rs) override
		{
			auto i = mapSurfaceToColour.find(surface);
			if (i != mapSurfaceToColour.end())
			{
				GRRenderStateMiniScheme::HoverAndFocus& hf = rs.value.bitValues.pressed ? i->second.pressed : i->second.notPressed;
				GRRenderStateMiniScheme::MiniFocusScheme& f = rs.value.bitValues.hovered ? hf.hovered : hf.notHovered;
				RGBAb& target = rs.value.bitValues.focused ? f.focusedColour : f.notFocusedColour;
				target = colour;
			}
			else
			{						
				GRRenderStateMiniScheme newScheme;

				GRRenderStateMiniScheme::MiniFocusScheme mfs;
				mfs.focusedColour = colour;
				mfs.notFocusedColour = colour;

				GRRenderStateMiniScheme::HoverAndFocus hf;
				hf.hovered = mfs;
				hf.notHovered = mfs;

				newScheme.pressed = hf;
				newScheme.notPressed = hf;

				if (rs.value.intValue == 0)
				{
					// By default if a new colour for render state 0 is defined we generate some default intensity for other render states

					float pressedScale = 1.25f;
					float focusedScale = 1.35f;
					float hoveredScale = 1.15f;

					Intensify(newScheme.pressed.hovered.focusedColour, pressedScale * focusedScale * hoveredScale);
					Intensify(newScheme.pressed.hovered.notFocusedColour , pressedScale * hoveredScale);
					Intensify(newScheme.pressed.notHovered.focusedColour, pressedScale * focusedScale);
					Intensify(newScheme.pressed.notHovered.notFocusedColour, pressedScale);
					Intensify(newScheme.notPressed.hovered.focusedColour, focusedScale * hoveredScale);
					Intensify(newScheme.notPressed.hovered.notFocusedColour, hoveredScale);
					Intensify(newScheme.notPressed.notHovered.focusedColour, focusedScale);
				}

				mapSurfaceToColour.emplace(surface, newScheme);
			}
		}

		bool TryGetColour(EGRSchemeColourSurface surface, RGBAb& colour, GRRenderState rs) const override
		{
			auto i = mapSurfaceToColour.find(surface);
			if (i == mapSurfaceToColour.end())
			{
				return false;
			}

			const GRRenderStateMiniScheme::HoverAndFocus& hf = rs.value.bitValues.pressed ? i->second.pressed : i->second.notPressed;
			const GRRenderStateMiniScheme::MiniFocusScheme f = rs.value.bitValues.hovered ? hf.hovered : hf.notHovered;
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

	ROCOCO_GUI_RETAINED_API void SetSchemeColours_ThemeGrey(IGRScheme& scheme)
	{
		SetUniformColourForAllRenderStates(scheme, EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(64, 64, 64, 192));
		scheme.SetColour(EGRSchemeColourSurface::CONTAINER_TOP_LEFT, RGBAb(64, 64, 64, 192), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT, RGBAb(64, 64, 64, 192), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::BACKGROUND, RGBAb(64, 64, 64, 192), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::MENU_BUTTON, RGBAb(96, 96, 96, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::BUTTON, RGBAb(96, 96, 96, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_TEXT, RGBAb(255, 255, 255, 255), GRGenerateIntensities());

		scheme.SetColour(EGRSchemeColourSurface::TEXT, RGBAb(224, 224, 224, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::EDIT_TEXT, RGBAb(224, 224, 224, 255), GRGenerateIntensities());

		scheme.SetColour(EGRSchemeColourSurface::TEXT, RGBAb(255, 255, 255, 255), GRRenderState(0, 1, 0));
		scheme.SetColour(EGRSchemeColourSurface::TEXT, RGBAb(255, 255, 255, 255), GRRenderState(0, 1, 1));

		scheme.SetColour(EGRSchemeColourSurface::EDIT_TEXT, RGBAb(224, 224, 224, 255), GRGenerateIntensities());

		scheme.SetColour(EGRSchemeColourSurface::EDIT_TEXT, RGBAb(255, 255, 255, 255), GRRenderState(0, 1, 0));
		scheme.SetColour(EGRSchemeColourSurface::EDIT_TEXT, RGBAb(255, 255, 255, 255), GRRenderState(0, 1, 1));

		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(0, 0, 0, 0), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(0, 0, 0,   128), GRRenderState(0, 0, 0));
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(64, 64, 64, 64), GRRenderState(0, 0, 1));
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(0, 0, 0,    64), GRRenderState(0, 1, 0));
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(64, 64, 64, 32), GRRenderState(0, 1, 1));
		scheme.SetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(64, 64, 64, 64), GRRenderState(0, 0, 1));

		scheme.SetColour(EGRSchemeColourSurface::SLIDER_BACKGROUND, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::GAME_OPTION_TEXT, RGBAb(255, 255, 255, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::GAME_OPTION_BACKGROUND, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::SLIDER_SLOT_BACKGROUND, RGBAb(128, 128, 128, 255), GRGenerateIntensities());

		scheme.SetColour(EGRSchemeColourSurface::GAME_OPTION_TOP_LEFT, RGBAb(192, 192, 192, 255), GRGenerateIntensities());
		scheme.SetColour(EGRSchemeColourSurface::GAME_OPTION_BOTTOM_RIGHT, RGBAb(128, 128, 128, 255), GRGenerateIntensities());
	}

	ROCOCO_GUI_RETAINED_API void MakeTransparent(IGRPanel& panel, EGRSchemeColourSurface surface)
	{
		SetUniformColourForAllRenderStates(panel, surface, RGBAb(255, 255, 0, 0));
	}

	template<class T> void ForEachRenderState(T t)
	{
		t(GRRenderState(0, 0, 0));
		t(GRRenderState(0, 0, 1));
		t(GRRenderState(0, 1, 0));
		t(GRRenderState(0, 1, 1));
		t(GRRenderState(1, 0, 0));
		t(GRRenderState(1, 0, 1));
		t(GRRenderState(1, 1, 0));
		t(GRRenderState(1, 1, 1));
	}

	ROCOCO_GUI_RETAINED_API void SetUniformColourForAllRenderStates(IGRScheme& scheme, EGRSchemeColourSurface surface, RGBAb colour)
	{
		ForEachRenderState([&scheme, surface, colour](GRRenderState rs)
			{
				scheme.SetColour(surface, colour, rs);
			}
		);
	}

	ROCOCO_GUI_RETAINED_API void SetUniformColourForAllRenderStates(IGRPanel& panel, EGRSchemeColourSurface surface, RGBAb colour)
	{
		ForEachRenderState([&panel, surface, colour](GRRenderState rs)
			{
				panel.Set(surface, colour, rs);
			}
		);
	}
}