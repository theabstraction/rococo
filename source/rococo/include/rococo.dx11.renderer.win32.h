#ifndef ROCOCO_DX11_RENDERER_WIN32_H
#define ROCOCO_DX11_RENDERER_WIN32_H

namespace Rococo::IO
{
	struct IInstallation;
}

namespace Rococo
{
	struct IAppFactory;
	struct IApp;

	namespace Graphics
	{
		struct IRenderer;
	}

	using namespace Rococo::Graphics;

	namespace OS
	{
		struct IAppControl;
	}

	void CALLBACK RendererMain(HANDLE hInstanceLock, IO::IInstallation& installation, IAppFactory& factory);

	ROCOCO_INTERFACE IDX11Window
	{
	   virtual void Free() = 0;
	   virtual IRenderer& Renderer() = 0;
	};

	struct IAppEventHandler
	{
		virtual void OnWindowClose() = 0;
		virtual void OnKeyboardEvent(const RAWKEYBOARD& k, HKL hKeyboardEvent) = 0;
		virtual void OnMouseEvent(const RAWMOUSE& m) = 0;
	};

	ROCOCO_INTERFACE IDX11GraphicsWindow
	{
		virtual IRenderer& Renderer() = 0;
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

	IAppManager* CreateAppManager(IDX11GraphicsWindow& window, IApp& app);

	struct IDirectAppFactory;

	// An IDirectAppManager implementation passes control to an IDirectApp. The IDirectApp implementation
	// implements its own mainloop. This allows a mod to have complete control of program execution.
	// MHost uses this methodology to give script writers complete control of program flow.
	ROCOCO_INTERFACE IDirectAppManager
	{
		virtual void Free() = 0;
		virtual void Run(HANDLE hInstanceLock) = 0;
	};
	IDirectAppManager* CreateAppManager(Platform& platform, IDX11GraphicsWindow& window, IDirectAppFactory& factory);

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

	ROCOCO_INTERFACE IDX11Factory
	{
		virtual IDX11GraphicsWindow* CreateDX11Window(const WindowSpec& ws, bool linkedToDX11Controls) = 0;
		virtual void Free() = 0;
	};

	bool DX11_TryGetAdapterInfo(int index, AdapterDesc& d);

	ROCOCO_INTERFACE IDX11Logger
	{
		virtual void Log(cstr message, ...) = 0;
		virtual void OnMessageException(IException& ex, uint32 uMsg) = 0;
		virtual void Free() = 0;
	};

	struct FactorySpec
	{
		HINSTANCE hResourceInstance;
		HICON largeIcon;
		HICON smallIcon;
		int adapterIndex = 0;
	};

	IDX11Factory* CreateDX11Factory(IO::IInstallation& installation, IDX11Logger& logger, const FactorySpec& spec);
	IDX11Logger* CreateStandardOutputLogger();
}

#endif