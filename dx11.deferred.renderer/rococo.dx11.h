#pragma once

#include <rococo.renderer.h>

#define VALIDATE_HR(hr) ValidateHR(hr, #hr, __FUNCTION__, __FILE__, __LINE__);

struct IDXGIFactory7;
struct IDXGIAdapter4;
struct IDXGIOutput;
struct ID3D11Device5;
struct ID3D11DeviceContext4;

namespace Rococo::Textures
{
	struct ITextureArrayBuilderSupervisor;
}

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
		// Monitor and report shader errors. The cache reference must be valid for the monitor duration
		// If null is passed monitoring ends.
		virtual void MonitorShaderErrors(IShaderCache* cache) = 0;
		virtual Rococo::Windows::IWindow& Window() = 0;
		virtual void UpdateFrame() = 0;
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

	enum class TextureType : uint32
	{
		TextureType_None,
		TextureType_2D,
		TextureType_2D_Array,
		TextureType_2D_UVAtlas
	};

	struct TextureId
	{
		uint32 index : 24;
		TextureType type : 4;
		uint32 unused : 4;
	};

	static_assert(sizeof(TextureId) == sizeof(uint32));

	ROCOCOAPI ITextureBuilder
	{
		virtual TextureId AddTx2D_Grey(cstr name) = 0;
		virtual TextureId AddTx2DArray_Grey(cstr name, Vec2i span) = 0;
		virtual TextureId AddTx2D_RGBAb(cstr name) = 0;
		virtual TextureId AddTx2DArray_RGBAb(cstr name, Vec2i span) = 0;
		virtual TextureId AddTx2D_UVAtlas(cstr name) = 0;
		virtual int32 AddElementToArray(TextureId, cstr name) = 0;
		virtual void EnableMipMapping(TextureId id) = 0;
	};

	ROCOCOAPI ITextureCache : ITextureBuilder
	{
		virtual void ReloadAsset(TextureId id) = 0;
		virtual bool AssignTextureToDC(TextureId id, uint32 textureUnit) = 0;
		virtual void Free() = 0;
	};

	ITextureCache* CreateTextureCache(IInstallation& installation, ID3D11Device5& device, ID3D11DeviceContext4& dc);

	ROCOCOAPI IDX11System
	{
		virtual ITextureCache & Textures() = 0;
		virtual IShaderCache& Shaders() = 0;
		virtual IDX11Window * CreateDX11Window(DX11WindowContext& context) = 0;
		virtual void Free() = 0;
	};

	IDX11System* CreateDX11System(AdapterContext& ac, IInstallation& installation);

	void ValidateHR(HRESULT hr, const char* badcall, const char* function, const char* file, int lineNumber);
}
