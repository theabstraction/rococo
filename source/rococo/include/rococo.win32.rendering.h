// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once
#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.ui.h>

#ifndef ROCOCO_GRAPHICS_API
# define ROCOCO_GRAPHICS_API ROCOCO_API_IMPORT
#endif

namespace Rococo::IO
{
	struct IInstallation;
	struct ISysMonitor;
}

namespace Rococo
{
	struct IAppFactory;
	struct IApp;
	struct Platform;

	namespace Graphics
	{
		struct IRenderer;
		struct IShaderOptions;
		struct IWindowEventHandler;
	}

	namespace OS
	{
		struct IAppControl;
	}
}

namespace Rococo
{
	void CALLBACK RendererMain(HANDLE hInstanceLock, IO::IInstallation& installation, IAppFactory& factory);

	struct IAppEventHandler
	{
		virtual void OnWindowClose() = 0;
		virtual void OnKeyboardEvent(const RAWKEYBOARD& k, HKL hKeyboardEvent) = 0;
		virtual void OnMouseEvent(const RAWMOUSE& m, MouseContext context) = 0;
	};

	ROCOCO_INTERFACE IGraphicsWindow
	{
		virtual Graphics::IRenderer & Renderer() = 0;
		virtual Windows::IWindow& Window() = 0;
		virtual void CaptureEvents(IAppEventHandler* handler) = 0;
		virtual void MakeRenderTarget() = 0;
		virtual void Free() = 0;
	};

	// An IAppManager implementation manages a mainloop and periodically calls app.OnFrameUpdate
	// It is generally used when the major implementation language is C++
	ROCOCO_INTERFACE IAppManager
	{
		virtual void Free() = 0;
		virtual void Run(HANDLE hInstanceLock, IApp& app, OS::IAppControl& appControl) = 0;
	};

	ROCOCO_GRAPHICS_API IAppManager* CreateAppManager(IGraphicsWindow& window, IApp& app);

	struct IDirectAppFactory;

	// An IDirectAppManager implementation passes control to an IDirectApp. The IDirectApp implementation
	// implements its own mainloop. This allows a mod to have complete control of program execution.
	// MHost uses this methodology to give script writers complete control of program flow.
	ROCOCO_INTERFACE IDirectAppManager
	{
		virtual void Free() = 0;
		virtual void Run(HANDLE hInstanceLock) = 0;
		virtual void AddSysMonitor(Rococo::IO::ISysMonitor& sysMonitor) = 0;
	};
	ROCOCO_GRAPHICS_API IDirectAppManager* CreateAppManager(Platform& platform, IGraphicsWindow& window, IDirectAppFactory& factory);

	struct IMessageSink
	{
		virtual bool InterceptMessage(LRESULT& result, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	};


	struct WindowSpec
	{
		DWORD style;
		DWORD exStyle;
		int X;
		int Y;
		int Width;
		int Height;
		HWND hParentWnd;
		HINSTANCE hInstance;
		IMessageSink* messageSink;
		Vec2i minSpan;
	};

	struct AdapterDesc
	{
		char description[256];
		size_t sysMemoryMB;
		size_t videoMemoryMB;
		size_t sharedMemoryMB;
	};

	ROCOCO_INTERFACE IGraphicsWindowFactory
	{
		virtual IGraphicsWindow * CreateGraphicsWindow(Graphics::IWindowEventHandler& eventHandler, const WindowSpec & ws, bool linkedToControls) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IGraphicsLogger
	{
		virtual void Log(cstr message, ...) = 0;
		virtual void OnMessageException(IException& ex, uint32 uMsg) = 0;
		virtual void Free() = 0;
	};

	struct FactorySpec
	{
		HINSTANCE hResourceInstance = NULL;
		HICON largeIcon = NULL;;
		HICON smallIcon = NULL;;
		UINT adapterIndex = 0;
		bool preparePix = false;
	};

	// mplat will link to some graphics library, currently DX11, which implements the following 3 functions that establish the rendering system.

	ROCOCO_GRAPHICS_API bool Graphics_TryGetAdapterInfo(int index, AdapterDesc& d);
	ROCOCO_GRAPHICS_API IGraphicsLogger* CreateStandardOutputLogger();
	ROCOCO_GRAPHICS_API IGraphicsWindowFactory* CreateGraphicsWindowFactory(IO::IInstallation& installation, IGraphicsLogger& logger, const FactorySpec& spec, Rococo::Graphics::IShaderOptions& options);
}