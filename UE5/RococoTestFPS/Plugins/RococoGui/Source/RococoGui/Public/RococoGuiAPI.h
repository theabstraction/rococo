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
		virtual void Bind(IGRSystemSupervisor& grSystem) = 0;
		virtual void Render(SlateRenderContext& rc) = 0;
		virtual void RouteKeyboardEvent(const KeyboardEvent& key) = 0;
		virtual void RouteMouseEvent(const MouseEvent& me) = 0;
	};
}

namespace Rococo::Gui
{
	ROCOCOGUI_API IUE5_GRCustodianSupervisor* Create_UE5_GRCustodian();
}
