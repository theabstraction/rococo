#include "dx11.renderer.h"
#include <rococo.renderer.h>

#include <vector>
#include <algorithm>

#include <rococo.imaging.h>
#include <rococo.fonts.h>
#include <rococo.maths.h>
#include <rococo.io.h>

#include <rococo.hashtable.h>

#include <rococo.dx11.renderer.win32.h>

#include "dx11helpers.inl"
#include "dx11buffers.inl"

#include "rococo.visitors.h"

#include <random>

#include <Dxgi1_3.h>
#include <comdef.h>

#include "dx11.factory.h"

#include <memory>

#include <rococo.fonts.hq.h>

namespace Rococo::DX11
{
	void ShowWindowVenue(HWND hWnd, IMathsVisitor& visitor);
}

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Fonts;
	using namespace Rococo::Windows;
	using namespace Rococo::Samplers;
	using namespace Rococo::DX11;

	struct DX11Shader
	{
		std::string name;
	};

	struct AmbientData
	{
		RGBA localLight;
		float fogConstant = -0.2218f; // light = e(Rk). Where R is distance. k = -0.2218 gives modulation of 1/256 at 25 metres, reducing full brightness to dark
		float a = 0;
		float b = 0;
		float c = 0;
	};

	struct DX11VertexShader : public DX11Shader
	{
		AutoRelease<ID3D11InputLayout> inputLayout;
		AutoRelease<ID3D11VertexShader> vs;
	};

	struct DX11GeometryShader : public DX11Shader
	{
		AutoRelease<ID3D11InputLayout> inputLayout;
		AutoRelease<ID3D11GeometryShader> gs;
	};

	struct DX11PixelShader : public DX11Shader
	{
		AutoRelease<ID3D11PixelShader> ps;
	};

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

   using namespace Rococo::Textures;

   class DX11AppRenderer :
	   public IRenderer,
	   public IRenderContext,
	   public IMathsVenue,
	   public IDX11ResourceLoader,
	   public IShaderStateControl
   {
   private:
	   std::default_random_engine rng;
	   OS::ticks lastTick;
	   ID3D11Device& device;
	   ID3D11DeviceContext& dc;
	   IDXGIFactory& factory;
	   IInstallation& installation;
	   int64 trianglesThisFrame = 0;
	   int64 entitiesThisFrame = 0;
	   std::unordered_map<ID_TEXTURE, IDX11TextureArray*, ID_TEXTURE> genericTextureArray;
	   stringmap<ID_TEXTURE> nameToGenericTextureId;

	   AutoRelease<IDXGISwapChain> mainSwapChain;
	   AutoRelease<ID3D11RenderTargetView> mainBackBufferView;

	   ID_TEXTURE mainDepthBufferId;
	   ID_TEXTURE shadowBufferId;

	   std::vector<DX11VertexShader*> vertexShaders;
	   std::vector<DX11PixelShader*> pixelShaders;
	   std::vector<DX11GeometryShader*> geometryShaders;

	   AutoRelease<ID3D11Buffer> particleBuffer;
	   AutoRelease<ID3D11Buffer> gui3DBuffer;

	   AutoFree<IDX11Gui> gui;

	   enum { PARTICLE_BUFFER_VERTEX_CAPACITY = 1024 };
	   enum { GUI3D_BUFFER_TRIANGLE_CAPACITY = 1024 };
	   enum { GUI3D_BUFFER_VERTEX_CAPACITY = 3 * GUI3D_BUFFER_TRIANGLE_CAPACITY };

	   AutoRelease<ID3D11Texture2D> fontTexture;
	   AutoRelease<ID3D11ShaderResourceView> fontBinding;

	   AutoRelease<ID3D11RasterizerState> spriteRasterizering;
	   AutoRelease<ID3D11RasterizerState> objectRasterizering;
	   AutoRelease<ID3D11RasterizerState> particleRasterizering;
	   AutoRelease<ID3D11RasterizerState> skyRasterizering;
	   AutoRelease<ID3D11RasterizerState> shadowRasterizering;

	   AutoRelease<ID3D11BlendState> alphaAdditiveBlend;
	   AutoRelease<ID3D11BlendState> disableBlend;
	   AutoRelease<ID3D11BlendState> additiveBlend;
	   AutoRelease<ID3D11BlendState> plasmaBlend;

	   AutoRelease<ID3D11Buffer> globalStateBuffer;
	   AutoRelease<ID3D11Buffer> depthRenderStateBuffer;
	   AutoRelease<ID3D11Buffer> lightStateBuffer;
	   AutoRelease<ID3D11Buffer> textureDescBuffer;
	   AutoRelease<ID3D11Buffer> ambientBuffer;
	   AutoRelease<ID3D11Buffer> sunlightStateBuffer;

	   BoneMatrices boneMatrices = { 0 };
	   AutoRelease<ID3D11Buffer> boneMatricesStateBuffer;

	   AutoRelease<ID3D11Buffer> lightConeBuffer;

	   AutoFree<IDX11Materials> materials;

	   RAWMOUSE lastMouseEvent;
	   Vec2i screenSpan;

	   ID_VERTEX_SHADER idObjVS;
	   ID_PIXEL_SHADER idObjPS;
	   ID_PIXEL_SHADER idLightConePS;

	   ID_VERTEX_SHADER idLightConeVS;
	   ID_VERTEX_SHADER idObjAmbientVS;
	   ID_PIXEL_SHADER idObjAmbientPS;

	   ID_PIXEL_SHADER idObj_Spotlight_NoEnvMap_PS;
	   ID_PIXEL_SHADER idObj_Ambient_NoEnvMap_PS;

	   ID_VERTEX_SHADER idParticleVS;
	   ID_PIXEL_SHADER idPlasmaPS;
	   ID_PIXEL_SHADER idFogAmbientPS;
	   ID_PIXEL_SHADER idFogSpotlightPS;
	   ID_GEOMETRY_SHADER idPlasmaGS;
	   ID_GEOMETRY_SHADER idFogSpotlightGS;
	   ID_GEOMETRY_SHADER idFogAmbientGS;

	   ID_VERTEX_SHADER idObjVS_Shadows;
	   ID_VERTEX_SHADER idSkinnedObjVS_Shadows;
	   ID_PIXEL_SHADER idObjPS_Shadows;

	   ID_VERTEX_SHADER idObjSkyVS;
	   ID_PIXEL_SHADER idObjSkyPS;

	   AutoRelease<ID3D11Buffer> instanceBuffer;

	   AutoRelease<ID3D11BlendState> alphaBlend;

	   AutoRelease<ID3D11DepthStencilState> objDepthState;
	   AutoRelease<ID3D11DepthStencilState> objDepthState_NoWrite;
	   AutoRelease<ID3D11DepthStencilState> noDepthTestOrWrite;

	   AutoRelease<ID3D11ShaderResourceView> envMap;

	   std::vector<DX11::TextureBind> textures;
	   stringmap<ID_TEXTURE> mapNameToTexture;

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

	   ID_TEXTURE lastTextureId;

	   AutoFree<IExpandingBuffer> scratchBuffer;
	   DX11::TextureLoader textureLoader;

	   void Load(cstr name, IEventCallback<CompressedTextureBuffer>& onLoad) override
	   {
		   StandardLoadFromCompressedTextureBuffer(name, onLoad, installation, *scratchBuffer);
	   }

	   void LoadTextFile(cstr pingPath, Rococo::Function<void(const fstring& text)> callback)
	   {
		   installation.LoadResource(pingPath, *scratchBuffer, 256_kilobytes);
		   fstring text{ (cstr) scratchBuffer->GetData(), (int32) scratchBuffer->Length() };
		   callback(text);
	   }

	   ID_TEXTURE LoadAlphaTextureArray(cstr uniqueName, Vec2i span, int32 nElements, ITextureLoadEnumerator& enumerator) override
	   {
			auto i = nameToGenericTextureId.find(uniqueName);
			if (i != nameToGenericTextureId.end()) return i->second;
		   
			IDX11TextureArray* array = DX11::LoadAlphaTextureArray(device, span, nElements, enumerator, dc);

			auto id = ID_TEXTURE{ (size_t)array | 0x8000000000000000LL };
			nameToGenericTextureId[uniqueName] = id;
			genericTextureArray[id] = array;
			return id;
	   }

	   void SetBoneMatrix(uint32 index, cr_m4x4 m) override
	   {
		   if (index >= BoneMatrices::BONE_MATRIX_CAPACITY)
		   {
			   Throw(0, "Bad bone index #%u", index);
		   }

		   auto& target = boneMatrices.bones[index];
		   target = m;
		   target.row3 = Vec4{ 0, 0, 0, 1.0f };
	   }

	   void SetGenericTextureArray(ID_TEXTURE id)
	   {
		   auto i = genericTextureArray.find(id);
		   if (i != genericTextureArray.end())
		   {
			   ID3D11ShaderResourceView* gtaViews[1] = { i->second->View() };
			   dc.PSSetShaderResources(TXUNIT_GENERIC_TXARRAY, 1, gtaViews);
		   }
	   }

	   stringmap<ID_PIXEL_SHADER> nameToPixelShader;
	   stringmap<ID_VERTEX_SHADER> nameToVertexShader;

	   std::vector<ParticleVertex> fog;
	   std::vector<ParticleVertex> plasma;

	   ID3D11SamplerState* samplers[16] = { 0 };
	   AutoRelease<ID3D11SamplerState> skySampler;

	   ID_SYS_MESH skyMeshId;

	   AutoFree<IDX11CubeTextures> cubeTextures;
   public:
	   Windows::IWindow& window;
	   bool isBuildingAlphaBlendedSprites{ false };

	   DX11AppRenderer(DX11::Factory& _factory, Windows::IWindow& _window) :
		   device(_factory.device), dc(_factory.dc), factory(_factory.factory),
		   installation(_factory.installation), materials(CreateMaterials(_factory.installation, _factory.device, _factory.dc)),
		   scratchBuffer(CreateExpandingBuffer(64_kilobytes)),
		   textureLoader(_factory.installation, _factory.device, _factory.dc, *scratchBuffer),
		   window(_window)
	   {
		   particleBuffer = DX11::CreateDynamicVertexBuffer<ParticleVertex>(device, PARTICLE_BUFFER_VERTEX_CAPACITY);
		   gui3DBuffer = DX11::CreateDynamicVertexBuffer<ObjectVertex>(device, GUI3D_BUFFER_VERTEX_CAPACITY);

		   objDepthState = DX11::CreateObjectDepthStencilState(device);
		   objDepthState_NoWrite = DX11::CreateObjectDepthStencilState_NoWrite(device);
		   noDepthTestOrWrite = DX11::CreateNoDepthCheckOrWrite(device);

		   spriteRasterizering = DX11::CreateSpriteRasterizer(device);
		   objectRasterizering = DX11::CreateObjectRasterizer(device);
		   particleRasterizering = DX11::CreateParticleRasterizer(device);
		   skyRasterizering = DX11::CreateSkyRasterizer(device);
		   shadowRasterizering = DX11::CreateShadowRasterizer(device);

		   alphaBlend = DX11::CreateAlphaBlend(device);
		   alphaAdditiveBlend = DX11::CreateAlphaAdditiveBlend(device);
		   disableBlend = DX11::CreateNoBlend(device);
		   additiveBlend = DX11::CreateAdditiveBlend(device);
		   plasmaBlend = DX11::CreatePlasmaBlend(device);

		   DX11::TextureBind fb = textureLoader.LoadAlphaBitmap("!font1.tif");
		   fontTexture = fb.texture;
		   fontBinding = fb.shaderView;

		   globalStateBuffer = DX11::CreateConstantBuffer<GlobalState>(device);
		   depthRenderStateBuffer = DX11::CreateConstantBuffer<DepthRenderData>(device);
		   lightStateBuffer = DX11::CreateConstantBuffer<Light>(device);
		   sunlightStateBuffer = DX11::CreateConstantBuffer<Vec4>(device);
		   boneMatricesStateBuffer = DX11::CreateConstantBuffer<BoneMatrices>(device);
		   textureDescBuffer = DX11::CreateConstantBuffer<TextureDescState>(device);
		   ambientBuffer = DX11::CreateConstantBuffer<AmbientData>(device);

		   gui = CreateDX11Gui(*this, *this, *this, device, dc);
		   
		   idObjVS = CreateObjectVertexShader("!object.vs");
		   idObjPS = CreatePixelShader("!object.ps");
		   idParticleVS = CreateParticleVertexShader("!particle.vs");
		   idPlasmaGS = CreateGeometryShader("!plasma.gs");
		   idFogSpotlightGS = CreateGeometryShader("!fog.spotlight.gs");
		   idFogAmbientGS = CreateGeometryShader("!fog.ambient.gs");
		   idObj_Spotlight_NoEnvMap_PS = CreatePixelShader("!obj.spotlight.no_env.ps");
		   idObj_Ambient_NoEnvMap_PS = CreatePixelShader("!obj.ambient.no_env.ps");
		   idPlasmaPS = CreatePixelShader("!plasma.ps");
		   idFogSpotlightPS = CreatePixelShader("!fog.spotlight.ps");
		   idFogAmbientPS = CreatePixelShader("!fog.ambient.ps");
		   idObjAmbientPS = CreatePixelShader("!ambient.ps");
		   idObjAmbientVS = CreateObjectVertexShader("!ambient.vs");
		   idObjVS_Shadows = CreateObjectVertexShader("!shadow.vs");
		   idSkinnedObjVS_Shadows = CreateVertexShader("!skinned.shadow.vs", DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
		   idObjPS_Shadows = CreatePixelShader("!shadow.ps");
		   idLightConePS = CreatePixelShader("!light_cone.ps");
		   idLightConeVS = CreateVertexShader("!light_cone.vs", DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
		   idObjSkyVS = CreateVertexShader("!skybox.vs", DX11::GetSkyVertexDesc(), DX11::NumberOfSkyVertexElements());
		   idObjSkyPS = CreatePixelShader("!skybox.ps");

		   instanceBuffer = DX11::CreateConstantBuffer<ObjectInstance>(device);

		   lastTick = OS::CpuTicks();
		   rng.seed(123456U);

		   cubeTextures = CreateCubeTextureManager(device, dc);

		   D3D11_SAMPLER_DESC desc;
		   GetSkySampler(desc);
		   VALIDATEDX11(device.CreateSamplerState(&desc, &skySampler));

		   SkyVertex topNW{ -1.0f, 1.0f, 1.0f };
		   SkyVertex topNE{  1.0f, 1.0f, 1.0f };
		   SkyVertex topSW{ -1.0f,-1.0f, 1.0f };
		   SkyVertex topSE{  1.0f,-1.0f, 1.0f };
		   SkyVertex botNW{ -1.0f, 1.0f,-1.0f };
		   SkyVertex botNE{  1.0f, 1.0f,-1.0f };
		   SkyVertex botSW{ -1.0f,-1.0f,-1.0f };
		   SkyVertex botSE{  1.0f,-1.0f,-1.0f };

		   SkyVertex skyboxVertices[36] =
		   {
			   topSW, topNW, topNE, // top,
			   topNE, topSE, topSW, // top,
			   botSW, botNW, botNE, // bottom,
			   botNE, botSE, botSW, // bottom,
			   topNW, topSW, botSW, // West
			   botSW, botNW, topNW, // West
			   topNE, topSE, botSE, // East
			   botSE, botNE, topNE, // East
			   topNW, topNE, botNE, // North
			   botNE, botNW, topNW, // North
			   topSW, topSE, botSE, // South
			   botSE, botSW, topSW, // South
		   };
		   skyMeshId = CreateSkyMesh(skyboxVertices, sizeof(skyboxVertices) / sizeof(SkyVertex));

		   RGBA red{ 1.0f, 0, 0, 1.0f };
		   RGBA transparent{ 0.0f, 0, 0, 0.0f };

		   SetSampler(TXUNIT_FONT,	 Filter_Linear, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		   SetSampler(TXUNIT_SHADOW, Filter_Linear, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		   SetSampler(TXUNIT_ENV_MAP,   Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		   SetSampler(TXUNIT_SELECT, Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		   SetSampler(TXUNIT_MATERIALS, Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		   SetSampler(TXUNIT_SPRITES,   Filter_Point, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		   SetSampler(TXUNIT_GENERIC_TXARRAY, Filter_Point, AddressMode_Border, AddressMode_Border, AddressMode_Border, transparent);

		   lightConeBuffer = DX11::CreateDynamicVertexBuffer<ObjectVertex>(device, 3);
	   }

	   ~DX11AppRenderer()
	   {
		   DetachContext();
		   ClearMeshes();

		   for (auto& x : vertexShaders)
		   {
			   delete x;
		   }

		   for (auto& x : pixelShaders)
		   {
			   delete x;
		   }

		   for (auto& x : geometryShaders)
		   {
			   delete x;
		   }

		   for (auto& t : textures)
		   {
			   if (t.texture) t.texture->Release();
			   if (t.shaderView) t.shaderView->Release();
			   if (t.renderView) t.renderView->Release();
			   if (t.depthView) t.depthView->Release();
		   }

		   for (auto& s : samplers)
		   {
			   if (s) s->Release();
		   }

		   for (auto& t : genericTextureArray)
		   {
			   t.second->Free();
		   }
	   }

	   IGuiResources& Gui()
	   {
		   return gui->Gui();
	   }

	   IMaterials& Materials()
	   {
		   return *materials;
	   }

	   void AddFog(const ParticleVertex& p) override
	   {
		   fog.push_back(p);
	   }

	   void AddPlasma(const ParticleVertex& p) override
	   {
		   plasma.push_back(p);
	   }

	   void CaptureMouse(bool enable) override
	   {
		   if (enable) SetCapture(this->window);
		   else ReleaseCapture();
	   }

	   void ClearPlasma() override
	   {
		   plasma.clear();
	   }

	   void ClearFog() override
	   {
		   fog.clear();
	   }

	   EWindowCursor cursorId = EWindowCursor_Default;

	   void SetSysCursor(EWindowCursor id) override
	   {
		   cursorId = id;
	   }

	   void SetSampler(uint32 index, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour) override
	   {
		   if (samplers[index])
		   {
			   samplers[index]->Release();
			   samplers[index] = nullptr;
		   }

		   auto* sampler = Rococo::DX11::GetSampler(device, index, filter, u, v, w, borderColour);
		   samplers[index] = sampler;
	   }

	   ID_CUBE_TEXTURE CreateCubeTexture(cstr path, cstr extension)
	   {
		   return cubeTextures->CreateCubeTexture(textureLoader, path, extension);
	   }

	   void SyncCubeTexture(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace) override
	   {
		   cubeTextures->SyncCubeTexture(XMaxFace, XMinFace, YMaxFace, YMinFace, ZMaxFace, ZMinFace, materials->Textures());
	   }

	   struct : IMathsVenue
	   {
		   DX11AppRenderer* This;
		   virtual void ShowVenue(IMathsVisitor& visitor)
		   {
			   This->ShowTextureVenue(visitor);
		   }
	   } textureVenue;

	   virtual IMathsVenue* TextureVenue()
	   {
		   textureVenue.This = this;
		   return &textureVenue;
	   }

	   bool TryGetTextureDesc(TextureDesc& desc, ID_TEXTURE id) const
	   {
		   size_t index = id.value - 1;
		   if (index < 0 || index >= textures.size())
		   {
			   return false;
		   }

		   const auto& t = textures[index];
		   if (!t.texture) return false;
		   GetTextureDesc(desc, *t.texture);
		   return true;
	   }

	   struct TextureItem
	   {
		   ID_TEXTURE id;
		   std::string name;

		   bool operator < (const TextureItem& other)
		   {
			   return name < other.name;
		   }
	   };

	   std::vector<TextureItem> orderedTextureList;

	   void ShowTextureVenue(IMathsVisitor& visitor)
	   {
		   if (orderedTextureList.empty())
		   {
			   for (auto& t : mapNameToTexture)
			   {
				   orderedTextureList.push_back({ t.second, t.first });
			   }

			   std::sort(orderedTextureList.begin(), orderedTextureList.end());
		   }

		   for (auto& t : orderedTextureList)
		   {
			   auto& tx = textures[t.id.value-1];

			   D3D11_TEXTURE2D_DESC desc;
			   tx.texture->GetDesc(&desc);
			   char name[64];
			   SafeFormat(name, 64, "TxId %u", t.id.value);
			   visitor.ShowSelectableString("overlay.select.texture", name, "  %s - 0x%p. %d x %d. %d levels", t.name.c_str(), (const void*) tx.texture, desc.Width, desc.Height, desc.MipLevels);
		   }

		   materials->ShowVenue(visitor);
	   }

	   void GetMeshDesc(char desc[256], ID_SYS_MESH id) override
	   {
		   if (!id || id.value >= meshBuffers.size())
		   {
			   SafeFormat(desc, 256, "invalid id");
		   }
		   else
		   {
			   auto& m = meshBuffers[id.value];

			   if (m.vertexBuffer)
			   {
				   UINT byteWidth = 0;
				   D3D11_BUFFER_DESC bdesc;
				   m.vertexBuffer->GetDesc(&bdesc);
				   byteWidth += bdesc.ByteWidth;

				   if (m.weightsBuffer)
				   {
					   m.weightsBuffer->GetDesc(&bdesc);
					   byteWidth += bdesc.ByteWidth;
				   }
				   SafeFormat(desc, 256, " %p %6d %svertices. %6u bytes", m.vertexBuffer, m.numberOfVertices, m.weightsBuffer != nullptr ? "(weighted) " : "", byteWidth);
			   }
			   else
			   {
				   SafeFormat(desc, 256, "Null object");
			   }
		   }
	   }

	   void SetShadowCasting(ID_SYS_MESH id, boolean32 isActive)
	   {
		   if (!id || id.value >= meshBuffers.size())
		   {
			   Throw(0, "DX11ApRenderer::SetShadowCasting(...): unknown id");
		   }
		   else
		   {
			   auto& m = meshBuffers[id.value];
			   m.disableShadowCasting = !isActive;
		   }
	   }

	   virtual void ShowVenue(IMathsVisitor& visitor)
	   {
#ifdef _DEBUG
		   visitor.ShowString("Renderer", "DirectX 11.1 Rococo MPLAT - Debug");
#else
		   visitor.ShowString("Renderer", "DirectX 11.1 Rococo MPLAT - Release");
#endif
		   visitor.ShowString("Screen Span", "%d x %d pixels", screenSpan.x, screenSpan.y);
		   visitor.ShowString("Last error", "%s", *lastError ? lastError : "- none -");

		   ShowVenueForDevice(visitor, device);

		   D3D11_RENDER_TARGET_VIEW_DESC desc;
		   mainBackBufferView->GetDesc(&desc);

		   visitor.ShowString("BackBuffer format", "%u", desc.Format);

		   D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc;
		   GetTexture(mainDepthBufferId).depthView->GetDesc(&dsDesc);

		   visitor.ShowString("DepthStencil format", "%u", dsDesc.Format);

		   visitor.ShowDecimal("Number of textures", (int64)textures.size());
		   visitor.ShowDecimal("Number of meshes", (int64)meshBuffers.size());
		   visitor.ShowDecimal("Mesh updates", meshUpdateCount);

		   gui->ShowVenue(visitor);

		   double hz = (double)OS::CpuHz();

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

	   IMathsVenue* Venue()
	   {
		   return this;
	   }

	   void ShowWindowVenue(IMathsVisitor& visitor)
	   {
		   visitor.ShowString("IsFullScreen", isFullScreen ? "TRUE" : "FALSE");
		   DX11::ShowWindowVenue(window, visitor);
	   }

	   void UpdatePixelShader(cstr pingPath) override
	   {
		   for (auto& i : pixelShaders)
		   {
			   if (Eq(pingPath, i->name.c_str()))
			   {
				   if (!i->ps)
				   {
					   lastError[0] = 0;
				   }

				   i->ps = nullptr;
				   installation.LoadResource(pingPath, *scratchBuffer, 64_kilobytes);
				   HRESULT hr = device.CreatePixelShader(scratchBuffer->GetData(), scratchBuffer->Length(), nullptr, &i->ps);
				   if FAILED(hr)
				   {
					   SafeFormat(lastError, "device.CreatePixelShader for %s returned 0x%X", pingPath, hr);
				   }
				   break;
			   }
		   }
	   }

	   char lastError[256] = { 0 };

	   void UpdateVertexShader(cstr pingPath) override
	   {
		   for (auto& i : vertexShaders)
		   {
			   if (Eq(pingPath, i->name.c_str()))
			   {
				   if (!i->vs)
				   {
					   lastError[0] = 0;
				   }

				   i->vs = nullptr;
				   i->inputLayout = nullptr;
				   installation.LoadResource(pingPath, *scratchBuffer, 64_kilobytes);

				   const D3D11_INPUT_ELEMENT_DESC *elements = nullptr;
				   uint32 nElements;

				   if (Eq(pingPath, "!gui.vs"))
				   {
					   elements = DX11::GetGuiVertexDesc();
					   nElements = DX11::NumberOfGuiVertexElements();
				   }
				   else
				   {
					   elements = DX11::GetObjectVertexDesc();
					   nElements = DX11::NumberOfObjectVertexElements();
				   }

				   HRESULT hr = device.CreateInputLayout(elements, nElements, scratchBuffer->GetData(), scratchBuffer->Length(), &i->inputLayout);
				   if SUCCEEDED(hr)
				   {
					   hr = device.CreateVertexShader(scratchBuffer->GetData(), scratchBuffer->Length(), nullptr, &i->vs);
					   if FAILED(hr)
					   {
						   SafeFormat(lastError, sizeof(lastError), "device.CreateVertexShader for %s returned 0x%X", pingPath, hr);
						   i->inputLayout = nullptr;
					   }
				   }
				   else
				   {
					   i->inputLayout = nullptr;
					   SafeFormat(lastError, sizeof(lastError), "device.CreateInputLayout for %s returned 0x%X", pingPath, hr);
				   }
				   break;
			   }
		   }
	   }

	   void Free()
	   {
		   delete this;
	   }

	   void ClearMeshes() override
	   {
		   for (auto& x : meshBuffers)
		   {
			   if (x.vertexBuffer) x.vertexBuffer->Release();
			   if (x.weightsBuffer) x.weightsBuffer->Release();
		   }

		   meshBuffers.clear();
	   }

	   void DeleteMesh(ID_SYS_MESH id) override
	   {
		   if (id.value < 0 || id.value >= meshBuffers.size()) Throw(0, "DX11AppRenderer::DeleteMesh(...): Bad ID_SYS_MESH");
		  
		   meshBuffers[id.value].numberOfVertices = 0;

		   if (meshBuffers[id.value].vertexBuffer)
		   {
			   meshBuffers[id.value].vertexBuffer->Release();
			   meshBuffers[id.value].vertexBuffer = nullptr;
		   }

		   if (meshBuffers[id.value].weightsBuffer)
		   {
			   meshBuffers[id.value].weightsBuffer->Release();
			   meshBuffers[id.value].weightsBuffer = nullptr;
		   }
	   }

	   void BuildEnvironmentalMap(int32 topIndex, int32 bottomIndex, int32 leftIndex, int32 rightIndex, int frontIndex, int32 backIndex)
	   {
		   envMap = nullptr;
	   }

	   IInstallation& Installation() override
	   {
		   return installation;
	   }

	   virtual IRenderer& Renderer() override
	   {
		   return *this;
	   }

	   ID_TEXTURE LoadTexture(IBuffer& buffer, cstr uniqueName) override
	   {
		   auto i = mapNameToTexture.find(uniqueName);
		   if (i != mapNameToTexture.end())
		   {
			   return i->second;
		   }

		   auto bind = textureLoader.LoadColourBitmap(uniqueName);
		   textures.push_back(bind);

		   auto id = ID_TEXTURE(textures.size());
		   mapNameToTexture.insert(uniqueName, id);

		   orderedTextureList.clear();

		   return id;
	   }

	   ID_TEXTURE FindTexture(cstr name) const
	   {
		   auto i = mapNameToTexture.find(name);
		   return i != mapNameToTexture.end() ? i->second : ID_TEXTURE::Invalid();
	   }

	   Vec2i SelectTexture(ID_TEXTURE id) override
	   {
		   size_t index = id.value - 1;
		   if (index >= textures.size())
		   {
			   Throw(0, "Bad texture id");
		   }

		   auto& t = textures[index];

		   D3D11_TEXTURE2D_DESC desc;
		   t.texture->GetDesc(&desc);

		   D3D11_SHADER_RESOURCE_VIEW_DESC vdesc;
		   t.shaderView->GetDesc(&vdesc);

		   if (id != lastTextureId)
		   {
			   gui->FlushLayer();
			   lastTextureId = id;
			   dc.PSSetShaderResources(4, 1, &t.shaderView);

			   float dx = (float) desc.Width;
			   float dy = (float) desc.Height;

			   TextureDescState state;

			   if (vdesc.Format == DXGI_FORMAT_R32_FLOAT)
			   {
				   state =
				   {
						dx,
						dy,
						1.0f / dx,
						1.0f / dy,
						1.0f,
						0.0f,
						0.0f,
						0.0f
				   };
			   }
			   else
			   {
				   state =
				   {
					   dx,
					   dy,
					   1.0f / dx,
					   1.0f / dy,
					   1.0f,
					   1.0f,
					   1.0f,
					   1.0f
				   };
			   }

			   DX11::CopyStructureToBuffer(dc, textureDescBuffer, &state, sizeof(TextureDescState));
			   dc.PSSetConstantBuffers(CBUFFER_INDEX_SELECT_TEXTURE_DESC, 1, &textureDescBuffer);
		   }

		   return Vec2i{ (int32)desc.Width, (int32)desc.Height };
	   }

	   void ExpandViewportToEntireTexture(ID_TEXTURE depthId)
	   {
		   auto depth = GetTexture(depthId).texture;

		   D3D11_TEXTURE2D_DESC desc;
		   depth->GetDesc(&desc);

		   D3D11_VIEWPORT viewport = { 0 };
		   viewport.Width = FLOAT(desc.Width);
		   viewport.Height = FLOAT(desc.Height);
		   viewport.MinDepth = 0.0f;
		   viewport.MaxDepth = 1.0f;

		   dc.RSSetViewports(1, &viewport);

		   screenSpan.x = (int32)viewport.Width;
		   screenSpan.y = (int32)viewport.Height;
	   }

	   void BindMainWindow()
	   {
		   DXGI_SWAP_CHAIN_DESC swapChainDesc = DX11::GetSwapChainDescription(window);
		   VALIDATEDX11(factory.CreateSwapChain((ID3D11Device*)&device, &swapChainDesc, &mainSwapChain));

		   AutoRelease<ID3D11Texture2D> backBuffer;
		   VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

		   VALIDATEDX11(device.CreateRenderTargetView(backBuffer, nullptr, &mainBackBufferView));

		   RECT rect;
		   GetClientRect(window, &rect);

		   auto mainDepthBufferDesc = CreateDepthTarget(rect.right - rect.left, rect.bottom - rect.top);
		   textures.push_back(mainDepthBufferDesc);
		   this->mainDepthBufferId = ID_TEXTURE{ textures.size() };
		   mapNameToTexture[scdt_name] = mainDepthBufferId;

		   auto shadowBufferDesc = CreateDepthTarget(1024, 1024);
		   textures.push_back(shadowBufferDesc);

		   this->shadowBufferId = ID_TEXTURE{ textures.size() };
		   mapNameToTexture["ShadowBuffer"] = shadowBufferId;

		   ExpandViewportToEntireTexture(mainDepthBufferId);
	   }

	   ID_VERTEX_SHADER CreateVertexShader(cstr name, const byte* shaderCode, size_t shaderLength, const D3D11_INPUT_ELEMENT_DESC* vertexDesc, UINT nElements)
	   {
		   if (name == nullptr || rlen(name) > 1024) Throw(0, "Bad <name> for vertex shader");
		   if (shaderCode == nullptr || shaderLength < 4 || shaderLength > 65536) Throw(0, "Bad shader code for vertex shader %s", name);

		   DX11VertexShader* shader = new DX11VertexShader;
		   HRESULT hr;
		   
		   try
		   {
			   hr = device.CreateInputLayout(vertexDesc, nElements, shaderCode, shaderLength, &shader->inputLayout);
		   }
		   catch (_com_error& e)
		   {
			   const wchar_t* msg = e.ErrorMessage();
			   Throw(e.Error(), "device.CreateInputLayout failed for shader %s: %ls. %s\n", name, msg, (cstr) e.Description());
		   }

		   if FAILED(hr)
		   {
			   delete shader;
			   Throw(hr, "device.CreateInputLayout failed with shader %s", name);
			   return ID_VERTEX_SHADER();
		   }

		   hr = device.CreateVertexShader(shaderCode, shaderLength, nullptr, &shader->vs);
		   if FAILED(hr)
		   {
			   delete shader;
			   Throw(hr, "device.CreateVertexShader failed with shader %s", name);
			   return ID_VERTEX_SHADER::Invalid();
		   }

		   shader->name = name;
		   vertexShaders.push_back(shader);
		   return ID_VERTEX_SHADER(vertexShaders.size() - 1);
	   }

	   ID_VERTEX_SHADER CreateVertexShader(cstr pingPath, const D3D11_INPUT_ELEMENT_DESC* vertexDesc, UINT nElements) override
	   {
		   installation.LoadResource(pingPath, *scratchBuffer, 64_kilobytes);
		   return CreateVertexShader(pingPath, scratchBuffer->GetData(), scratchBuffer->Length(), vertexDesc, nElements);
	   }

	   virtual ID_VERTEX_SHADER CreateObjectVertexShader(cstr pingPath)
	   {
		   installation.LoadResource(pingPath, *scratchBuffer, 64_kilobytes);
		   return CreateVertexShader(pingPath, scratchBuffer->GetData(), scratchBuffer->Length(), DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
	   }

	   virtual ID_VERTEX_SHADER CreateParticleVertexShader(cstr pingPath)
	   {
		   installation.LoadResource(pingPath, *scratchBuffer, 64_kilobytes);
		   return CreateVertexShader(pingPath, scratchBuffer->GetData(), scratchBuffer->Length(), DX11::GetParticleVertexDesc(), DX11::NumberOfParticleVertexElements());
	   }

	   ID_PIXEL_SHADER CreatePixelShader(cstr name, const byte* shaderCode, size_t shaderLength)
	   {
		   if (name == nullptr || rlen(name) > 1024) Throw(0, "Bad <name> for pixel shader");
		   if (shaderCode == nullptr || shaderLength < 4 || shaderLength > 65536) Throw(0, "Bad shader code for pixel shader %s", name);

		   DX11PixelShader* shader = new DX11PixelShader;
		   HRESULT hr = device.CreatePixelShader(shaderCode, shaderLength, nullptr, &shader->ps);
		   if FAILED(hr)
		   {
			   delete shader;
			   Throw(hr, "device.CreatePixelShader failed with shader %s", name);
			   return ID_PIXEL_SHADER::Invalid();
		   }

		   shader->name = name;
		   pixelShaders.push_back(shader);
		   return ID_PIXEL_SHADER(pixelShaders.size() - 1);
	   }

	   ID_PIXEL_SHADER CreatePixelShader(cstr pingPath) override
	   {
		   installation.LoadResource(pingPath, *scratchBuffer, 64_kilobytes);
		   return CreatePixelShader(pingPath, scratchBuffer->GetData(), scratchBuffer->Length());
	   }

	   TextureBind CreateDepthTarget(int32 width, int32 height)
	   {
		   return DX11::CreateDepthTarget(device, width, height);
	   }

	   ID_TEXTURE CreateRenderTarget(int32 width, int32 height) override
	   {
		   TextureBind tb = DX11::CreateRenderTarget(device, width, height);   
		   textures.push_back(tb);
		   auto id = ID_TEXTURE(textures.size());

		   char name[32];
		   SafeFormat(name, "RenderTarget_%llu", id.value);

		   mapNameToTexture[name] = id;
		   return id;
	   }

	   ID_GEOMETRY_SHADER CreateGeometryShader(cstr pingPath)
	   {
		   if (pingPath == nullptr || rlen(pingPath) > 1024) Throw(0, "Bad <pingPath> for geometry shader");

		   installation.LoadResource(pingPath, *scratchBuffer, 64_kilobytes);

		   if (scratchBuffer->Length() == 0) Throw(0, "Bad shader code for geometry shader %s", pingPath);

		   DX11GeometryShader* shader = new DX11GeometryShader;
		   HRESULT hr = device.CreateGeometryShader(scratchBuffer->GetData(), scratchBuffer->Length(), nullptr, &shader->gs);
		   if FAILED(hr)
		   {
			   delete shader;
			   Throw(hr, "device.CreateGeometryShader failed with shader %s", pingPath);
			   return ID_GEOMETRY_SHADER::Invalid();
		   }

		   shader->name = pingPath;
		   geometryShaders.push_back(shader);
		   return ID_GEOMETRY_SHADER(geometryShaders.size() - 1);
	   }

	   ID_PIXEL_SHADER currentPixelShaderId;
	   ID_VERTEX_SHADER currentVertexShaderId;

	   bool UseGeometryShader(ID_GEOMETRY_SHADER gid)
	   {
		   if (!gid)
		   {
			   dc.GSSetShader(nullptr, nullptr, 0);
			   return true;
		   }

		   if (gid.value >= geometryShaders.size()) Throw(0, "Bad shader Id in call to UseGeometryShader");

		   auto& gs = *geometryShaders[gid.value];
		   if (gs.gs == nullptr)
		   {
			   Throw(0, "Geometry Shader null for %s", gs.name.c_str());
		   }

		   dc.GSSetShader(gs.gs, nullptr, 0);

		   return true;
	   }

	   bool UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid) override
	   {
		   if (vid.value >= vertexShaders.size()) Throw(0, "Bad vertex shader Id in call to UseShaders");
		   if (pid.value >= pixelShaders.size()) Throw(0, "Bad pixel shader Id in call to UseShaders");

		   auto& vs = *vertexShaders[vid.value];
		   auto& ps = *pixelShaders[pid.value];

		   if (vs.vs == nullptr)
		   {
			   Throw(0, "Vertex Shader null for %s", vs.name.c_str());
		   }

		   if (ps.ps == nullptr)
		   {
			   Throw(0, "Pixel Shader null for %s", ps.name.c_str());
		   }

		   if (vs.vs == nullptr || ps.ps == nullptr)
		   {
			   dc.IASetInputLayout(nullptr);
			   dc.VSSetShader(nullptr, nullptr, 0);
			   dc.PSSetShader(nullptr, nullptr, 0);
			   currentVertexShaderId = ID_VERTEX_SHADER();
			   currentPixelShaderId = ID_PIXEL_SHADER();
			   return false;
		   }
		   else
		   {
			   dc.IASetInputLayout(vs.inputLayout);
			   dc.VSSetShader(vs.vs, nullptr, 0);
			   dc.PSSetShader(ps.ps, nullptr, 0);
			   currentVertexShaderId = vid;
			   currentPixelShaderId = pid;
			   return true;
		   }
	   }

	   void DetachContext()
	   {
		   for (UINT i = 0; i < 10; ++i)
		   {
			   ID3D11ShaderResourceView* nullView = nullptr;
			   dc.PSSetShaderResources(i, 1, &nullView);
		   }

		   ID3D11Buffer* nullBuffer = nullptr;
		   for (UINT i = 0; i < 8; ++i)
		   {
			   dc.VSSetConstantBuffers(i, 1, &nullBuffer);
			   dc.PSSetConstantBuffers(i, 1, &nullBuffer);
			   dc.GSSetConstantBuffers(i, 1, &nullBuffer);
		   }

		   dc.OMSetBlendState(nullptr, nullptr, 0);
		   dc.RSSetState(nullptr);
		   dc.OMSetDepthStencilState(nullptr, 0);
		   dc.OMSetRenderTargets(0, nullptr, nullptr);

		   UINT nStrides = 0;
		   dc.IASetVertexBuffers(0, 1, &nullBuffer, &nStrides, &nStrides);

		   dc.IASetInputLayout(nullptr);
		   dc.VSSetShader(nullptr, nullptr, 0);
		   dc.PSSetShader(nullptr, nullptr, 0);

		   ID3D11ShaderResourceView* nullView = nullptr;
		   dc.PSSetShaderResources(0, 1, &nullView);

		   ID3D11SamplerState* sampler = nullptr;
		   for (UINT i = 0; i < 16; ++i)
		   {
			   dc.PSSetSamplers(i, 1, &sampler);
			   dc.GSSetSamplers(i, 1, &sampler);
			   dc.VSSetSamplers(i, 1, &sampler);
		   }
	   }

	   void SwitchToWindowMode() override
	   {
		   BOOL isFullScreen;
		   AutoRelease<IDXGIOutput> output;
		   if SUCCEEDED(mainSwapChain->GetFullscreenState(&isFullScreen, &output))
		   {
			   if (isFullScreen)
			   {
				   mainSwapChain->SetFullscreenState(false, nullptr);
			   }
		   }
	   }

	   void OnMouseEvent(const RAWMOUSE& m)
	   {
		   lastMouseEvent = m;
	   }

	   void GetGuiMetrics(GuiMetrics& metrics) const override
	   {
		   POINT p;
		   GetCursorPos(&p);
		   ScreenToClient(window, &p);

		   metrics.cursorPosition = Vec2i{ p.x, p.y };
		   metrics.screenSpan = screenSpan;
	   }

	   struct MeshBuffer
	   {
		   ID3D11Buffer* vertexBuffer;
		   ID3D11Buffer* weightsBuffer;
		   UINT numberOfVertices;
		   D3D_PRIMITIVE_TOPOLOGY topology;
		   ID_PIXEL_SHADER psSpotlightShader;
		   ID_PIXEL_SHADER psAmbientShader;
		   ID_VERTEX_SHADER vsSpotlightShader;
		   ID_VERTEX_SHADER vsAmbientShader;
		   bool alphaBlending;
		   bool disableShadowCasting;
	   };

	   std::vector<MeshBuffer> meshBuffers;
	   int64 meshUpdateCount = 0;

	   ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) override
	   {
		   ID3D11Buffer* meshBuffer = vertices ? DX11::CreateImmutableVertexBuffer(device, vertices, nVertices) : nullptr;
		   ID3D11Buffer* weightsBuffer = weights ? DX11::CreateImmutableVertexBuffer(device, weights, nVertices) : nullptr;
		   meshBuffers.push_back(MeshBuffer{ meshBuffer, weightsBuffer, nVertices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ID_PIXEL_SHADER(), ID_PIXEL_SHADER(), ID_VERTEX_SHADER(), ID_VERTEX_SHADER(), false, false});
		   int32 index = (int32)meshBuffers.size();
		   return ID_SYS_MESH(index - 1);
	   }

	   ID_SYS_MESH CreateSkyMesh(const SkyVertex* vertices, uint32 nVertices)
	   {
		   ID3D11Buffer* meshBuffer = vertices ? DX11::CreateImmutableVertexBuffer(device, vertices, nVertices) : nullptr;
		   meshBuffers.push_back(MeshBuffer{ meshBuffer, nullptr, nVertices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ID_PIXEL_SHADER(), ID_PIXEL_SHADER(), ID_VERTEX_SHADER(), ID_VERTEX_SHADER(), false, false });
		   int32 index = (int32)meshBuffers.size();
		   return ID_SYS_MESH(index - 1);
	   }

	   void SetSpecialAmbientShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) override
	   {
		   if (id.value < 0 || id.value >= meshBuffers.size())
		   {
			   Throw(E_INVALIDARG, "renderer.SetSpecialAmbientShader(ID_SYS_MESH id, ....) - Bad id ");
		   }

		   auto& m = meshBuffers[id.value];
		   
		   auto i = nameToPixelShader.find(ps);
		   if (i == nameToPixelShader.end())
		   {
			   ID_PIXEL_SHADER pxId;
			   if (ps == nullptr || *ps == 0)
			   {
				   pxId = ID_PIXEL_SHADER::Invalid();
			   }
			   else
			   {
				   installation.LoadResource(ps, *scratchBuffer, 64_kilobytes);
				   pxId = CreatePixelShader(ps, scratchBuffer->GetData(), scratchBuffer->Length());
			   }

			   i = nameToPixelShader.insert(ps, pxId).first;
		   }

		   m.psAmbientShader = i->second;
		   
		   auto j = nameToVertexShader.find(vs);
		   if (j == nameToVertexShader.end())
		   {
			   ID_VERTEX_SHADER vxId;
			   if (vs == nullptr || *vs == 0)
			   {
				   vxId = ID_VERTEX_SHADER::Invalid();
			   }
			   else
			   {
				   installation.LoadResource(vs, *scratchBuffer, 64_kilobytes);

				   if (strstr(vs, "skinned"))
				   {
					   vxId = CreateVertexShader(vs, scratchBuffer->GetData(), scratchBuffer->Length(), DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
				   }
				   else
				   {
					   vxId = CreateVertexShader(vs, scratchBuffer->GetData(), scratchBuffer->Length(), DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
				   }
			   }

			   j = nameToVertexShader.insert(vs, vxId).first;
		   }

		   m.vsAmbientShader = j->second;
		   m.alphaBlending = alphaBlending;
	   }

	   void SetSpecialSpotlightShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) override
	   {
		   if (id.value < 0 || id.value >= meshBuffers.size())
		   {
			   Throw(E_INVALIDARG, "renderer.SetSpecialAmbientShader(ID_SYS_MESH id, ....) - Bad id ");
		   }

		   auto& m = meshBuffers[id.value];

		   auto i = nameToPixelShader.find(ps);
		   if (i == nameToPixelShader.end())
		   {
			   ID_PIXEL_SHADER pxId;
			   if (ps == nullptr || *ps == 0)
			   {
				   pxId = ID_PIXEL_SHADER::Invalid();
			   }
			   else
			   {
				   installation.LoadResource(ps, *scratchBuffer, 64_kilobytes);
				   pxId = CreatePixelShader(ps, scratchBuffer->GetData(), scratchBuffer->Length());
			   }

			   i = nameToPixelShader.insert(ps, pxId).first;
		   }

		   m.psSpotlightShader = i->second;

		   auto j = nameToVertexShader.find(vs);
		   if (j == nameToVertexShader.end())
		   {
			   ID_VERTEX_SHADER vxId;
			   if (vs == nullptr || *vs == 0)
			   {
				   vxId = ID_VERTEX_SHADER::Invalid();
			   }
			   else
			   {
				   installation.LoadResource(vs, *scratchBuffer, 64_kilobytes);

				   if (strstr(vs, "skinned"))
				   {
					   vxId = CreateVertexShader(vs, scratchBuffer->GetData(), scratchBuffer->Length(), DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
				   }
				   else
				   {
					   vxId = CreateVertexShader(vs, scratchBuffer->GetData(), scratchBuffer->Length(), DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
				   }
			   }

			   j = nameToVertexShader.insert(vs, vxId).first;
		   }

		   m.vsSpotlightShader = j->second;
		   m.alphaBlending = alphaBlending;
	   }
	   
	   void UpdateMesh(ID_SYS_MESH id, const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) override
	   {
		   if (id.value < 0 || id.value >= meshBuffers.size())
		   {
			   Throw(E_INVALIDARG, "renderer.UpdateMesh(ID_MESH id, ....) - Bad id ");
		   }

		   meshUpdateCount++;

		   meshBuffers[id.value].numberOfVertices = nVertices;

		   ID3D11Buffer* newMesh = vertices != nullptr ? DX11::CreateImmutableVertexBuffer(device, vertices, nVertices) : nullptr;
		   ID3D11Buffer* newWeights = weights != nullptr ? DX11::CreateImmutableVertexBuffer(device, weights, nVertices) : nullptr;

		   if (meshBuffers[id.value].vertexBuffer) meshBuffers[id.value].vertexBuffer->Release();
		   meshBuffers[id.value].vertexBuffer = newMesh;

		   if (meshBuffers[id.value].weightsBuffer) meshBuffers[id.value].weightsBuffer->Release();
		   meshBuffers[id.value].weightsBuffer = newWeights;
	   }

	   enum RenderPhase
	   {
		   RenderPhase_None,
		   RenderPhase_DetermineShadowVolumes,
		   RenderPhase_DetermineSpotlight,
		   RenderPhase_DetermineAmbient
	   };

	   RenderPhase phase = RenderPhase_None;

	   void DrawLightCone(const Light& light)
	   {
		   trianglesThisFrame += DX11::DrawLightCone(light, currentGlobalState.viewDir, dc, *lightConeBuffer);
	   }

	   void Draw(ID_SYS_MESH id, const ObjectInstance* instances, uint32 nInstances) override
	   {
		   if (id == ID_SYS_MESH::Invalid()) return;
		   if (id.value < 0 || id.value >= meshBuffers.size()) Throw(E_INVALIDARG, "renderer.DrawObject(ID_MESH id) - Bad id ");

		   auto& m = meshBuffers[id.value];

		   Draw(m, instances, nInstances);
	   }

	   void Draw(MeshBuffer& m, const ObjectInstance* instances, uint32 nInstances)
	   {
		   if (!m.vertexBuffer)
			   return;

		   if (phase == RenderPhase_DetermineShadowVolumes && m.disableShadowCasting)
			   return;

		   ID3D11Buffer* buffers[2] = { m.vertexBuffer, m.weightsBuffer };

		   entitiesThisFrame += (int64) nInstances;

		   bool overrideShader = false;

		   if (m.psSpotlightShader && phase == RenderPhase_DetermineSpotlight)
		   {
			   UseShaders(m.vsSpotlightShader, m.psSpotlightShader);
			   overrideShader = true;
		   }
		   else if (m.psAmbientShader && phase == RenderPhase_DetermineAmbient)
		   {
			   UseShaders(m.vsAmbientShader, m.psAmbientShader);
			   overrideShader = true;
		   }

		   FLOAT blendFactorUnused[] = { 0,0,0,0 };
		   if (m.alphaBlending)
		   {
			   dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			   dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
		   }

		   UINT strides[] = { sizeof(ObjectVertex), sizeof(BoneWeights) };
		   UINT offsets[]{ 0, 0 };
		   dc.IASetPrimitiveTopology(m.topology);
		   dc.IASetVertexBuffers(0, m.weightsBuffer ? 2 : 1, buffers, strides, offsets);

		   for (uint32 i = 0; i < nInstances; i++)
		   {
			   // dc.DrawInstances crashed the debugger, replace with single instance render call for now
			   DX11::CopyStructureToBuffer(dc, instanceBuffer, instances + i, sizeof(ObjectInstance));
			   dc.VSSetConstantBuffers(CBUFFER_INDEX_INSTANCE_BUFFER, 1, &instanceBuffer);
			   dc.Draw(m.numberOfVertices, 0);

			   trianglesThisFrame += m.numberOfVertices / 3;
		   }

		   if (overrideShader)
		   {
			   UseShaders(currentVertexShaderId, currentPixelShaderId);	 
		   }
		   
		   if (m.alphaBlending)
		   {
			   if (builtFirstPass)
			   {
				   dc.OMSetBlendState(additiveBlend, blendFactorUnused, 0xffffffff);
				   dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
			   }
			   else
			   {
				   dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
				   builtFirstPass = true;
			   }
		   }
	   }

	   virtual Windows::IWindow& Window()
	   {
		   return window;
	   }

	   void DrawCursor()
	   {
		   GuiMetrics metrics;
		   GetGuiMetrics(metrics);
		   gui->DrawCursor(metrics, cursorId);
	   }

	   int64 AIcost = 0;
	   int64 guiCost = 0;
	   int64 objCost = 0;
	   int64 presentCost = 0;
	   int64 frameTime = 0;

	   ID_PIXEL_SHADER CreateNamedPixelShader(cstr pingPath) override
	   {
		   auto i = nameToPixelShader.find(pingPath);
		   if (i == nameToPixelShader.end())
		   {
			   auto pxId = CreatePixelShader(pingPath);
			   i = nameToPixelShader.insert(pingPath, pxId).first;
		   }
		   return i->second;
	   }

	   bool builtFirstPass = false;

	   void RenderParticles(std::vector<ParticleVertex>& particles, ID_PIXEL_SHADER psID, ID_VERTEX_SHADER vsID, ID_GEOMETRY_SHADER gsID)
	   {
		   if (particles.empty()) return;
		   if (!UseShaders(vsID, psID)) return;
		   if (!UseGeometryShader(gsID)) return;

		   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		   dc.RSSetState(particleRasterizering);
		   dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);

		   size_t qSize = particles.size();

		   const ParticleVertex* start = &particles[0];

		   size_t i = 0;

		   while (qSize > 0)
		   {
			   size_t chunkSize = min(qSize, (size_t)PARTICLE_BUFFER_VERTEX_CAPACITY);
			   DX11::CopyStructureToBuffer(dc, particleBuffer, start + i, chunkSize * sizeof(ParticleVertex));

			   UINT strides[1] = { sizeof(ParticleVertex) };
			   UINT offsets[1] = { 0 };
			  
			   dc.IASetVertexBuffers(0, 1, &particleBuffer, strides, offsets);
			   dc.Draw((UINT)chunkSize, 0);

			   i += chunkSize;
			   qSize -= chunkSize;
		   }

		   UseGeometryShader(ID_GEOMETRY_SHADER::Invalid());
	   }

	   Graphics::RenderPhaseConfig phaseConfig;

	   ID_PIXEL_SHADER GetObjectShaderPixelId(RenderPhase phase)
	   {
		   switch (phaseConfig.EnvironmentalMap)
		   {
		   case Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE:
			   switch (phase)
			   {
			   case RenderPhase_DetermineAmbient:
				   return idObjAmbientPS;
			   case RenderPhase_DetermineSpotlight:
				   return idObjPS;
			   case RenderPhase_DetermineShadowVolumes:
				   return idObjPS_Shadows;
			   default:
				   Throw(0, "Unknown render phase: %d", phase);
			   }
		   case Graphics::ENVIRONMENTAL_MAP_PROCEDURAL:
			   switch (phase)
			   {
			   case RenderPhase_DetermineAmbient:
				   return idObj_Ambient_NoEnvMap_PS;
			   case RenderPhase_DetermineSpotlight:
				   return idObj_Spotlight_NoEnvMap_PS;
			   case RenderPhase_DetermineShadowVolumes:
				   return idObjPS_Shadows;
			   default:
				   Throw(0, "Unknown render phase: %d", phase);
			   }
		   default:
			   Throw(0, "Environemtn mode %d not implemented", phaseConfig.EnvironmentalMap);
		   }

		   return ID_PIXEL_SHADER();
	   }

	   DX11::TextureBind& GetTexture(ID_TEXTURE id)
	   {
		   auto index = id.value - 1;
		   if (index < 0 || index >= textures.size()) Throw(0, "Bad texture id: %llu", index);
		   return textures[index];
	   }

	   void Render3DGui()
	   { 
		   size_t cursor = 0;
		   size_t len = gui3DTriangles.size();

		   ObjectInstance one = { Matrix4x4::Identity(), RGBA(1.0f, 1.0f, 1.0f, 1.0f) };

		   while (len > 0)
		   {
			   auto* v = gui3DTriangles.data() + cursor;

			   size_t nTriangleBatchCount = min<size_t>(len, GUI3D_BUFFER_TRIANGLE_CAPACITY);

			   DX11::CopyStructureToBuffer(dc, gui3DBuffer, v, nTriangleBatchCount * sizeof(VertexTriangle));

			   MeshBuffer m;
			   m.alphaBlending = false;
			   m.disableShadowCasting = false;
			   m.vertexBuffer = gui3DBuffer;
			   m.weightsBuffer = nullptr;
			   m.numberOfVertices = (UINT) nTriangleBatchCount * 3;
			   m.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			   Draw(m, &one, 1);

			   len -= nTriangleBatchCount;
			   cursor += nTriangleBatchCount;
		   }
	   }

	   void RenderToShadowBuffer(DepthRenderData& drd, ID_TEXTURE shadowBuffer, IScene& scene)
	   {
		   auto shadowBind = GetTexture(shadowBuffer);

		   dc.OMSetRenderTargets(0, nullptr, shadowBind.depthView);

		   D3D11_TEXTURE2D_DESC desc;
		   shadowBind.texture->GetDesc(&desc);

		   D3D11_VIEWPORT viewport = { 0 };
		   viewport.Width = (FLOAT)desc.Width;
		   viewport.Height = (FLOAT)desc.Height;
		   viewport.MinDepth = 0.0f;
		   viewport.MaxDepth = 1.0f;
		   dc.RSSetViewports(1, &viewport);

		   dc.ClearDepthStencilView(shadowBind.depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		   FLOAT blendFactorUnused[] = { 0,0,0,0 };
		   dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);

		   dc.RSSetState(shadowRasterizering);

		   UseShaders(idSkinnedObjVS_Shadows, idObjPS_Shadows);

		   DX11::CopyStructureToBuffer(dc, depthRenderStateBuffer, drd);
		   dc.VSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);
		   dc.PSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);

		   phase = RenderPhase_DetermineShadowVolumes;
		   scene.RenderShadowPass(drd, *this, true);

		   UseShaders(idObjVS_Shadows, idObjPS_Shadows);

		   DX11::CopyStructureToBuffer(dc, depthRenderStateBuffer, drd);
		   dc.VSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);
		   dc.PSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);

		   scene.RenderShadowPass(drd, *this, false);
	   }

	   void SetupSpotlightConstants()
	   {
		   dc.VSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
		   dc.PSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
		   dc.GSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
	   }

	   void RenderSpotlightLitScene(const Light& lightSubset, IScene& scene)
	   {
		   Light light = lightSubset;

		   DepthRenderData drd;
		   if (PrepareDepthRenderFromLight(light, drd))
		   {
			   const float f = 1.0f / rng.max();
			   drd.randoms.x = rng() * f;
			   drd.randoms.y = rng() * f;
			   drd.randoms.z = rng() * f;
			   drd.randoms.w = rng() * f;

			   RenderToShadowBuffer(drd, phaseConfig.shadowBuffer, scene);

			   RenderTarget rt = GetCurrentRenderTarget();
			   dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

			   phase = RenderPhase_DetermineSpotlight;

			   ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase);
			   UseShaders(idObjVS, idPS);

			   ExpandViewportToEntireTexture(phaseConfig.depthTarget);

			   light.randoms = drd.randoms;
			   light.time = drd.time;
			   light.right = drd.right;
			   light.up = drd.up;
			   light.worldToShadowBuffer = drd.worldToScreen;

			   DX11::CopyStructureToBuffer(dc, lightStateBuffer, light);
			   SetupSpotlightConstants();

			   FLOAT blendFactorUnused[] = { 0,0,0,0 };

			   if (builtFirstPass)
			   {
				   dc.OMSetBlendState(additiveBlend, blendFactorUnused, 0xffffffff);
				   dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
			   }
			   else
			   {
				   dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
				   builtFirstPass = true;
			   }

			   auto shadowBind = GetTexture(phaseConfig.shadowBuffer);

			   dc.PSSetShaderResources(2, 1, &shadowBind.shaderView);
			   dc.RSSetState(objectRasterizering);

			   scene.RenderObjects(*this, false);
			   scene.RenderObjects(*this, true);

			   Render3DGui();

			   dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			   RenderParticles(fog, idFogSpotlightPS, idParticleVS, idFogSpotlightGS);
			   phase = RenderPhase_None;

			   dc.OMSetDepthStencilState(objDepthState, 0);
		   }
	   }

	   void RenderGui(IScene& scene)
	   {
		   OS::ticks now = OS::CpuTicks();

		   GuiMetrics metrics;
		   GetGuiMetrics(metrics);
		   gui->RenderGui(scene, metrics, !phaseConfig.renderTarget);

		   gui->FlushLayer();

		   if (!phaseConfig.renderTarget)
		   {
			   DrawCursor();
			   gui->FlushLayer();
		   }

		   guiCost = OS::CpuTicks() - now;
	   }

	   void SetAmbientConstants()
	   {
		   AmbientData ad;
		   ad.localLight = ambientLight.ambient;
		   ad.fogConstant = ambientLight.fogConstant;
		   DX11::CopyStructureToBuffer(dc, ambientBuffer, ad);
		   dc.PSSetConstantBuffers(CBUFFER_INDEX_AMBIENT_LIGHT, 1, &ambientBuffer);
	   }

	   Light ambientLight = { 0 };

	   void RenderAmbient(IScene& scene, const Light& ambientLight)
	   {
		   phase = RenderPhase_DetermineAmbient;

		   ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase);
		   if (UseShaders(idObjAmbientVS, idPS))
		   {
			   FLOAT blendFactorUnused[] = { 0,0,0,0 };
			   ExpandViewportToEntireTexture(phaseConfig.depthTarget);

			   if (builtFirstPass)
			   {
				   dc.OMSetBlendState(additiveBlend, blendFactorUnused, 0xffffffff);
				   dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
			   }
			   else
			   {
				   dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
				   builtFirstPass = true;
			   }

			   dc.RSSetState(objectRasterizering);

			   this->ambientLight = ambientLight;
			   SetAmbientConstants();

			   scene.RenderObjects(*this, false);
			   scene.RenderObjects(*this, true);
			   Render3DGui();

			   dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			   RenderParticles(fog, idFogAmbientPS, idParticleVS, idFogAmbientGS);
		   }
		   
		   phase = RenderPhase_None;
	   }

	   void ClearCurrentRenderBuffers(const RGBA& clearColour)
	   {
		   RenderTarget rt = GetCurrentRenderTarget();
		   dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

		   dc.ClearDepthStencilView(rt.depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		   if (clearColour.alpha > 0)
		   {
			   dc.ClearRenderTargetView(rt.renderTargetView, (const FLOAT*)&clearColour);
		   }
	   }

	   void InitFontAndMaterialAndSpriteShaderResourceViewsAndSamplers()
	   {
		   dc.PSSetShaderResources(TXUNIT_FONT, 1, &fontBinding);
		   auto* view = cubeTextures->ShaderResourceView();
		   dc.PSSetShaderResources(TXUNIT_ENV_MAP, 1, &view);

		   ID3D11ShaderResourceView* materialViews[1] = { materials->Textures().View() };
		   dc.PSSetShaderResources(TXUNIT_MATERIALS, 1, materialViews);

		   ID3D11ShaderResourceView* spriteviews[1] = { gui->SpriteView() };
		   dc.PSSetShaderResources(TXUNIT_SPRITES, 1, spriteviews);

		   dc.PSSetSamplers(0, 16, samplers);
		   dc.GSSetSamplers(0, 16, samplers);
		   dc.VSSetSamplers(0, 16, samplers);
	   }

	   GlobalState currentGlobalState = { 0 };

	   void UpdateGlobalState(IScene& scene)
	   {
		   GlobalState g;
		   scene.GetCamera(g.worldMatrixAndProj, g.worldMatrix, g.projMatrix, g.eye, g.viewDir);

		   float aspectRatio = screenSpan.y / (float)screenSpan.x;
		   g.aspect = { aspectRatio,0,0,0 };

		   g.guiScale = gui->GetGuiScale();

		   DX11::CopyStructureToBuffer(dc, globalStateBuffer, g);

		   currentGlobalState = g;

		   dc.VSSetConstantBuffers(CBUFFER_INDEX_GLOBAL_STATE, 1, &globalStateBuffer);
		   dc.PSSetConstantBuffers(CBUFFER_INDEX_GLOBAL_STATE, 1, &globalStateBuffer);
		   dc.GSSetConstantBuffers(CBUFFER_INDEX_GLOBAL_STATE, 1, &globalStateBuffer);

		   Vec4 sunlight = { Sin(45_degrees), 0, Cos(45_degrees), 0 };
		   Vec4 sunlightLocal = sunlight;

		   DX11::CopyStructureToBuffer(dc, sunlightStateBuffer, sunlightLocal);

		   dc.VSSetConstantBuffers(CBUFFER_INDEX_SUNLIGHT, 1, &sunlightStateBuffer);
		   dc.PSSetConstantBuffers(CBUFFER_INDEX_SUNLIGHT, 1, &sunlightStateBuffer);
		   dc.GSSetConstantBuffers(CBUFFER_INDEX_SUNLIGHT, 1, &sunlightStateBuffer);

		   DX11::CopyStructureToBuffer(dc, boneMatricesStateBuffer, boneMatrices);
		   dc.VSSetConstantBuffers(CBUFFER_INDEX_BONE_MATRICES, 1, &boneMatricesStateBuffer);
	   }

	   struct RenderTarget
	   {
		   ID3D11RenderTargetView* renderTargetView;
		   ID3D11DepthStencilView* depthView;
	   };

	   RenderTarget GetCurrentRenderTarget()
	   {
		   RenderTarget rt = { 0 };

		   rt.depthView = GetTexture(phaseConfig.depthTarget).depthView;

		   if (phaseConfig.renderTarget)
		   {
			   rt.renderTargetView = GetTexture(phaseConfig.renderTarget).renderView;
		   }
		   else
		   {
			   rt.renderTargetView = mainBackBufferView;
		   }

		   if (rt.depthView == nullptr)
		   {
			   Throw(0, "GetCurrentRenderTarget - bad depth buffer");
		   }

		   if (rt.renderTargetView == nullptr)
		   {
			   Throw(0, "GetCurrentRenderTarget - bad render target buffer");
		   }

		   return rt;
	   }

	   void RenderSkybox(IScene& scene)
	   {
		   ID_CUBE_TEXTURE cubeId = scene.GetSkyboxCubeId();

		   ID3D11ShaderResourceView* skyCubeTextureView = cubeTextures->GetShaderView(cubeId);
		   if (!skyCubeTextureView)
		   {
			   return;
		   }

		   if (UseShaders(idObjSkyVS, idObjSkyPS))
		   {
			   auto& mesh = meshBuffers[skyMeshId.value];
			   UINT strides[] = { sizeof(SkyVertex) };
			   UINT offsets[]{ 0 };
			   dc.IASetPrimitiveTopology(mesh.topology);
			   dc.IASetVertexBuffers(0, 1, &mesh.vertexBuffer, strides, offsets);
			   dc.PSSetShaderResources(0, 1, &skyCubeTextureView);
			   dc.PSSetSamplers(0, 1, &skySampler);

			   dc.RSSetState(skyRasterizering);
			   dc.OMSetDepthStencilState(noDepthTestOrWrite, 0);

			   FLOAT blendFactorUnused[] = { 0,0,0,0 };
			   dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
			   dc.Draw(mesh.numberOfVertices, 0);

			   dc.PSSetSamplers(0, 16, samplers);
		   }
		   else
		   {
			   Throw(0, "DX11Renderer::RenderSkybox failed. Error setting sky shaders");
		   }
	   }

	   void Render3DObjects(IScene& scene)
	   {
		   auto now = OS::CpuTicks();

		   dc.RSSetState(objectRasterizering);
		   dc.OMSetDepthStencilState(objDepthState, 0);

		   builtFirstPass = false;

		   uint32 nLights = 0;
		   const Light* lights = scene.GetLights(nLights);
		   if (lights != nullptr)
		   {
			   for (size_t i = 0; i < nLights; ++i)
			   {
				   try
				   {
					   RenderSpotlightLitScene(lights[i], scene);
				   }
				   catch (IException& ex)
				   {
					   Throw(ex.ErrorCode(), "Error lighting scene with light #%d: %s", i, ex.Message());
				   }
			   }

			   RenderAmbient(scene, lights[0]);
		   }

		   objCost = OS::CpuTicks() - now;
	   }

	   void Render(Graphics::ENVIRONMENTAL_MAP envMap, IScene& scene) override
	   {
		   phaseConfig.EnvironmentalMap = envMap;
		   phaseConfig.shadowBuffer = shadowBufferId;
		   phaseConfig.depthTarget = mainDepthBufferId;

		   if (GetTexture(phaseConfig.shadowBuffer).depthView == nullptr)
		   {
			   Throw(0, "No shadow depth buffer set for DX1AppRenderer::Render(...)");
		   }

		   trianglesThisFrame = 0;
		   entitiesThisFrame = 0;

		   auto now = OS::CpuTicks();
		   AIcost = now - lastTick;

		   if (mainBackBufferView.IsNull()) return;

		   lastTextureId = ID_TEXTURE::Invalid();

		   ExpandViewportToEntireTexture(phaseConfig.depthTarget);

		   ClearCurrentRenderBuffers(scene.GetClearColour());

		   now = OS::CpuTicks();

		   UpdateGlobalState(scene);

		   RenderTarget rt = GetCurrentRenderTarget();
		   dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

		   RenderSkybox(scene);

		   InitFontAndMaterialAndSpriteShaderResourceViewsAndSamplers();

		   Render3DObjects(scene);

		   FLOAT blendFactorUnused[] = { 0,0,0,0 };
		   dc.OMSetBlendState(plasmaBlend, blendFactorUnused, 0xffffffff);
		   RenderParticles(plasma, idPlasmaPS, idParticleVS, idPlasmaGS);

		   DrawLightCones(scene);

		   UpdateGlobalState(scene);

		   RenderGui(scene);

		   now = OS::CpuTicks();

		   mainSwapChain->Present(1, 0);

		   presentCost = OS::CpuTicks() - now;

		   DetachContext();

		   now = OS::CpuTicks();
		   frameTime = now - lastTick;
		   lastTick = now;
	   }

	   void DrawLightCones(IScene& scene)
	   {
		   uint32 nLights = 0;
		   const Light* lights = scene.GetLights(nLights);

		   if (lights != nullptr)
		   {
			   UINT strides[] = { sizeof(ObjectVertex) };
			   UINT offsets[]{ 0 };

			   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			   dc.IASetVertexBuffers(0, 1, &lightConeBuffer, strides, offsets);
			   dc.RSSetState(spriteRasterizering);

			   UseShaders(idLightConeVS, idLightConePS);

			   FLOAT blendFactorUnused[] = { 0,0,0,0 };
			   dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);
			   dc.OMSetDepthStencilState(objDepthState, 0);
			   dc.PSSetConstantBuffers(0, 1, &globalStateBuffer);
			   dc.VSSetConstantBuffers(0, 1, &globalStateBuffer);

			   ObjectInstance identity;
			   identity.orientation = Matrix4x4::Identity();
			   identity.highlightColour = { 0 };
			   DX11::CopyStructureToBuffer(dc, instanceBuffer, &identity, sizeof(ObjectInstance));
			   dc.VSSetConstantBuffers(CBUFFER_INDEX_INSTANCE_BUFFER, 1, &instanceBuffer);

			   for (uint32 i = 0; i < nLights; ++i)
			   {
				   if (lights[i].hasCone)
				   {
					   DrawLightCone(lights[i]);
				   }
			   }

			   dc.IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
			   dc.RSSetState(nullptr);
			   dc.PSSetConstantBuffers(0, 0, nullptr);
			   dc.VSSetConstantBuffers(0, 0, nullptr);
		   }
	   }

	   cstr scdt_name = "SwapChainDepthTarget";

	   void ResizeBuffers(const Vec2i& span)
	   {
		   mainBackBufferView.Detach();

		   VALIDATEDX11(mainSwapChain->ResizeBuffers(1, span.x, span.y, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

		   AutoRelease<ID3D11Texture2D> backBuffer;
		   VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));
		   VALIDATEDX11(device.CreateRenderTargetView(backBuffer, nullptr, &mainBackBufferView));

		   RECT rect;
		   GetClientRect(window, &rect);

		   auto newDepthBuffer = CreateDepthTarget(rect.right - rect.left, rect.bottom - rect.top);

		   auto i = mapNameToTexture.find(scdt_name);
		   if (i == mapNameToTexture.end())
		   {
			   Throw(0, "Expecting to find %s in mapNameToTexture", scdt_name);
		   }

		   auto& t = textures[i->second - 1];
		   t.depthView->Release();
		   t.shaderView->Release();
		   t.texture->Release();
		   t = newDepthBuffer;
	   }

	   BOOL isFullScreen = FALSE;

	   void OnSize(Vec2i span) override
	   {
		   if (span.x > 0 && span.y > 0)
		   {
			   D3D11_TEXTURE2D_DESC desc;
			   {
				   AutoRelease<ID3D11Texture2D> backBuffer;
				   VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));
				   backBuffer->GetDesc(&desc);
			   }

			   ResizeBuffers(span);

			   AutoRelease<IDXGIOutput> output;
			   mainSwapChain->GetFullscreenState(&isFullScreen, &output);
			   if (!isFullScreen)
			   {
			   }
		   }
	   }

	   void SetCursorBitmap(const Textures::BitmapLocation& sprite, Vec2i hotspotOffset) override
	   {
		   gui->SetCursorBitmap(sprite, hotspotOffset);
	   }

	   void SetCursorVisibility(bool isVisible) override
	   {
		   Rococo::OS::SetCursorVisibility(isVisible, window);
	   }
   };
}

namespace Rococo
{
	namespace DX11
	{
		static_assert(sizeof(DepthRenderData) % 16 == 0, "DX11 requires size of DepthRenderData to be multipe of 16 bytes");
		static_assert(sizeof(GlobalState) % 16 == 0, "DX11 requires size of GlobalState to be multipe of 16 bytes");
		static_assert(sizeof(Light) % 16 == 0, "DX11 requires size of Light to be multipe of 16 bytes");

		Rococo::IRenderer* CreateDX11Renderer(Factory& factory, Windows::IWindow& window)
		{
			auto* renderer = new ANON::DX11AppRenderer(factory, window);
			
			try
			{
				renderer->BindMainWindow();
			}
			catch (IException&)
			{
				renderer->Free();
				throw;
			}

			return renderer;
		}
	}
}

