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

   using namespace Rococo::Textures;

   class DX11AppRenderer :
	   public IDX11Renderer,
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

	   AutoFree<IDX11Gui> gui;

	   AutoRelease<ID3D11Texture2D> fontTexture;
	   AutoRelease<ID3D11ShaderResourceView> fontBinding;

	   BoneMatrices boneMatrices = { 0 };
	   AutoRelease<ID3D11Buffer> boneMatricesStateBuffer;

	   OS::ticks lastTick;

	   ID_TEXTURE mainDepthBufferId;

	   RAWMOUSE lastMouseEvent;
	   Vec2i screenSpan;

	   AutoRelease<ID3D11ShaderResourceView> envMap;

	   AutoFree<IDX11Pipeline> pipeline;

	   AutoRelease<ID3D11Buffer> textureDescBuffer;
	   AutoRelease<ID3D11Buffer> globalStateBuffer;
	   AutoRelease<ID3D11Buffer> sunlightStateBuffer;

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

	   ID3D11SamplerState* samplers[16] = { 0 };
   public:
	   Windows::IWindow& window;
	   bool isBuildingAlphaBlendedSprites{ false };

	   DX11AppRenderer(DX11::Factory& _factory, Windows::IWindow& _window) :
		   installation(_factory.installation), 
		   device(_factory.device), dc(_factory.dc), factory(_factory.factory),
		   scratchBuffer(CreateExpandingBuffer(64_kilobytes)),
		   window(_window),
		   textureManager(CreateTextureManager(installation, device, dc)),
		   meshes(CreateMeshManager(device)),
		   shaders(CreateShaderManager(installation, device, dc)),
		   pipeline(CreateDX11Pipeline(installation, *shaders, *textureManager, *meshes, *this, *this, device, dc))
	   {
		   DX11::TextureBind fb = textureManager->Loader().LoadAlphaBitmap("!font1.tif");
		   fontTexture = fb.texture;
		   fontBinding = fb.shaderView;

		   boneMatricesStateBuffer = DX11::CreateConstantBuffer<BoneMatrices>(device);
		
		   gui = CreateDX11Gui(*this, *this, *this, device, dc);

		   textureDescBuffer = DX11::CreateConstantBuffer<TextureDescState>(device);

		   lastTick = OS::CpuTicks();

		   globalStateBuffer = DX11::CreateConstantBuffer<GlobalState>(device);
		   sunlightStateBuffer = DX11::CreateConstantBuffer<Vec4>(device);

		   RGBA red{ 1.0f, 0, 0, 1.0f };
		   RGBA transparent{ 0.0f, 0, 0, 0.0f };

		   SetSampler(TXUNIT_FONT,	 Filter_Linear, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		   SetSampler(TXUNIT_SHADOW, Filter_Linear, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		   SetSampler(TXUNIT_ENV_MAP,   Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		   SetSampler(TXUNIT_SELECT, Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		   SetSampler(TXUNIT_MATERIALS, Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		   SetSampler(TXUNIT_SPRITES,   Filter_Point, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		   SetSampler(TXUNIT_GENERIC_TXARRAY, Filter_Point, AddressMode_Border, AddressMode_Border, AddressMode_Border, transparent);
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
		   return textureManager->Materials();
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

	   IGui3D& Gui3D() override
	   {
		   return pipeline->Gui3D();
	   }

	   ICubeTextures& CubeTextures() override
	   {
		   return textureManager->CubeTextures();
	   }

	   void CaptureMouse(bool enable) override
	   {
		   if (enable) SetCapture(this->window);
		   else ReleaseCapture();
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
		   textureManager->Materials().ShowVenue(visitor);
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

		   pipeline->ShowVenue(visitor);
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

	   void ExpandViewportToEntireTexture(ID_TEXTURE depthId) override
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

	   ID3D11RenderTargetView* BackBuffer()
	   {
		   return mainBackBufferView;
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

	   void Draw(ID_SYS_MESH id, const ObjectInstance* instances, uint32 nInstances) override
	   {
		   auto& m = meshes->GetBuffer(id);
		   pipeline->Draw(m, instances, nInstances);
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

	   IParticles& Particles()
	   {
		   return pipeline->Particles();
	   }

	   void RenderGui(IScene& scene) override
	   {
		   OS::ticks now = OS::CpuTicks();

		   GuiMetrics metrics;
		   GetGuiMetrics(metrics);
		   gui->RenderGui(scene, metrics, pipeline->IsGuiReady());

		   gui->FlushLayer();

		   if (pipeline->IsGuiReady())
		   {
			   DrawCursor();
			   gui->FlushLayer();
		   }

		   guiCost = OS::CpuTicks() - now;
	   }

	   void AssignGlobalStateBufferToShaders() override
	   {
		   dc.PSSetConstantBuffers(0, 1, &globalStateBuffer);
		   dc.VSSetConstantBuffers(0, 1, &globalStateBuffer);
	   }

	   void InitFontAndMaterialAndSpriteShaderResourceViewsAndSamplers()
	   {
		   dc.PSSetShaderResources(TXUNIT_FONT, 1, &fontBinding);
		   auto* view = textureManager->GetCubeShaderResourceView();
		   dc.PSSetShaderResources(TXUNIT_ENV_MAP, 1, &view);

		   ID3D11ShaderResourceView* materialViews[1] = { textureManager->Materials().Textures().View()};
		   dc.PSSetShaderResources(TXUNIT_MATERIALS, 1, materialViews);

		   ID3D11ShaderResourceView* spriteviews[1] = { gui->SpriteView() };
		   dc.PSSetShaderResources(TXUNIT_SPRITES, 1, spriteviews);

		   dc.PSSetSamplers(0, 16, samplers);
		   dc.GSSetSamplers(0, 16, samplers);
		   dc.VSSetSamplers(0, 16, samplers);
	   }

	   GlobalState currentGlobalState = { 0 };

	   void UpdateGlobalState(IScene& scene) override
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

	   void RestoreSamplers()
	   {
		   dc.PSSetSamplers(0, 16, samplers);
	   }

	   void Render(Graphics::ENVIRONMENTAL_MAP envMap, IScene& scene) override
	   {
		   if (mainBackBufferView.IsNull()) return;

		   auto now = OS::CpuTicks();
		   AIcost = now - lastTick;

		   pipeline->Render(envMap, scene);

		   RenderGui(scene);

		   now = OS::CpuTicks();

		   mainSwapChain->Present(1, 0);

		   presentCost = OS::CpuTicks() - now;

		   DetachContext();

		   now = OS::CpuTicks();
		   frameTime = now - lastTick;
		   lastTick = now;
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

	   ID_TEXTURE GetMainDepthBufferId() const override
	   {
		   return mainDepthBufferId;
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

