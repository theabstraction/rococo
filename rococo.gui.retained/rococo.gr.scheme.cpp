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

		RGBAb GetColour(ESchemeColourSurface surface) override
		{
			auto i = mapSurfaceToColour.find(surface);
			return i != mapSurfaceToColour.end() ? i->second : RGBAb(0, 0, 0, 0);
		}

		void SetColour(ESchemeColourSurface surface, RGBAb colour) override
		{
			mapSurfaceToColour[surface] = colour;
		}
	};
}

namespace Rococo::Gui
{
	ISchemeSupervisor* CreateScheme()
	{
		return new ANON::Scheme();
	}
}