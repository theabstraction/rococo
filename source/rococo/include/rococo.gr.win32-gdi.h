#pragma once

#include <rococo.gr.client.h>
#ifdef _WIN32
# include <rococo.win32.target.win7.h>
# define NOMINMAX
# include <windows.h>
#endif

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
		virtual void OnPaint(IGR2DScene& scene, HWND hWnd, HDC paintDC) = 0;
		virtual void RenderGui(Gui::IGRSystem& gr, HWND hWnd, HDC paintDC) = 0;
		virtual void RouteKeyboardEvent(const KeyboardEvent& key, Gui::IGRSystem& gr) = 0;
		virtual void RouteMouseEvent(const MouseEvent& me, Gui::IGRSystem& gr) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_GR_GDI_API IWin32GDICustodianSupervisor* CreateGDICustodian();

	ROCOCO_INTERFACE IWin32GDIApp
	{
		virtual void Free() = 0;
	};

	// This should be called in any Win32 App that uses this library, it manages GDI startup and shutdown
	ROCOCO_GR_GDI_API IWin32GDIApp* CreateWin32GDIApp();
}