#pragma once

#pragma once
#define ROCOCO_API __declspec(dllimport)
#define ROCOCO_UTIL_API __declspec(dllimport)
#define SEXYUTIL_API __declspec(dllimport)
#define SCRIPTEXPORT_API __declspec(dllimport)
#define ROCOCO_SEXML_API __declspec(dllimport)
#define ROCOCO_GUI_RETAINED_API _declspec(dllimport)

#define ROCOCO_USE_SAFE_V_FORMAT

#include <rococo.gui.retained.ex.h>

namespace Rococo::Gui
{
	ROCOCO_INTERFACE IUE5_GRCustodianSupervisor : IGRCustodianSupervisor
	{
		virtual void Render(IGRSystem& gr) = 0;
		virtual void RouteKeyboardEvent(const KeyboardEvent& key, IGRSystem& gr) = 0;
		virtual void RouteMouseEvent(const MouseEvent& me, IGRSystem& gr) = 0;
	};
}