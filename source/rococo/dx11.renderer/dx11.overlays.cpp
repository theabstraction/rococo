#include "dx11.renderer.h"
#include <vector>
#include <algorithm>

using namespace Rococo;
using namespace Rococo::DX11;

struct Overlay
{
	int32 zOrder;
	IUIOverlay* overlay;
};

bool operator < (const Overlay& a, const Overlay& b)
{
	return a.zOrder < b.zOrder;
}

bool operator == (const Overlay& a, IUIOverlay* b)
{
	return a.overlay == b;
}

struct Overlays : public IOverlaySupervisor
{
	std::vector<Overlay> overlays;

	void AddOverlay(int zorder, IUIOverlay* overlay)
	{
		auto i = std::find(overlays.begin(), overlays.end(), overlay);
		if (i == overlays.end())
		{
			overlays.push_back({ zorder, overlay });
		}
		else
		{
			i->zOrder = zorder;
		}

		std::sort(overlays.begin(), overlays.end());
	}

	void RemoveOverlay(IUIOverlay* overlay)
	{
		auto i = std::remove(overlays.begin(), overlays.end(), overlay);
		overlays.erase(i, overlays.end());
	}

	void Render(IGuiRenderContext& grc) override
	{
		for (auto& o : overlays)
		{
			o.overlay->Render(grc);
		}
	}

	void Free() override
	{
		delete this;
	}
};

namespace Rococo::DX11
{
	IOverlaySupervisor* CreateOverlays()
	{
		return new Overlays();
	}
}