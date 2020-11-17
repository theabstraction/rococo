#pragma once

#include <rococo.renderer.h>

#define VALIDATE_HR(hr) ValidateHR(hr, #hr, __FUNCTION__, __FILE__, __LINE__);

struct IDXGIFactory7;
struct IDXGIAdapter4;
struct IDXGIOutput;

namespace Rococo::Graphics
{
	struct IShaderCache;

	struct AdapterContext
	{
		IDXGIFactory7& f;
		IDXGIAdapter4& adapter;
		IDXGIOutput& output;
	};

	ROCOCOAPI IWin32AdapterContext
	{
		virtual AdapterContext& AC() = 0;
		virtual void Free() = 0;
	};

	IWin32AdapterContext* CreateWin32AdapterContext(int32 adapterIndex, int32 outputIndex);

	ROCOCOAPI IDX11Window
	{
		virtual Rococo::Windows::IWindow& Window() = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX1RendererWindowEventHandler
	{
		virtual void OnCloseRequested(IDX11Window & window) = 0;
		virtual void OnMessageQueueException(IDX11Window& window, IException& ex) = 0;
		virtual void OnKeyboardEvent(const RAWKEYBOARD& k, HKL hKeyboardLayout) = 0;
		virtual void OnMouseEvent(const RAWMOUSE& m) = 0;
	};

	struct DX11WindowContext
	{
		Windows::IWindow& parent;
		IDX1RendererWindowEventHandler& evHandler;
		Vec2i windowedSpan;
		cstr title;
		void* hResourceInstance;
	};

	ROCOCOAPI IDX11System
	{
		virtual IShaderCache & Shaders() = 0;
		virtual IDX11Window * CreateDX11Window(DX11WindowContext& context) = 0;
		virtual void Free() = 0;
	};

	IDX11System* CreateDX11System(AdapterContext& ac, IInstallation& installation);

	void ValidateHR(HRESULT hr, const char* badcall, const char* function, const char* file, int lineNumber);
}
