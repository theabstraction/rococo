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

	ROCOCO_INTERFACE IGRClientWindow
	{
		// Presents the given scene as background in the window. (The GUI is foreground).
		// The scene object must be valid for the lifetime of the client window, or until LinkScene is invoked again
		virtual void LinkScene(IGR2DScene* scene) = 0;

		// Triggers WM_PAINT as soon as the message queue is free
		virtual void QueuePaint() = 0;

		virtual Gui::IGRSystem& GRSystem() = 0;

		virtual operator HWND () = 0;
	};

	ROCOCO_INTERFACE IGRClientWindowSupervisor : IGRClientWindow
	{
		virtual void Free() = 0;
	};

	ROCOCO_GR_GDI_API IGRClientWindowSupervisor* CreateGRClientWindow(HWND hParentWnd);

	ROCOCO_GR_GDI_API cstr GetGRClientClassName();

	ROCOCO_INTERFACE IGRMainFrameWindow
	{
		virtual IGRClientWindow& Client() = 0;
		virtual operator HWND() = 0;
	};

	ROCOCO_INTERFACE IGRMainFrameWindowSupervisor : IGRMainFrameWindow
	{
		virtual void Free() = 0;
	};

	struct GRMainFrameConfig
	{
		HICON hLargeIconPath = nullptr;
		HICON hSmallIconPath = nullptr;
		HMENU hMainWindowMenu = nullptr;
	};

	ROCOCO_GR_GDI_API IGRMainFrameWindowSupervisor* CreateGRMainFrameWindow(HWND hOwner, const GRMainFrameConfig& config);
}