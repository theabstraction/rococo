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
		std::unordered_map<ESchemeColourSurface, GRRenderStateMiniScheme> mapSurfaceToColour;

		void Free() override
		{
			delete this;
		}

		RGBAb GetColour(ESchemeColourSurface surface, GRRenderState rs) const override
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

		void SetColour(ESchemeColourSurface surface, RGBAb colour, GRRenderState rs) override
		{
			auto i = mapSurfaceToColour.find(surface);
			if (i != mapSurfaceToColour.end())
			{
				GRRenderStateMiniScheme::HoverAndFocus& hf = rs.value.bitValues.pressed ? i->second.pressed : i->second.notPressed;
				GRRenderStateMiniScheme::MiniFocusScheme f = rs.value.bitValues.hovered ? hf.hovered : hf.notHovered;
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

		bool TryGetColour(ESchemeColourSurface surface, RGBAb& colour, GRRenderState rs) const override
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
	IGRSchemeSupervisor* CreateScheme()
	{
		return new ANON::Scheme();
	}

	ROCOCO_GUI_RETAINED_API void SetSchemeColours_ThemeGrey(IGRScheme& scheme)
	{
		scheme.SetColour(ESchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(64, 64, 64, 192), GRGenerateIntensities());
		scheme.SetColour(ESchemeColourSurface::CONTAINER_TOP_LEFT, RGBAb(64, 64, 64, 192), GRGenerateIntensities());
		scheme.SetColour(ESchemeColourSurface::CONTAINER_BOTTOM_RIGHT, RGBAb(64, 64, 64, 192), GRGenerateIntensities());
		scheme.SetColour(ESchemeColourSurface::BACKGROUND, RGBAb(64, 64, 64, 192), GRGenerateIntensities());
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON, RGBAb(96, 96, 96, 255), GRGenerateIntensities());
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
		scheme.SetColour(ESchemeColourSurface::BUTTON, RGBAb(96, 96, 96, 255), GRGenerateIntensities());
		scheme.SetColour(ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
		scheme.SetColour(ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
		scheme.SetColour(ESchemeColourSurface::BUTTON_TEXT, RGBAb(255, 255, 255, 255), GRGenerateIntensities());
	}
}