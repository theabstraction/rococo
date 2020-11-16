#pragma once

#include <rococo.renderer.h>

#define VALIDATE_HR(hr) ValidateHR(hr, #hr, __FUNCTION__, __FILE__, __LINE__);

struct IDXGIFactory7;
struct IDXGIAdapter4;
struct IDXGIOutput;

namespace Rococo::Graphics
{
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

	struct ShaderView
	{
		cstr resourceName;
		int hr;
		cstr errorString;
		const void* blob;
		size_t blobCapacity;
	};

	// N.B the shader thread is locked until OnGrab returns
	// and the contents may change, so copy what you need and do not block within the method
	// A correct implementation should generally allow an exception within the handler
	ROCOCOAPI IShaderViewGrabber
	{
		// N.B the shader thread is locked until OnGrab returns
		// and the contents may change, so copy what you need and do not block within the method
		virtual void OnGrab(const ShaderView & view) = 0;
	};

	ROCOCOAPI IShaderCache
	{
		virtual	ID_PIXEL_SHADER AddPixelShader(const char* resourceName) = 0;
		virtual ID_VERTEX_SHADER AddVertexShader(const char* resourceName) = 0;
		virtual void GrabShaderObject(ID_PIXEL_SHADER pxId, IShaderViewGrabber& grabber) = 0;
		virtual void GrabShaderObject(ID_VERTEX_SHADER vxId, IShaderViewGrabber& grabber) = 0;
		virtual void GrabShaderObject(const char* resourceName, IShaderViewGrabber& grabber) = 0;
		virtual void ReloadShader(const char* resourceName) = 0;
		virtual uint32 InputQueueLength() = 0;
		virtual bool TryGrabAndPopNextError(IShaderViewGrabber& grabber) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX11System
	{
		virtual IShaderCache & Shaders() = 0;
		virtual IDX11Window * CreateDX11Window(DX11WindowContext& context) = 0;
		virtual void Free() = 0;
	};

	IDX11System* CreateDX11System(AdapterContext& ac, IInstallation& installation);

	void ValidateHR(HRESULT hr, const char* badcall, const char* function, const char* file, int lineNumber);

	IShaderCache* CreateShaderCache(IInstallation& installation);
}
