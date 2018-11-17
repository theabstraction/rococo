#ifndef ROCOCO_DX11_RENDERER_WIN32_H
#define ROCOCO_DX11_RENDERER_WIN32_H

namespace Rococo
{
	struct IAppFactory;
	struct IInstallation;
	struct IApp;

	void CALLBACK RendererMain(HANDLE hInstanceLock, IInstallation& installation, IAppFactory& factory);

	ROCOCOAPI IDX11Window
	{
	   virtual void Free() = 0;
	   virtual IRenderer& Renderer() = 0;
	};

	struct IAppEventHandler
	{
		virtual void OnWindowClose() = 0;
		virtual void OnKeyboardEvent(const RAWKEYBOARD& k) = 0;
		virtual void OnMouseEvent(const RAWMOUSE& m) = 0;
	};

	ROCOCOAPI IDX11GraphicsWindow
	{
		virtual IRenderer& Renderer() = 0;
		virtual Windows::IWindow& Window() = 0;
		virtual void CaptureEvents(IAppEventHandler* handler) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IAppManager
	{
		virtual void Free() = 0;
		virtual void Run(HANDLE hInstanceLock, IApp& app) = 0;
	};

	IAppManager* CreateAppManager(IDX11GraphicsWindow& window, IApp& app);

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

	ROCOCOAPI IDX11Factory
	{
		virtual IDX11GraphicsWindow* CreateDX11Window(const WindowSpec& ws) = 0;
		virtual void Free() = 0;
	};

	bool DX11_TryGetAdapterInfo(int index, AdapterDesc& d);

	ROCOCOAPI IDX11Logger
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

	IDX11Factory* CreateDX11Factory(IInstallation& installation, IDX11Logger& logger, const FactorySpec& spec);
	IDX11Logger* CreateStandardOutputLogger();
}

#endif