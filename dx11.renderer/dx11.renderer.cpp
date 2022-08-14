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

	struct AmbientData
	{
		RGBA localLight;
		float fogConstant = -0.2218f; // light = e(Rk). Where R is distance. k = -0.2218 gives modulation of 1/256 at 25 metres, reducing full brightness to dark
		float a = 0;
		float b = 0;
		float c = 0;
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
	   IInstallation& installation;
	   ID3D11Device& device;
	   ID3D11DeviceContext& dc;
	   IDXGIFactory& factory;

	   AutoFree<IDX11TextureManager> textureManager;
	   AutoFree<IDX11Meshes> meshes;
	   AutoFree<IDX11Shaders> shaders;

	   AutoRelease<IDXGISwapChain> mainSwapChain;
	   AutoRelease<ID3D11RenderTargetView> mainBackBufferView;

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

	   std::default_random_engine rng;
	   OS::ticks lastTick;

	   int64 trianglesThisFrame = 0;
	   int64 entitiesThisFrame = 0;
	   ID_TEXTURE mainDepthBufferId;
	   ID_TEXTURE shadowBufferId;

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
		   return textureManager->LoadAlphaTextureArray(uniqueName, span, nElements, enumerator);		
	   }

	   void SetSpecialAmbientShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) override
	   {
		   auto& m = meshes->GetBuffer(id);

		   ID_PIXEL_SHADER pxId = Shaders().CreatePixelShader(ps);

		   m.psAmbientShader = pxId;

		   ID_VERTEX_SHADER vxId;

		   if (strstr(vs, "skinned"))
		   {
			   vxId = shaders->CreateVertexShader(vs, DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
		   }
		   else
		   {
			   vxId = shaders->CreateVertexShader(vs, DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
		   }

		   m.vsAmbientShader = vxId;
		   m.alphaBlending = alphaBlending;
	   }

	   void SetSpecialSpotlightShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) override
	   {
		   auto& m = meshes->GetBuffer(id);

		   ID_PIXEL_SHADER pxId = Shaders().CreatePixelShader(ps);

		   m.psSpotlightShader = pxId;

		   ID_VERTEX_SHADER vxId;

		   if (strstr(vs, "skinned"))
		   {
			   vxId = shaders->CreateVertexShader(vs, DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
		   }
		   else
		   {
			   vxId = shaders->CreateVertexShader(vs, DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
		   }

		   m.vsSpotlightShader = vxId;
		   m.alphaBlending = alphaBlending;
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
		   textureManager->SetGenericTextureArray(id);
	   }

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
		   installation(_factory.installation), 
		   device(_factory.device), dc(_factory.dc), factory(_factory.factory),
		   materials(CreateMaterials(installation, device, dc)),
		   scratchBuffer(CreateExpandingBuffer(64_kilobytes)),
		   window(_window),
		   textureManager(CreateTextureManager(installation, device, dc)),
		   meshes(CreateMeshManager(device)),
		   shaders(CreateShaderManager(installation, device, dc))
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

		   DX11::TextureBind fb = textureManager->Loader().LoadAlphaBitmap("!font1.tif");
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
		   
		   idObjVS = Shaders().CreateObjectVertexShader("!object.vs");
		   idObjPS = Shaders().CreatePixelShader("!object.ps");
		   idParticleVS = Shaders().CreateParticleVertexShader("!particle.vs");
		   idPlasmaGS = Shaders().CreateGeometryShader("!plasma.gs");
		   idFogSpotlightGS = Shaders().CreateGeometryShader("!fog.spotlight.gs");
		   idFogAmbientGS = Shaders().CreateGeometryShader("!fog.ambient.gs");
		   idObj_Spotlight_NoEnvMap_PS = Shaders().CreatePixelShader("!obj.spotlight.no_env.ps");
		   idObj_Ambient_NoEnvMap_PS = Shaders().CreatePixelShader("!obj.ambient.no_env.ps");
		   idPlasmaPS = Shaders().CreatePixelShader("!plasma.ps");
		   idFogSpotlightPS = Shaders().CreatePixelShader("!fog.spotlight.ps");
		   idFogAmbientPS = Shaders().CreatePixelShader("!fog.ambient.ps");
		   idObjAmbientPS = Shaders().CreatePixelShader("!ambient.ps");
		   idObjAmbientVS = Shaders().CreateObjectVertexShader("!ambient.vs");
		   idObjVS_Shadows = Shaders().CreateObjectVertexShader("!shadow.vs");
		   idSkinnedObjVS_Shadows = shaders->CreateVertexShader("!skinned.shadow.vs", DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
		   idObjPS_Shadows = Shaders().CreatePixelShader("!shadow.ps");
		   idLightConePS = Shaders().CreatePixelShader("!light_cone.ps");
		   idLightConeVS = shaders->CreateVertexShader("!light_cone.vs", DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
		   idObjSkyVS = shaders->CreateVertexShader("!skybox.vs", DX11::GetSkyVertexDesc(), DX11::NumberOfSkyVertexElements());
		   idObjSkyPS = Shaders().CreatePixelShader("!skybox.ps");

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
		   skyMeshId = meshes->CreateSkyMesh(skyboxVertices, sizeof(skyboxVertices) / sizeof(SkyVertex));

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

		   for (auto& s : samplers)
		   {
			   if (s) s->Release();
		   }
	   }

	   IGuiResources& Gui() override
	   {
		   return gui->Gui();
	   }

	   IMaterials& Materials() override
	   {
		   return *materials;
	   }

	   ITextureManager& Textures() override
	   {
		   return *textureManager;
	   }

	   IMeshes& Meshes() override
	   {
		   return *meshes;
	   }

	   IShaders& Shaders() override
	   {
		   return *shaders;
	   }

	   IDX11Shaders& DX11Shaders() override
	   {
		   return *shaders;
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

	   bool UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid) override
	   {
		   return shaders->UseShaders(vid, pid);
	   }

	   ID_CUBE_TEXTURE CreateCubeTexture(cstr path, cstr extension)
	   {
		   return cubeTextures->CreateCubeTexture(textureManager->Loader(), path, extension);
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

	   void ShowTextureVenue(IMathsVisitor& visitor)
	   {
		   materials->ShowVenue(visitor);
	   }

	   void SetShadowCasting(ID_SYS_MESH id, boolean32 isActive)
	   {
		   meshes->SetShadowCasting(id, isActive);
	   }

	   virtual void ShowVenue(IMathsVisitor& visitor)
	   {
#ifdef _DEBUG
		   visitor.ShowString("Renderer", "DirectX 11.1 Rococo MPLAT - Debug");
#else
		   visitor.ShowString("Renderer", "DirectX 11.1 Rococo MPLAT - Release");
#endif
		   visitor.ShowString("Screen Span", "%d x %d pixels", screenSpan.x, screenSpan.y);

		   shaders->ShowVenue(visitor);

		   ShowVenueForDevice(visitor, device);

		   D3D11_RENDER_TARGET_VIEW_DESC desc;
		   mainBackBufferView->GetDesc(&desc);

		   visitor.ShowString("BackBuffer format", "%u", desc.Format);

		   D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc;
		   textureManager->GetTexture(mainDepthBufferId).depthView->GetDesc(&dsDesc);

		   visitor.ShowString("DepthStencil format", "%u", dsDesc.Format);

		   visitor.ShowDecimal("Number of textures", (int64)textureManager->Size());
		   meshes->ShowVenue(visitor);

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

	   void Free()
	   {
		   delete this;
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

	   Vec2i SelectTexture(ID_TEXTURE id) override
	   {
		   TextureBind t = textureManager->GetTexture(id);
		   
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
		   auto depth = textureManager->GetTexture(depthId).texture;

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
		   mainDepthBufferId = textureManager->CreateDepthTarget(scdt_name, rect.right - rect.left, rect.bottom - rect.top);
		   shadowBufferId = textureManager->CreateDepthTarget("ShadowBuffer", 1024, 1024);

		   ExpandViewportToEntireTexture(mainDepthBufferId);
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
		   auto& m = meshes->GetBuffer(id);
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
			  // UseShaders(currentVertexShaderId, currentPixelShaderId);	 
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

	   bool builtFirstPass = false;

	   void RenderParticles(std::vector<ParticleVertex>& particles, ID_PIXEL_SHADER psID, ID_VERTEX_SHADER vsID, ID_GEOMETRY_SHADER gsID)
	   {
		   if (particles.empty()) return;
		   if (!UseShaders(vsID, psID)) return;
		   if (!Shaders().UseGeometryShader(gsID)) return;

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

		   Shaders().UseGeometryShader(ID_GEOMETRY_SHADER::Invalid());
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
		   auto shadowBind = textureManager->GetTexture(shadowBuffer);

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

			   auto shadowBind = textureManager->GetTexture(phaseConfig.shadowBuffer);

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

		   rt.depthView = textureManager->GetTexture(phaseConfig.depthTarget).depthView;

		   if (phaseConfig.renderTarget)
		   {
			   rt.renderTargetView = textureManager->GetTexture(phaseConfig.renderTarget).renderView;
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
			   auto& mesh = meshes->GetBuffer(skyMeshId);
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

		   if (textureManager->GetTexture(phaseConfig.shadowBuffer).depthView == nullptr)
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

		   TextureBind& t = textureManager->GetTexture(mainDepthBufferId);
		   t.depthView->Release();
		   t.shaderView->Release();
		   t.texture->Release();
		   t = CreateDepthTarget(device, rect.right - rect.left, rect.bottom - rect.top);
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

