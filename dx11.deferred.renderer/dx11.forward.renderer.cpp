#include <rococo.api.h>
#include <rococo.os.win32.h>
#include <rococo.libs.inl>
#include <dxgi.h>
#include <rococo.window.h>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>
#include <vector>
#include <rococo.mplat.h>
#include <rococo.fonts.hq.h>
#include <rococo.DirectX.h>

using namespace Rococo;
using namespace Rococo::Graphics;

ROCOCOAPI IDX11ForwardRenderer
{
	virtual IRenderer& Renderer() = 0;
};

namespace ANON
{
	class ForwardRenderer : public IDX11ForwardRenderer, public IRenderer
	{
	public:
		ForwardRenderer()
		{

		}

		const Fonts::ArrayFontMetrics& GetFontMetrics(ID_FONT idFont) override
		{
			const Fonts::ArrayFontMetrics* metrics = nullptr;
			return *metrics;
		}

		void GetGuiMetrics(GuiMetrics& metrics) const override
		{

		}

		void GetMaterialArrayMetrics(MaterialArrayMetrics& metrics) const override
		{

		}

		Textures::ITextureArrayBuilder& SpriteBuilder() override
		{
			Textures::ITextureArrayBuilder* builder = nullptr;
			return *builder;
		}

		Fonts::IFont& FontMetrics() override
		{
			Fonts::IFont* font = nullptr;
			return *font;
		}

		IHQFontResource& HQFontsResources() override
		{
			IHQFontResource* hq = nullptr;
			return *hq;
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

		ID_TEXTURE CreateRenderTarget(int32 width, int32 height)  override
		{
			return ID_TEXTURE::Invalid();
		}

		void CaptureMouse(bool enable)  override
		{

		}

		void ClearMeshes()  override
		{

		}

		void ClearFog()  override
		{

		}

		void ClearPlasma()  override
		{

		}

		ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) override
		{
			return ID_SYS_MESH();
		}

		ID_CUBE_TEXTURE CreateCubeTexture(cstr path, cstr extension)  override
		{
			return ID_CUBE_TEXTURE();
		}

		void DeleteMesh(ID_SYS_MESH id)  override
		{

		}

		ID_TEXTURE FindTexture(cstr name) const  override
		{
			return ID_TEXTURE();
		}

		void SetSysCursor(EWindowCursor id)  override
		{

		}

		void GetMeshDesc(char desc[256], ID_SYS_MESH id)  override
		{

		}

		bool TryGetTextureDesc(TextureDesc& desc, ID_TEXTURE id) const  override
		{
			return false;
		}

		IInstallation& Installation()  override
		{
			IInstallation* installation = nullptr;
			return *installation;
		}

		void LoadMaterialTextureArray(IMaterialTextureArrayBuilder& builder)  override
		{

		}

		MaterialId GetMaterialId(cstr name) const  override
		{
			return MaterialId();
		}

		cstr GetMaterialTextureName(MaterialId id) const  override
		{
			return nullptr;
		}

		ID_TEXTURE LoadTexture(IBuffer& rawImageBuffer, cstr uniqueName)  override
		{
			return ID_TEXTURE();
		}

		void OnSize(Vec2i span)  override
		{

		}

		void Render(Graphics::ENVIRONMENTAL_MAP EnvironmentalMap, IScene& scene)  override
		{

		}

		void RemoveOverlay(IUIOverlay* overlay)  override
		{

		}

		void SetCursorBitmap(const Textures::BitmapLocation& sprite, Vec2i hotspotOffset)  override
		{

		}

		void SetCursorVisibility(bool isVisible)  override
		{

		}

		void SetSampler(uint32 index, Samplers::Filter, Samplers::AddressMode u, Samplers::AddressMode v, Samplers::AddressMode w, const RGBA& borderColour)  override
		{

		}

		void SetShadowCasting(ID_SYS_MESH id, boolean32 isActive)  override
		{

		}

		void SetSpecialAmbientShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending)  override
		{

		}

		void SetSpecialSpotlightShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending)  override
		{

		}

		void ShowWindowVenue(IMathsVisitor& visitor)  override
		{

		}

		void SyncCubeTexture(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace)  override
		{

		}

		void SwitchToWindowMode()  override
		{

		}

		IMathsVenue* TextureVenue()  override
		{
			return nullptr;
		}

		ID_TEXTURE LoadAlphaTextureArray(cstr uniqueName, Vec2i span, int nElements, ITextureLoadEnumerator& enumerator)  override
		{
			return ID_TEXTURE();
		}

		void UpdateMesh(ID_SYS_MESH rendererId, const ObjectVertex* vertices, uint32 nVerticess, const BoneWeights* weights) override
		{

		}

		void UpdatePixelShader(cstr pingPath)  override
		{

		}

		void UpdateVertexShader(cstr pingPath)  override
		{

		}

		Windows::IWindow& Window()  override
		{
			Windows::IWindow* window = nullptr;
			return *window;
		}

		IMathsVenue* Venue()  override
		{
			return nullptr;
		}

		IRenderer& Renderer() override
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Graphics
{
	IDX11ForwardRenderer* CreateForwardRenderer()
	{
		return new ANON::ForwardRenderer();
	}
}