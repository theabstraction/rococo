#include <rococo.os.win32.h>
#include <rococo.strings.h>
#include <rococo.api.h>
#include <rococo.window.h>
#include <rococo.maths.h>
#include <rococo.dx12.h>
#include "rococo.dx12.helpers.inl"
#include <rococo.auto-release.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "rococo.dx12-msoft.h" // If this does not compile update your Windows 10 kit thing
#include <vector>
#include <algorithm>
#include <rococo.renderer.h>
#include <rococo.fonts.h>
#include <rococo.fonts.hq.h>
#include <rococo.textures.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Textures;
using namespace Rococo::OS;

namespace ANON
{
	bool operator == (const Fonts::FontSpec& a, const Fonts::FontSpec& b)
	{
		return
			Eq(a.fontName, b.fontName) &&
			a.height == b.height &&
			a.italic == b.italic &&
			a.weight == b.weight;
	}

	D3D12_TEXTURE_ADDRESS_MODE From(Samplers::AddressMode mode)
	{
		switch (mode)
		{
		case Samplers::AddressMode_Border:
			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		case Samplers::AddressMode_Wrap:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case Samplers::AddressMode_Mirror:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		default:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		}
	}

	struct Overlay
	{
		int32 zOrder;
		IUIOverlay* overlay;
	};

	bool operator == (const Overlay& a, const IUIOverlay* pB)
	{
		return a.overlay == pB;
	}

	class DX12Renderer: public IDX12Renderer, private IRenderer, public IMathsVenue
	{
	private:
		DX12WindowInternalContext ic;
		IShaderCache& shaders;
		IPipelineBuilder& pipelineBuilder;
		IDX12RendererWindow* window = nullptr;
		IInstallation& installation;

		AutoFree<IDX12TextureArray> spriteArray;
		AutoFree<IDX12TextureTable> textures;
		AutoFree<IDX12MaterialList> materials;
		AutoFree<ITextureManager> textureManager;
		ITextureMemory& textureMemory;
	public:
		DX12Renderer(IInstallation& ref_installation,
			DX12WindowInternalContext& ref_ic, 
			ITextureMemory& txMemory,
			IShaderCache& ref_shaders, 
			IPipelineBuilder& ref_pipelineBuilder
			)
			:
			installation(ref_installation), 
			ic(ref_ic), 
			shaders(ref_shaders),
			pipelineBuilder(ref_pipelineBuilder),
			textureMemory(txMemory)
		{
			meshBuffers = CreateMeshBuffers(ic);

			DX12TextureArraySpec spec{ txMemory };
			spriteArray = CreateDX12TextureArray("sprites", false, spec, ic);
			textures = CreateTextureTable(installation, ic);
			materials = CreateMaterialList(ic, txMemory, installation);
			textureManager = CreateTextureManager(installation);
		}

		virtual ~DX12Renderer()
		{
			
		}

		IRenderer& Renderer() override
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}

		std::vector<VertexTriangle> gui3DTriangles;

		void Add3DGuiTriangles(const VertexTriangle* first, const VertexTriangle* last)
		{
			for (auto i = first; i != last; ++i)
			{
				gui3DTriangles.push_back(*i);
			}
		}

		void Clear3DGuiTriangles()
		{
			gui3DTriangles.clear();
		}

		void SetTargetWindow(IDX12RendererWindow* window)
		{
			this->window = window;
		}

		enum { ID_FONT_OSFONT_OFFSET = 400 };

		struct OSFont
		{
			Fonts::IArrayFontSupervisor* arrayFont;
			Fonts::FontSpec spec;
			IDX12TextureArray* texArray2D;
		};

		std::vector<OSFont> osFonts;

		ID_FONT CreateOSFont(Fonts::IArrayFontSet& glyphs, const Fonts::FontSpec& spec) override
		{
			int i = 0;
			for (auto& osFont : osFonts)
			{
				if (osFont.spec == spec)
				{
					return ID_FONT{ i + ID_FONT_OSFONT_OFFSET };
				}

				i++;
			}

			AutoFree<Fonts::IArrayFontSupervisor> new_Font = Fonts::CreateOSFont(glyphs, spec);

			DX12TextureArraySpec txSpec{ this->textureMemory };
			IDX12TextureArray* fontArray = CreateDX12TextureArray("os-fonts", false, txSpec, ic);
			OSFont osFont{ new_Font, spec , fontArray };
			osFonts.push_back(osFont); // osFonts manages lifetime, so we can release our references
			new_Font.Release();

			struct : IEventCallback<const Fonts::GlyphDesc>
			{
				OSFont* font;
				size_t index = 0;
				void OnEvent(const Fonts::GlyphDesc& gd) override
				{
					struct : IImagePopulator<GRAYSCALE>
					{
						size_t index;
						OSFont* font;
						void OnImage(const GRAYSCALE* pixels, int32 width, int32 height) override
						{
							font->texArray2D->WriteSubImage((int) index, pixels, width, height);
						}
					} addImageToTextureArray;
					addImageToTextureArray.font = font;
					addImageToTextureArray.index = index++;
					font->arrayFont->GenerateImage(gd.charCode, addImageToTextureArray);
				}
			} addGlyphToTextureArray;

			addGlyphToTextureArray.font = &osFont;

			auto& font = *osFont.arrayFont;
			osFont.texArray2D->SetDimensions(font.Metrics().imgWidth, font.Metrics().imgHeight, font.NumberOfGlyphs());
			osFont.arrayFont->ForEachGlyph(addGlyphToTextureArray);

			return ID_FONT{ i + ID_FONT_OSFONT_OFFSET };
		}

		std::vector<Overlay> overlays;

		void AddOverlay(int zorder, IUIOverlay* overlay) override
		{
			auto i = std::find(overlays.begin(), overlays.end(), overlay);
			if (i == overlays.end())
			{
				overlays.push_back({ zorder, overlay });
			}
			else
			{
				i->zOrder = zorder;
			}

			std::sort(overlays.begin(), overlays.end(), 
				[] (const Overlay& a, const Overlay& b)
				{
					return a.zOrder < b.zOrder;
				}
			);
		}

		std::vector<ParticleVertex> fog;

		void AddFog(const ParticleVertex& p) override
		{
			fog.push_back(p);
		}

		std::vector<ParticleVertex> plasma;

		void AddPlasma(const ParticleVertex& p) override
		{
			plasma.push_back(p);
		}

		ID_TEXTURE CreateRenderTarget(int32 width, int32 height) override
		{
			return ID_TEXTURE::Invalid();
		}

		void CaptureMouse(bool enable) override
		{
			if (enable) SetCapture(Window());
			else ReleaseCapture();
		}

		AutoFree<IDX12MeshBuffers> meshBuffers;

		void ClearMeshes() override
		{
			meshBuffers->Clear();
		}

		void ClearFog() override
		{
			fog.clear();
		}

		void ClearPlasma() override
		{
			plasma.clear();
		}

		ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) override
		{
			return meshBuffers->CreateTriangleMesh(vertices, nVertices, weights);
		}

		ID_CUBE_TEXTURE CreateCubeTexture(cstr path, cstr extension) override
		{
			return ID_CUBE_TEXTURE::Invalid();
		}

		void DeleteMesh(ID_SYS_MESH id) override
		{
			meshBuffers->DeleteMesh(id);
		}

		ID_TEXTURE FindTexture(cstr name) const override
		{
			return ID_TEXTURE::Invalid();
		}

		AutoFree<Fonts::IFontSupervisor> fonts;

		Fonts::IFont& FontMetrics() override
		{
			return *fonts;
		}

		EWindowCursor cursorId = EWindowCursor_Default;

		void SetSysCursor(EWindowCursor id) override
		{
			cursorId = id;
		}

		Vec2i screenSpan = { 0,0 };

		void GetGuiMetrics(GuiMetrics& metrics) const override
		{
			HWND hWnd = window->Window();
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(hWnd, &p);

			metrics.cursorPosition = Vec2i{ p.x, p.y };
			metrics.screenSpan = screenSpan;
		}

		void GetMaterialArrayMetrics(MaterialArrayMetrics& metrics) const override
		{
			materials->GetMaterialArrayMetrics(metrics);
		}

		void GetMeshDesc(char desc[256], ID_SYS_MESH id) override
		{
			meshBuffers->GetDesc(desc, id);
		}

		bool TryGetTextureDesc(TextureDesc& desc, ID_TEXTURE id) const override
		{
			return false;
		}

		IInstallation& Installation() override
		{
			return installation;
		}

		void LoadMaterialTextureArray(IMaterialTextureArrayBuilder& builder) override
		{
			materials->LoadMaterialTextureArray(builder);
		}

		MaterialId GetMaterialId(cstr name) const override
		{
			return materials->GetMaterialId(name);
		}

		cstr GetMaterialTextureName(MaterialId id) const override
		{
			return materials->GetMaterialTextureName(id);
		}

		ID_TEXTURE LoadTexture(IBuffer& rawImageBuffer, cstr uniqueName) override
		{
			return textures->LoadTexture(rawImageBuffer, uniqueName);
		}

		AutoFree<ITextureArrayBuilderSupervisor> spriteArrayBuilder;

		Textures::ITextureArrayBuilder& SpriteBuilder() override
		{
			return *spriteArrayBuilder;
		}

		void OnSize(Vec2i span) override
		{
			screenSpan = span;
		}

		void Render(Graphics::ENVIRONMENTAL_MAP EnvironmentalMap, IScene& scene) override
		{

		}

		void RemoveOverlay(IUIOverlay* overlay) override
		{
			auto i = std::remove(overlays.begin(), overlays.end(), overlay);
			overlays.erase(i, overlays.end());
		}

		struct Cursor
		{
			BitmapLocation sprite = { {0,0,0,0}, 0, 0 };
			Vec2i hotspotOffset = { 0,0 };
		} cursor;

		void SetCursorBitmap(const Textures::BitmapLocation& sprite, Vec2i hotspotOffset) override
		{
			cursor.sprite = sprite;
			cursor.hotspotOffset = hotspotOffset;
		}

		void SetCursorVisibility(bool isVisible) override
		{
			if (isVisible)
			{
				for (int i = 0; i < 3; ++i)
				{
					int index = ShowCursor(TRUE);
					if (index >= 0)
					{
						ClipCursor(nullptr);
						SetCapture(nullptr);
						return;
					}
				}
			}
			else
			{
				for (int i = 0; i < 3; ++i)
				{
					int index = ShowCursor(FALSE);
					if (index < 0)
					{
						POINT pos;
						GetCursorPos(&pos);

						RECT rect{ pos.x - 1, pos.y - 1, pos.x + 1, pos.y + 1 };

						ClipCursor(&rect);
						SetCapture(Window());
						return;
					}
				}
			}
		}

		void SetSampler(uint32 index, Samplers::Filter, Samplers::AddressMode u, Samplers::AddressMode v, Samplers::AddressMode w, const RGBA& borderColour) override
		{
			Throw(0, "Not implemented");
		}

		void SetShadowCasting(ID_SYS_MESH id, boolean32 isActive) override
		{
			meshBuffers->SetShadowCasting(id, isActive);
		}

		void SetSpecialAmbientShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) override
		{

		}

		void SetSpecialSpotlightShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) override
		{

		}

		void ShowWindowVenue(IMathsVisitor& visitor) override
		{
			if (window) window->ShowWindowVenue(visitor);
		}

		void SyncCubeTexture(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace) override
		{

		}

		void SwitchToWindowMode() override
		{

		}

		IMathsVenue* TextureVenue() override
		{
			return nullptr;
		}

		ID_TEXTURE LoadAlphaTextureArray(cstr uniqueName, Vec2i span, int nElements, ITextureLoadEnumerator& enumerator) override
		{
			return ID_TEXTURE::Invalid();
		}

		void UpdateMesh(ID_SYS_MESH id, const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) override
		{
			meshBuffers->UpdateMesh(id, vertices, nVertices, weights);
		}

		void UpdatePixelShader(cstr pingPath) override
		{
			shaders.ReloadShader(pingPath);
		}

		void UpdateVertexShader(cstr pingPath) override
		{
			shaders.ReloadShader(pingPath);
		}

		Rococo::Windows::IWindow& Window() override
		{
			if (window)
			{
				return window->Window();
			}
			else
			{
				Throw(0, "No window set in DX12 renderer");
			}
		}

		IMathsVenue* Venue() override
		{
			return this;
		}

		Fonts::ArrayFontMetrics GetFontMetrics(ID_FONT idFont) override
		{
			int32 index = idFont.value - ID_FONT_OSFONT_OFFSET;
			if (index < 0 || index >= (int32)osFonts.size())
			{
				Throw(0, "%s - ID_FONT parameter was unknown value", __FUNCTION__);
			}

			return osFonts[index].arrayFont->Metrics();
		}

		char lastError[1024] = "none";

		ticks AIcost = 0;
		ticks guiCost = 0;
		ticks objCost = 0;
		ticks presentCost = 0;
		double frameTime = 10;
		double frameRate = 0;
		double hz = 0;

		int trianglesThisFrame = 0;
		int entitiesThisFrame = 0;

		void ShowVenue(IMathsVisitor& visitor)
		{
#ifdef _DEBUG
			visitor.ShowString("Renderer", "DirectX 12.0 Rococo MPLAT - Debug");
#else
			visitor.ShowString("Renderer", "DirectX 12.0 Rococo MPLAT - Release");
#endif
			visitor.ShowString("Screen Span", "%d x %d pixels", screenSpan.x, screenSpan.y);
			visitor.ShowString("Last error", "%s", *lastError ? lastError : "- none -");

			double hz = (double)Time::TickHz();

			double ticks_to_ms = 1000.0 / hz;

			visitor.ShowString("Frame Profiles", "---------------");
			visitor.ShowString("AI+Logic Time", "%3.0lf ms", AIcost * ticks_to_ms);
			visitor.ShowString("UI Render Time", "%3.0lf ms", guiCost * ticks_to_ms);
			visitor.ShowString("3D Render Time", "%3.0lf ms", objCost * ticks_to_ms);
			visitor.ShowString("Present Cost", "%3.0lf ms", presentCost * ticks_to_ms);
			visitor.ShowString("Total Frame Time", "%3.0lf ms", frameTime * ticks_to_ms);
			visitor.ShowString("Frame Rate", "%.0lf FPS", hz / frameTime);
			visitor.ShowString("", "");
			visitor.ShowString("Geometry this frame", "%lld triangles. %lld entities, %lld particles", trianglesThisFrame, entitiesThisFrame, plasma.size() + fog.size());
		}
	};
}

namespace Rococo::Graphics
{
	IDX12Renderer* CreateDX12Renderer(IInstallation& installation,
		DX12WindowInternalContext& ic,
		ITextureMemory& txMemory,
		IShaderCache& shaders,
		IPipelineBuilder& pipelineBuilder)
	{
		return new ANON::DX12Renderer(installation, ic, txMemory, shaders, pipelineBuilder);
	}
}