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
#include <rococo.renderer.h>
#include <rococo.fonts.h>
#include <rococo.fonts.hq.h>
#include <rococo.textures.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Textures;

namespace ANON
{
	class DX12Renderer: public IDX12Renderer, private IRenderer
	{
	private:
		DX12WindowInternalContext& ic;
		IShaderCache& shaders;
		IPipelineBuilder& pipelineBuilder;
		IDX12RendererWindow* window = nullptr;
		IInstallation& installation;
	public:
		DX12Renderer(IInstallation& ref_installation,
			DX12WindowInternalContext& ref_ic, 
			IShaderCache& ref_shaders, 
			IPipelineBuilder& ref_pipelineBuilder)
			:
			installation(ref_installation), 
			ic(ref_ic), 
			shaders(ref_shaders),
			pipelineBuilder(ref_pipelineBuilder)
		{

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

		void SetTargetWindow(IDX12RendererWindow* window)
		{
			this->window = window;
		}

		ID_FONT CreateOSFont(Fonts::IArrayFontSet& glyphs, const Fonts::FontSpec& spec) override
		{
			return ID_FONT::Invalid();
		}

		void AddOverlay(int zorder, IUIOverlay* overlay) override
		{

		}

		void AddFog(const ParticleVertex& fog) override
		{

		}

		void AddPlasma(const ParticleVertex& p) override
		{

		}

		ID_TEXTURE CreateRenderTarget(int32 width, int32 height) override
		{
			return ID_TEXTURE::Invalid();
		}

		void CaptureMouse(bool enable) override
		{

		}

		void ClearMeshes() override
		{

		}

		void ClearFog() override
		{

		}

		void ClearPlasma() override
		{

		}

		ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) override
		{
			return ID_SYS_MESH::Invalid();
		}

		ID_CUBE_TEXTURE CreateCubeTexture(cstr path, cstr extension) override
		{
			return ID_CUBE_TEXTURE::Invalid();
		}

		void DeleteMesh(ID_SYS_MESH id) override
		{

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

		void SetSysCursor(EWindowCursor id) override
		{

		}

		void GetGuiMetrics(GuiMetrics& metrics) const override
		{

		}

		void GetMaterialArrayMetrics(MaterialArrayMetrics& metrics) const override
		{

		}

		void GetMeshDesc(char desc[256], ID_SYS_MESH id) override
		{

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

		}

		MaterialId GetMaterialId(cstr name) const override
		{
			return 0;
		}

		cstr GetMaterialTextureName(MaterialId id) const override
		{
			return nullptr;
		}

		ID_TEXTURE LoadTexture(IBuffer& rawImageBuffer, cstr uniqueName) override
		{
			return ID_TEXTURE::Invalid();
		}

		AutoFree<ITextureArrayBuilderSupervisor> spriteArrayBuilder;

		Textures::ITextureArrayBuilder& SpriteBuilder() override
		{
			return *spriteArrayBuilder;
		}

		void OnSize(Vec2i span) override
		{

		}

		void Render(Graphics::ENVIRONMENTAL_MAP EnvironmentalMap, IScene& scene) override
		{

		}

		void RemoveOverlay(IUIOverlay* overlay) override
		{

		}

		void SetCursorBitmap(const Textures::BitmapLocation& sprite, Vec2i hotspotOffset) override
		{

		}

		void SetCursorVisibility(bool isVisible) override
		{

		}

		void SetSampler(uint32 index, Samplers::Filter, Samplers::AddressMode u, Samplers::AddressMode v, Samplers::AddressMode w, const RGBA& borderColour) override
		{

		}

		void SetShadowCasting(ID_SYS_MESH id, boolean32 isActive) override
		{

		}

		void SetSpecialAmbientShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) override
		{

		}

		void SetSpecialSpotlightShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) override
		{

		}

		void ShowWindowVenue(IMathsVisitor& visitor) override
		{

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

		void UpdateMesh(ID_SYS_MESH rendererId, const ObjectVertex* vertices, uint32 nVerticess, const BoneWeights* weights) override
		{

		}

		void UpdatePixelShader(cstr pingPath) override
		{

		}

		void UpdateVertexShader(cstr pingPath) override
		{

		}

		Rococo::Windows::IWindow& Window() override
		{
			if (window)
			{
				return window->Window();
			}
			else
			{
				return Rococo::Windows::NoParent();
			}
		}

		IMathsVenue* Venue() override
		{
			return nullptr;
		}

		enum { ID_FONT_OSFONT_OFFSET = 400 };

		struct OSFont
		{
			Fonts::IArrayFontSupervisor* arrayFont;
			Fonts::FontSpec spec;
		};

		std::vector<OSFont> osFonts;

		Fonts::ArrayFontMetrics GetFontMetrics(ID_FONT idFont) override
		{
			int32 index = idFont.value - ID_FONT_OSFONT_OFFSET;
			if (index < 0 || index >= (int32)osFonts.size())
			{
				Throw(0, "DX11Renderer.GetFontMetrics - ID_FONT parameter was unknown value");
			}

			return osFonts[index].arrayFont->Metrics();
		}

	};
}

namespace Rococo::Graphics
{
	IDX12Renderer* CreateDX12Renderer(IInstallation& installation,
		DX12WindowInternalContext& ic,
		IShaderCache& shaders,
		IPipelineBuilder& pipelineBuilder)
	{
		return new ANON::DX12Renderer(installation, ic, shaders, pipelineBuilder);
	}
}