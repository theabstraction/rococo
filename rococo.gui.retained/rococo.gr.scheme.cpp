#include <rococo.gui.retained.h>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct Scheme : ISchemeSupervisor
	{
		std::unordered_map<ESchemeColourSurface, RGBAb> mapSurfaceToColour;

		void Free() override
		{
			delete this;
		}

		RGBAb GetColour(ESchemeColourSurface surface) const override
		{
			auto i = mapSurfaceToColour.find(surface);
			return i != mapSurfaceToColour.end() ? i->second : RGBAb(0, 0, 0, 0);
		}

		void SetColour(ESchemeColourSurface surface, RGBAb colour) override
		{
			mapSurfaceToColour[surface] = colour;
		}

		bool TryGetColour(ESchemeColourSurface surface, RGBAb& colour) const override
		{
			auto i = mapSurfaceToColour.find(surface);
			if (i == mapSurfaceToColour.end())
			{
				return false;
			}
			colour = i->second;
			return true;
		}
	};
}

namespace Rococo::Gui
{
	ISchemeSupervisor* CreateScheme()
	{
		return new ANON::Scheme();
	}

	ROCOCO_GUI_RETAINED_API void SetSchemeColours_ThemeGrey(IScheme& scheme)
	{
		scheme.SetColour(ESchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(64, 64, 64, 192));
		scheme.SetColour(ESchemeColourSurface::CONTAINER_BACKGROUND_HOVERED, RGBAb(64, 64, 64, 192));
		scheme.SetColour(ESchemeColourSurface::CONTAINER_TOP_LEFT, RGBAb(64, 64, 64, 192));
		scheme.SetColour(ESchemeColourSurface::CONTAINER_BOTTOM_RIGHT, RGBAb(64, 64, 64, 192));
		scheme.SetColour(ESchemeColourSurface::CONTAINER_TOP_LEFT_HOVERED, RGBAb(98, 98, 98, 192));
		scheme.SetColour(ESchemeColourSurface::CONTAINER_BOTTOM_RIGHT_HOVERED, RGBAb(80, 80, 80, 192));
		scheme.SetColour(ESchemeColourSurface::BACKGROUND, RGBAb(64, 64, 64, 192));
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON_PRESSED, RGBAb(96, 96, 96, 255));
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON_RAISED, RGBAb(64, 64, 64, 255));
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON_PRESSED_AND_HOVERED, RGBAb(128, 128, 128, 255));
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON_RAISED_AND_HOVERED, RGBAb(80, 80, 80, 255));
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(64, 64, 64, 255));
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(64, 64, 64, 255));
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT_PRESSED, RGBAb(255, 255, 255, 255));
		scheme.SetColour(ESchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT_PRESSED, RGBAb(224, 224, 224, 255));
		scheme.SetColour(ESchemeColourSurface::BUTTON_PRESSED, RGBAb(96, 96, 96, 255));
		scheme.SetColour(ESchemeColourSurface::BUTTON_RAISED, RGBAb(64, 64, 64, 255));
		scheme.SetColour(ESchemeColourSurface::BUTTON_PRESSED_AND_HOVERED, RGBAb(128, 128, 128, 255));
		scheme.SetColour(ESchemeColourSurface::BUTTON_RAISED_AND_HOVERED, RGBAb(80, 80, 80, 255));
		scheme.SetColour(ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT, RGBAb(64, 64, 64, 255));
		scheme.SetColour(ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(64, 64, 64, 255));
		scheme.SetColour(ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT_PRESSED, RGBAb(255, 255, 255, 255));
		scheme.SetColour(ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT_PRESSED, RGBAb(224, 224, 224, 255));
		scheme.SetColour(ESchemeColourSurface::BUTTON_TEXT, RGBAb(255, 255, 255, 255));
	}
}