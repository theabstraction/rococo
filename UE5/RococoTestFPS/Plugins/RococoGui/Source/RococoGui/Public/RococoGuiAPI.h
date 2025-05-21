#pragma once

#include <Runtime\Core\Public\HAL\Platform.h>

#include <rococo.gui.retained.ex.h>

namespace Rococo
{
	struct SlateRenderContext;
}

namespace Rococo::Gui
{
	ROCOCO_INTERFACE IUE5_GRCustodianSupervisor : IGRCustodianSupervisor
	{
		virtual void Render(SlateRenderContext& rc, IGRSystem & gr) = 0;
		virtual void RouteKeyboardEvent(const KeyboardEvent& key, IGRSystem& gr) = 0;
		virtual void RouteMouseEvent(const MouseEvent& me, IGRSystem& gr) = 0;
	};
}

namespace Rococo::Gui
{
	ROCOCOGUI_API IUE5_GRCustodianSupervisor* Create_UE5_GRCustodian();
}
