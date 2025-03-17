#pragma once

#include <rococo.gr.client.h>
#include <rococo.os.win32.h>

#ifdef DrawText
# undef DrawText
#endif

namespace Rococo
{
	struct KeyboardEvent;
	struct MouseEvent;
}

namespace Rococo::Gui
{
	DECLARE_ROCOCO_INTERFACE IGRCustodian;
	DECLARE_ROCOCO_INTERFACE IGRSystem;
}

#ifndef ROCOCO_GR_GDI_API
# define ROCOCO_GR_GDI_API ROCOCO_API_IMPORT
#endif

namespace Rococo::GR::Win32
{
	ROCOCO_INTERFACE IWin32GDICustodianSupervisor
	{
		virtual Gui::IGRCustodian& Custodian() = 0;
		virtual void OnPaint(IGR2DScene& scene, HWND hWnd) = 0;
		virtual void RenderGui(Gui::IGRSystem& gr, HWND hWnd) = 0;
		virtual void RouteKeyboardEvent(const KeyboardEvent& key, Gui::IGRSystem& gr) = 0;
		virtual void RouteMouseEvent(const MouseEvent& me, Gui::IGRSystem& gr) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_GR_GDI_API IWin32GDICustodianSupervisor* CreateGDICustodian();
}