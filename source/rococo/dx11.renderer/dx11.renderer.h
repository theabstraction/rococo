#pragma once

#include <rococo.api.h>

#ifndef ROCOCO_DX_API
# define ROCOCO_DX_API ROCOCO_API_IMPORT
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Rococo.target.h>

#define NOMINMAX
#include <Windows.h>

#include <Rococo.window.h>
#include <rococo.strings.h>
#include <rococo.textures.h>
#include <d3d11.h>
#include "rococo.dx11.api.h"
#include "rococo.renderer.h"
#include "rococo.fonts.h"
#include "rococo.functional.h"
#include <RAL/RAL.h>

namespace Rococo::RAL
{
	struct IRAL;
	struct IPipeline;
}

namespace Rococo::DX11
{
	using namespace Rococo::Graphics;
	using namespace Rococo::Graphics::Samplers;

	ROCOCO_INTERFACE IDX11TextureLoader
	{
		virtual TextureBind LoadAlphaBitmap(cstr resourceName) = 0;
		virtual TextureBind LoadColourBitmap(cstr resourceName) = 0;
		virtual void LoadColourBitmapIntoAddress(cstr resourceName, IColourBitmapLoadEvent& onLoad) = 0;
	};

	struct IDX11Shaders;
	struct IDX11WindowBacking;
	struct IDX11TextureManager;

	struct RenderTarget
	{
		ID3D11RenderTargetView* renderTargetView;
		ID3D11DepthStencilView* depthView;
	};

	ROCOCO_INTERFACE IDX11RenderTarget
	{
		virtual RenderTarget GetRenderTarget(ID_TEXTURE depthId, ID_TEXTURE colourBufferId) = 0;
	};

	ROCOCO_INTERFACE IDX11ResourceLoader: Textures::ICompressedResourceLoader
	{
		virtual IDX11Shaders& DX11Shaders() = 0;
		virtual void LoadTextFile(cstr pingPath, Rococo::Function<void(const fstring& text)> callback) = 0;
		virtual IO::IInstallation& Installation() = 0;
	};

	ROCOCO_INTERFACE IDX11BitmapArray : public Textures::IBitmapArray
	{
		virtual void Free() = 0;
		virtual void Resize(size_t nElements) = 0;
		virtual int32 Height() const = 0;
		virtual int32 Width() const = 0;
		virtual ID3D11ShaderResourceView* View() = 0;
		virtual DX11::TextureBind& Binding() = 0;
		virtual void SetActiveDC(ID3D11DeviceContext* dc) = 0;
	};

	IDX11BitmapArray* CreateDX11BitmapArray(ID3D11Device& device, ID3D11DeviceContext& activeDC);
	IDX11BitmapArray* LoadAlphaBitmapArray(ID3D11Device& device, Vec2i span, int32 nElements, ITextureLoadEnumerator& enumerator, ID3D11DeviceContext& activeDC);

	ROCOCO_INTERFACE IDX11FontRenderer
	{
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void FlushLayer() = 0;
		virtual bool ApplyGuiShader() = 0;
		virtual bool ApplyHQFontsShader() = 0;
	};

	ROCOCO_INTERFACE IDX11HQFontResource : public IHQFontResource
	{
		virtual void Free() = 0;		
		virtual void RenderHQText(ID_FONT id, Fonts::IHQTextJob& job, IGuiRenderContext::EMode mode, ID3D11DeviceContext& dc, const GuiRect& clipRect) = 0;
	};

	IDX11HQFontResource* CreateDX11HQFonts(IDX11FontRenderer& renderer, ID3D11Device& device, ID3D11DeviceContext& dc);

	ROCOCO_INTERFACE IDX11CubeTextures
	{
		virtual ID_CUBE_TEXTURE CreateCubeTexture(IDX11TextureLoader& textureLoader, cstr path, cstr extension) = 0;
		virtual void Free() = 0;
		virtual ID3D11ShaderResourceView* GetShaderView(ID_CUBE_TEXTURE id) = 0;
		virtual ID_CUBE_TEXTURE CreateCubeTextureFromMaterialArray(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace, IDX11BitmapArray& materialArray) = 0;
	};

	IDX11CubeTextures* CreateCubeTextureManager(ID3D11Device& device, ID3D11DeviceContext& dc);

	ROCOCO_INTERFACE IDX11Gui: public IGuiRenderContextSupervisor
	{
		virtual bool ApplyGuiShader() = 0;
		virtual bool ApplyHQFontsShader() = 0;
		virtual bool ApplyGuiShader(ID_PIXEL_SHADER idGuiOverrideShader) = 0;
		virtual void AssignShaderResourcesToDC() = 0;
		virtual void DrawCursor(const GuiMetrics& metrics) = 0;
		virtual void DrawCustomTexturedMesh(const GuiRect& absRect, ID_TEXTURE id, cstr pixelShaderPingPath, const GuiVertex* vertices, size_t nCount) = 0;
		virtual void DrawGlyph(cr_vec2 uvTopLeft, cr_vec2 uvBottomRight, cr_vec2 posTopLeft, cr_vec2 posBottomRight, Fonts::FontColour fcolour) = 0;
		virtual void Free() = 0;	
		virtual void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect) = 0;
		virtual void SetCursorBitmap(const Textures::BitmapLocation& sprite, Vec2i hotspotOffset) = 0;
		virtual void SetSysCursor(EWindowCursor id) = 0;
		virtual void ShowVenue(IMathsVisitor& visitor) = 0;
		virtual void RegisterSubsystem(ISubsystemMonitor& monitor, ID_SUBSYSTEM parentId) = 0;

		virtual Fonts::IFont& FontMetrics() = 0;
		virtual IDX11FontRenderer& FontRenderer() = 0;
		virtual IGuiResources& Resources() = 0;
		virtual Textures::IBitmapArrayBuilder& SpriteBuilder() = 0;
		virtual ID3D11ShaderResourceView* SpriteView() = 0;

	};

	ROCOCO_INTERFACE IRenderingResources
	{
		virtual ID_CUBE_TEXTURE GetEnvMapId() const = 0;
	};

	D3D11_TEXTURE_ADDRESS_MODE From(AddressMode mode);

	ID3D11SamplerState* GetSampler(ID3D11Device& device, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour);

	void GetTextureDesc(TextureDesc& desc, ID3D11Texture2D& texture);
	void ShowVenueForDevice(IMathsVisitor& visitor, ID3D11Device& device);
	TextureBind CreateDepthTarget(ID3D11Device& device, int32 width, int32 height);
	TextureBind CreateRenderTarget(ID3D11Device& device, int32 width, int32 height, TextureFormat format);

	bool PrepareDepthRenderFromLight(const LightConstantBuffer& light, DepthRenderData& drd);

	ROCOCO_INTERFACE IDX11Materials: IMaterials
	{
		virtual void Free() = 0;
		virtual void ShowVenue(IMathsVisitor& visitor) = 0;
		virtual IDX11BitmapArray& Textures() = 0;
	};

	IDX11Materials* CreateMaterials(IO::IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& dc);

	struct TextureDescState
	{
		float width;
		float height;
		float inverseWidth;
		float inverseHeight;
		float redActive;
		float greenActive;
		float blueActive;
		float alphaActive;
	};

	class TextureLoader : public IDX11TextureLoader
	{
		IO::IInstallation& installation;
		ID3D11Device& device;
		ID3D11DeviceContext& dc;
		IExpandingBuffer& scratchBuffer;

	public:
		TextureLoader(IO::IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& _dc, IExpandingBuffer& _scratchBuffer);
		TextureBind LoadAlphaBitmap(cstr resourceName);
		TextureBind LoadColourBitmap(cstr resourceName);
		void LoadColourBitmapIntoAddress(cstr resourceName, IColourBitmapLoadEvent& onLoad);
	};

	ROCOCO_INTERFACE IDX11Renderer : IRenderer
	{
		virtual ID3D11RenderTargetView* BackBuffer() = 0;
		virtual void OnWindowResized(IDX11WindowBacking& window, Vec2i span) = 0;
		virtual void SetWindowBacking(IDX11WindowBacking* windowBacking) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IDX11SpecialResources
	{
		virtual ID3D11RenderTargetView * BackBuffer() = 0;
	};

	ROCOCO_INTERFACE IDX11TextureManager: ITextureManager
	{
		virtual void Free() = 0;
		virtual ID3D11ShaderResourceView* GetShaderView(ID_CUBE_TEXTURE id) = 0;
		virtual TextureBind& GetTexture(ID_TEXTURE id) = 0;
		virtual IDX11TextureLoader& Loader() = 0;
		virtual IDX11Materials& Materials() = 0;
		virtual IDX11CubeTextures& DX11CubeTextures() = 0;
	};

	IDX11TextureManager* CreateTextureManager(IO::IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& dc, IDX11SpecialResources& specialResources);

	ROCOCO_INTERFACE IDX11Meshes: public IMeshes
	{
		virtual void Free() = 0;
	};

	IDX11Meshes* CreateMeshManager(ID3D11Device& device, ID3D11DeviceContext& dc);

	ROCOCO_INTERFACE IDX11Shaders : IShaders
	{
		virtual ID_VERTEX_SHADER CreateVertexShader(cstr pingPath, const D3D11_INPUT_ELEMENT_DESC* vertexDesc, UINT nElements) = 0;
		virtual void Free() = 0;
	};

	IDX11Shaders* CreateShaderManager(IO::IInstallation& installation, IShaderOptions& shaderOptions, ID3D11Device& device, ID3D11DeviceContext& dc);

	ROCOCO_INTERFACE IDX11Pipeline
	{
		virtual void Free() = 0;
		virtual void RegisterSubsystem(ISubsystemMonitor& monitor, ID_SUBSYSTEM parentId) = 0;
		virtual void SetSamplerDefaults(uint32 index, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour) = 0;
		virtual void SetBoneMatrix(uint32 index, cr_m4x4 m) = 0;
		virtual void ShowVenue(IMathsVisitor& visitor) = 0;
		virtual IGuiResources& GuiResources() = 0;
		virtual IGui3D& Gui3D() = 0;
		virtual RAL::IPipeline& RALPipeline() = 0;
	};

	struct RenderBundle
	{
		IO::IInstallation& installation;
		IRendererMetrics& metrics;
		IDX11ResourceLoader& resourceLoader;
		IDX11Shaders& shaders;
		IDX11TextureManager& textures;
		IDX11Meshes& meshes;
		IDX11Renderer& renderer;
		IRenderContext& rc;
		ID3D11Device& device;
		ID3D11DeviceContext& dc;
		IDX11ResourceLoader& loader;
		IRenderingResources& resources;
		Rococo::RAL::IRAL& RAL;
	};

	IDX11Gui* CreateDX11Gui(RenderBundle& bundle);
	IDX11Pipeline* CreateDX11Pipeline(RenderBundle& bundle);

	ROCOCO_INTERFACE IDX11WindowBacking
	{
		virtual ID3D11RenderTargetView* BackBufferView() = 0;
		virtual ID_TEXTURE DepthBufferId() const = 0;
		virtual void Free() = 0;
		virtual bool IsFullscreen() = 0;
		virtual void Present() = 0;	
		virtual void ResetOutputBuffersForWindow() = 0;
		virtual Vec2i Span() const = 0;
		virtual void SwitchToFullscreen() = 0;
		virtual void SwitchToWindowMode() = 0;
		virtual Windows::IWindow& Window() = 0;
	};

	IDX11WindowBacking* CreateDX11WindowBacking(ID3D11Device& device, ID3D11DeviceContext& dc, HWND hWnd, IDXGIFactory& factory, IDX11TextureManager& textures);

	ROCOCO_DX_API void ReportMemoryStatus();

	ROCOCO_INTERFACE IDX11IRALVertexDataBuffer : RAL::IRALVertexDataBuffer
	{
		virtual ID3D11Buffer* RawBuffer() = 0;
	};

	ROCOCO_INTERFACE IDX11IRALConstantDataBuffer : RAL::IRALConstantDataBuffer
	{
		virtual ID3D11Buffer* RawBuffer() = 0;
	};

	cstr ToString(D3D11_COMPARISON_FUNC f);
	void Reflect(Reflection::IReflectionVisitor& v, cstr section, ID3D11RasterizerState& state);
	void Reflect(Reflection::IReflectionVisitor& v, cstr section, ID3D11BlendState& state);
	void Reflect(Reflection::IReflectionVisitor& v, cstr section, ID3D11DepthStencilState& state);
} // Rococo::DX11

namespace Rococo::Memory
{
	IAllocator& GetDX11Allocator();
}