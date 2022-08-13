#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Rococo.target.h>

#define NOMINMAX
#include <Windows.h>

#include <rococo.api.h>
#include <Rococo.window.h>
#include <rococo.strings.h>
#include <rococo.textures.h>
#include <d3d11.h>
#include "rococo.dx11.api.h"
#include "rococo.renderer.h"
#include "rococo.fonts.h"
#include "rococo.functional.h"

namespace Rococo::DX11
{
	using namespace Rococo::Samplers;

	struct IDX11ResourceLoader: Textures::ICompressedResourceLoader
	{
		virtual ID_PIXEL_SHADER CreateNamedPixelShader(cstr pingPath) = 0;
		virtual ID_PIXEL_SHADER CreatePixelShader(cstr pingPath) = 0;
		virtual ID_VERTEX_SHADER CreateVertexShader(cstr pingPath, const D3D11_INPUT_ELEMENT_DESC* vertexDesc, UINT nElements) = 0;
		virtual void LoadTextFile(cstr pingPath, Rococo::Function<void(const fstring& text)> callback) = 0;
		virtual IInstallation& Installation() = 0;
	};

	ROCOCOAPI IShaderStateControl
	{
		virtual bool UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid) = 0;
		virtual Vec2i SelectTexture(ID_TEXTURE txId) = 0;
		virtual IMaterials& Materials() = 0;
	};

	ROCOCOAPI IDX11TextureArray : public Rococo::Textures::ITextureArray
	{
		virtual void Free() = 0;
		virtual void Resize(size_t nElements) = 0;
		virtual int32 Height() const = 0;
		virtual int32 Width() const = 0;
		virtual ID3D11ShaderResourceView* View() = 0;
		virtual DX11::TextureBind& Binding() = 0;
		virtual void SetActiveDC(ID3D11DeviceContext* dc) = 0;
	};

	IDX11TextureArray* CreateDX11TextureArray(ID3D11Device& device, ID3D11DeviceContext& activeDC);
	IDX11TextureArray* LoadAlphaTextureArray(ID3D11Device& device, Vec2i span, int32 nElements, ITextureLoadEnumerator& enumerator, ID3D11DeviceContext& activeDC);

	ROCOCOAPI IDX11FontRenderer
	{
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void FlushLayer() = 0;
		virtual bool ApplyGuiShader() = 0;
		virtual bool ApplyHQFontsShader() = 0;
	};

	ROCOCOAPI IDX11HQFontResource : public IHQFontResource
	{
		virtual void Free() = 0;
		virtual const Fonts::ArrayFontMetrics& GetFontMetrics(ID_FONT idFont) = 0;
		virtual void RenderHQText(ID_FONT id, Rococo::Fonts::IHQTextJob& job, IGuiRenderContext::EMode mode, ID3D11DeviceContext& dc, IShaderStateControl& shaders) = 0;
	};

	IDX11HQFontResource* CreateDX11HQFonts(IInstallation& installation, IDX11FontRenderer& renderer, ID3D11Device& device, ID3D11DeviceContext& dc);

	ROCOCOAPI IDX11CubeTextures
	{
		virtual ID_CUBE_TEXTURE CreateCubeTexture(TextureLoader& textureLoader, cstr path, cstr extension) = 0;
		virtual void Free() = 0;
		virtual ID3D11ShaderResourceView* GetShaderView(ID_CUBE_TEXTURE id) = 0;
		virtual ID3D11ShaderResourceView* ShaderResourceView() = 0;
		virtual void SyncCubeTexture(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace, IDX11TextureArray& materialArray) = 0;
	};

	IDX11CubeTextures* CreateCubeTextureManager(ID3D11Device& device, ID3D11DeviceContext& dc);

	ROCOCOAPI IDX11Gui: public IGuiRenderContext
	{
		virtual bool ApplyGuiShader() = 0;
		virtual bool ApplyHQFontsShader() = 0;
		virtual bool ApplyGuiShader(ID_PIXEL_SHADER idGuiOverrideShader) = 0;
		virtual void DrawCursor(const GuiMetrics& metrics, EWindowCursor cursorId) = 0;
		virtual void DrawCustomTexturedMesh(const GuiRect& absRect, ID_TEXTURE id, cstr pixelShaderPingPath, const GuiVertex* vertices, size_t nCount) = 0;
		virtual void DrawGlyph(cr_vec2 uvTopLeft, cr_vec2 uvBottomRight, cr_vec2 posTopLeft, cr_vec2 posBottomRight, Fonts::FontColour fcolour) = 0;
		virtual Fonts::IFont& FontMetrics() = 0;
		virtual void Free() = 0;	
		virtual IDX11FontRenderer& FontRenderer() = 0;
		virtual GuiScale GetGuiScale() const = 0;
		virtual void RenderGui(IScene& scene, const GuiMetrics& metrics, bool renderOverlays) = 0;
		virtual void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect) = 0;
		virtual void SetCursorBitmap(const Textures::BitmapLocation& sprite, Vec2i hotspotOffset) = 0;
		virtual void ShowVenue(IMathsVisitor& visitor) = 0;
		virtual Textures::ITextureArrayBuilder& SpriteBuilder() = 0;
		virtual ID3D11ShaderResourceView* SpriteView() = 0;

	};

	IDX11Gui* CreateDX11Gui(IRendererMetrics& metrics, IDX11ResourceLoader& loader, IShaderStateControl& shaders, ID3D11Device& device, ID3D11DeviceContext& dc);

	void GetSkySampler(D3D11_SAMPLER_DESC& desc);
	D3D11_TEXTURE_ADDRESS_MODE From(AddressMode mode);

	ID3D11SamplerState* GetSampler(ID3D11Device& device, uint32 index, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour);

	void GetTextureDesc(TextureDesc& desc, ID3D11Texture2D& texture);
	void ShowVenueForDevice(IMathsVisitor& visitor, ID3D11Device& device);
	TextureBind CreateDepthTarget(ID3D11Device& device, int32 width, int32 height);
	TextureBind CreateRenderTarget(ID3D11Device& device, int32 width, int32 height);

	// Draw a light cone and return the number of triangles in the mesh
	int DrawLightCone(const Light& light, cr_vec3 viewDir, ID3D11DeviceContext& dc, ID3D11Buffer& lightConeBuffer);

	bool PrepareDepthRenderFromLight(const Light& light, DepthRenderData& drd);

	ROCOCOAPI IOverlays
	{
		virtual void Render(IGuiRenderContext& grc) = 0;
	};

	ROCOCOAPI IOverlaySupervisor: IOverlays
	{
		virtual void Free() = 0;
	};

	IOverlaySupervisor* CreateOverlays();

	ROCOCOAPI IDX11Materials: IMaterials
	{
		virtual void Free() = 0;
		virtual void ShowVenue(IMathsVisitor& visitor) = 0;
		virtual IDX11TextureArray& Textures() = 0;
	};

	IDX11Materials* CreateMaterials(IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& dc);
}
