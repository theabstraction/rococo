#include "dx11.renderer.h"
#include <rococo.renderer.h>

#include "rococo.dx11.api.h"

#include <vector>
#include <algorithm>

#include <rococo.imaging.h>
#include <rococo.fonts.h>
#include <rococo.maths.h>
#include <rococo.io.h>

#include <unordered_map>

#include <rococo.dx11.renderer.win32.h>

#include "dx11helpers.inl"
#include "dx11buffers.inl"

#include "rococo.textures.h"

#include "rococo.visitors.h"

#include <random>

#include <Dxgi1_3.h>
#include <comdef.h>

#include "dx11.factory.h"

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Windows;
	using namespace Rococo::Samplers;

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

	class SpanEvaluator : public Fonts::IGlyphRenderer
	{
	public:
		GuiRectf renderZone;

		SpanEvaluator() : renderZone(10000, 10000, -10000, -10000)
		{

		}

		virtual void DrawGlyph(const Vec2& t0, const Vec2& p, float dx, float dy, Fonts::FontColour colour)
		{
			ExpandZoneToContain(renderZone, Vec2{ p.x, p.y });
			ExpandZoneToContain(renderZone, Vec2{ p.x + dx, p.y + dy });
		}

		Vec2 Span() const
		{
			return Rococo::Span(renderZone);
		}
	};

   struct Overlay
   {
      int32 zOrder;
      IUIOverlay* overlay;
   };

   bool operator < (const Overlay& a, const Overlay& b)
   {
      return a.zOrder < b.zOrder;
   }

   bool operator == (const Overlay& a, IUIOverlay* b)
   {
      return a.overlay == b;
   }

   using namespace Rococo::Textures;

   void ShowWindowVenue(HWND hWnd, IMathsVisitor& visitor)
   {
	   POINT pt;
	   GetCursorPos(&pt);

	   visitor.ShowString("Abs Mouse Cursor:", "(%d %d)", pt.x, pt.y);

	   ScreenToClient(hWnd, &pt);

	   visitor.ShowString("Rel Mouse Cursor:", "(%d %d)", pt.x, pt.y);

	   visitor.ShowPointer("HANDLE", hWnd);
	   RECT rect;
	   GetClientRect(hWnd, &rect);
	   visitor.ShowString("-> Client Rect", "(%d %d) to (%d %d)", rect.left, rect.top, rect.right, rect.bottom);

	   GetWindowRect(hWnd, &rect);
	   visitor.ShowString("-> Window Rect", "(%d %d) to (%d %d)", rect.left, rect.top, rect.right, rect.bottom);

	   LONG x = GetWindowLongA(hWnd, GWL_STYLE);
	   visitor.ShowString("-> GWL_STYLE", "0x%8.8X", x);

	   x = GetWindowLongA(hWnd, GWL_EXSTYLE);
	   visitor.ShowString("-> GWL_EXSTYLE", "0x%8.8X", x);

	   HWND hParent = GetParent(hWnd);
	   if (hParent) visitor.ShowPointer("-> Parent", hParent);
	   else			visitor.ShowString("-> Parent", "None (top-level window)");
   }

   struct DX11TextureArray : public ITextureArray
   {
      DX11::TextureBind tb;
      int32 width{ 0 };
      ID3D11Device& device;
      ID3D11DeviceContext& dc;

      size_t arrayCapacity{ 0 };
      size_t count{ 0 };

      ID3D11ShaderResourceView* View()
      {
         if (tb.shaderView == nullptr)
         {
            if (tb.texture != nullptr)
            {
               D3D11_SHADER_RESOURCE_VIEW_DESC desc;
               ZeroMemory(&desc, sizeof(desc));

               desc.Texture2DArray.MipLevels = -1;
               desc.Texture2DArray.FirstArraySlice = 0;
               desc.Texture2DArray.ArraySize = (UINT)count;
               desc.Texture2DArray.MostDetailedMip = 0;

               desc.Format = DXGI_FORMAT_UNKNOWN;
               desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;

               ID3D11ShaderResourceView* view = nullptr;
               VALIDATEDX11(device.CreateShaderResourceView(tb.texture, &desc, &view));
               tb.shaderView = view;
            }
         }

         return tb.shaderView;
      }

      DX11TextureArray(ID3D11Device& _device, ID3D11DeviceContext& _dc) :
         device(_device), dc(_dc)
      {
      }

      ~DX11TextureArray()
      {
         Clear();
      }

      void Clear()
      {
         if (tb.texture) tb.texture->Release();
         if (tb.shaderView) tb.shaderView->Release();
         tb.texture = nullptr;
         tb.shaderView = nullptr;
         count = 0;
         arrayCapacity = 0;
      }

      virtual void AddTexture()
      {
         if (arrayCapacity != 0)
         {
            Throw(0, "DX11TextureArray texture is already defined. You cannot add more textures.");
         }
         count++;
      }

      virtual void ResetWidth(int32 width)
      {
         Clear();
         this->width = width;
      }

	  void Resize(size_t nElements)
	  {
		  Clear();
		  arrayCapacity = count = nElements;
	  }

      virtual void WriteSubImage(size_t index, const RGBAb* pixels, const GuiRect& targetLocation)
      {
         if (width > 0 && tb.texture == nullptr)
         {
            arrayCapacity = count;

            D3D11_TEXTURE2D_DESC colourSpriteArray;
            colourSpriteArray.Width = width;
            colourSpriteArray.Height = width;
            colourSpriteArray.MipLevels = 1;
            colourSpriteArray.ArraySize = (UINT)arrayCapacity;
            colourSpriteArray.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            colourSpriteArray.SampleDesc.Count = 1;
            colourSpriteArray.SampleDesc.Quality = 0;
            colourSpriteArray.Usage = D3D11_USAGE_DEFAULT;
            colourSpriteArray.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            colourSpriteArray.CPUAccessFlags = 0;
            colourSpriteArray.MiscFlags = 0;
            VALIDATEDX11(device.CreateTexture2D(&colourSpriteArray, nullptr, &tb.texture));
         }

         if (width > 0)
         {
            UINT subresourceIndex = D3D11CalcSubresource(0, (UINT)index, 1);
            Vec2i span = Span(targetLocation);
            D3D11_BOX box;
            box.left = targetLocation.left;
            box.right = targetLocation.right;
            box.back = 1;
            box.front = 0;
            box.top = targetLocation.top;
            box.bottom = targetLocation.bottom;

            UINT srcDepth = span.x * span.y * sizeof(RGBAb);
            dc.UpdateSubresource(tb.texture, subresourceIndex, &box, pixels, span.x * sizeof(RGBAb), srcDepth);
         }
      }

      virtual int32 MaxWidth() const
      {
         return 1024;
      }

      virtual size_t TextureCount() const
      {
         return count;
      }
   };

   bool PrepareDepthRenderFromLight(const Light& light, DepthRenderData& drd)
   {
	   if (!TryNormalize(light.direction, drd.direction))
	   {
		   return false;
	   }

	   drd.direction.w = 0;
	   drd.eye = Vec4::FromVec3(light.position, 1.0f);
	   drd.fov = light.fov;

	   Matrix4x4 directionToCameraRot = RotateDirectionToNegZ(drd.direction);

	   Matrix4x4 cameraToDirectionRot = TransposeMatrix(directionToCameraRot);
	   drd.right = cameraToDirectionRot * Vec4 { 1, 0, 0, 0 };
	   drd.up = cameraToDirectionRot * Vec4 { 0, 1, 0, 0 };

	   drd.worldToCamera = directionToCameraRot * Matrix4x4::Translate(-drd.eye);

	   drd.nearPlane = light.nearPlane;
	   drd.farPlane = light.farPlane;

	   Matrix4x4 cameraToScreen = Matrix4x4::GetRHProjectionMatrix(drd.fov, 1.0f, drd.nearPlane, drd.farPlane);

	   drd.worldToScreen = cameraToScreen * drd.worldToCamera;

	   OS::ticks t = OS::CpuTicks();
	   OS::ticks ticksPerSecond = OS::CpuHz();

	   OS::ticks oneMinute = ticksPerSecond * 60;

	   OS::ticks secondOfMinute = t % oneMinute;
	    
	   drd.time = Seconds{ (secondOfMinute / (float)ticksPerSecond) * 0.9999f };

	   return true;
   }

   class DX11AppRenderer :
	   public IRenderer,
	   public IRenderContext,
	   public IGuiRenderContext,
	   public Fonts::IGlyphRenderer,
	   public IResourceLoader,
	   public IMathsVenue
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

	   DX11TextureArray spriteArray;
	   DX11TextureArray materialArray;

	   AutoFree<ITextureArrayBuilderSupervisor> spriteArrayBuilder;

	   AutoRelease<IDXGISwapChain> mainSwapChain;
	   AutoRelease<ID3D11RenderTargetView> mainBackBufferView;

	   ID_TEXTURE mainDepthBufferId;
	   ID_TEXTURE shadowBufferId;

	   std::vector<DX11VertexShader*> vertexShaders;
	   std::vector<DX11PixelShader*> pixelShaders;
	   std::vector<DX11GeometryShader*> geometryShaders;

	   AutoRelease<ID3D11Buffer> guiBuffer;
	   AutoRelease<ID3D11Buffer> particleBuffer;
	   std::vector<GuiVertex> guiVertices;

	   enum { GUI_BUFFER_VERTEX_CAPACITY = 3 * 512 };
	   enum { PARTICLE_BUFFER_VERTEX_CAPACITY = 1024 };

	   AutoFree<Fonts::IFontSupervisor> fonts;
	   AutoRelease<ID3D11Texture2D> fontTexture;
	   AutoRelease<ID3D11ShaderResourceView> fontBinding;

	   AutoRelease<ID3D11RasterizerState> spriteRaterizering;
	   AutoRelease<ID3D11RasterizerState> objectRaterizering;
	   AutoRelease<ID3D11RasterizerState> particleRaterizering;
	   AutoRelease<ID3D11RasterizerState> shadowRaterizering;

	   AutoRelease<ID3D11BlendState> alphaBlend;
	   AutoRelease<ID3D11BlendState> alphaAdditiveBlend;
	   AutoRelease<ID3D11BlendState> disableBlend;
	   AutoRelease<ID3D11BlendState> additiveBlend;
	   AutoRelease<ID3D11BlendState> plasmaBlend;

	   AutoRelease<ID3D11Buffer> globalStateBuffer;
	   AutoRelease<ID3D11Buffer> depthRenderStateBuffer;
	   AutoRelease<ID3D11Buffer> lightStateBuffer;
	   AutoRelease<ID3D11Buffer> textureDescBuffer;
	   AutoRelease<ID3D11Buffer> ambientBuffer;

	   AutoRelease<ID3D11Texture2D> cubeTexture;
	   AutoRelease<ID3D11ShaderResourceView> cubeTextureView;

	   RAWMOUSE lastMouseEvent;
	   Vec2i screenSpan;

	   ID_VERTEX_SHADER idGuiVS;
	   ID_PIXEL_SHADER idGuiPS;

	   ID_VERTEX_SHADER idObjVS;
	   ID_PIXEL_SHADER idObjPS;
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
	   ID_PIXEL_SHADER idObjPS_Shadows;

	   AutoRelease<ID3D11Buffer> instanceBuffer;

	   AutoRelease<ID3D11DepthStencilState> guiDepthState;
	   AutoRelease<ID3D11DepthStencilState> objDepthState;
	   AutoRelease<ID3D11DepthStencilState> objDepthState_NoWrite;

	   AutoRelease<ID3D11ShaderResourceView> envMap;

	   std::vector<DX11::TextureBind> textures;
	   std::unordered_map<std::string, ID_TEXTURE> mapNameToTexture;
	   std::unordered_map<std::string, MaterialId> nameToMaterialId;
	   std::vector<std::string> idToMaterialName;

	   std::vector<Overlay> overlays;

	   struct Cursor
	   {
		   BitmapLocation sprite;
		   Vec2i hotspotOffset;
	   } cursor;

	   ID_TEXTURE lastTextureId;

	   AutoFree<IExpandingBuffer> scratchBuffer;
	   DX11::TextureLoader textureLoader;

	   virtual void Load(cstr name, IEventCallback<CompressedTextureBuffer>& onLoad)
	   {
		   StandardLoadFromCompressedTextureBuffer(name, onLoad, installation, *scratchBuffer);
	   }

	   std::unordered_map<std::string, ID_PIXEL_SHADER> nameToPixelShader;

	   std::vector<ParticleVertex> fog;
	   std::vector<ParticleVertex> plasma;

	   ID3D11SamplerState* samplers[16] = { 0 };
   public:
	   Windows::IWindow& window;
	   bool isBuildingAlphaBlendedSprites{ false };

	   DX11AppRenderer(DX11::Factory& _factory, Windows::IWindow& _window) :
		   device(_factory.device), dc(_factory.dc), factory(_factory.factory),
		   cursor{ BitmapLocation { {0,0,0,0}, -1 }, { 0,0 } }, installation(_factory.installation),
		   spriteArray(_factory.device, dc),
		   materialArray(_factory.device, dc),
		   spriteArrayBuilder(CreateTextureArrayBuilder(*this, spriteArray)),
		   scratchBuffer(CreateExpandingBuffer(16_kilobytes)),
		   textureLoader(_factory.installation, _factory.device, _factory.dc, *scratchBuffer),
		   window(_window)
	   {
		   static_assert(GUI_BUFFER_VERTEX_CAPACITY % 3 == 0, "Capacity must be divisible by 3");
		   guiBuffer = DX11::CreateDynamicVertexBuffer<GuiVertex>(device, GUI_BUFFER_VERTEX_CAPACITY);
		   particleBuffer = DX11::CreateDynamicVertexBuffer<ParticleVertex>(device, PARTICLE_BUFFER_VERTEX_CAPACITY);

		   objDepthState = DX11::CreateObjectDepthStencilState(device);
		   objDepthState_NoWrite = DX11::CreateObjectDepthStencilState_NoWrite(device);
		   guiDepthState = DX11::CreateGuiDepthStencilState(device);

		   spriteRaterizering = DX11::CreateSpriteRasterizer(device);
		   objectRaterizering = DX11::CreateObjectRasterizer(device);
		   particleRaterizering = DX11::CreateParticleRasterizer(device);
		   shadowRaterizering = DX11::CreateShadowRasterizer(device);

		   alphaBlend = DX11::CreateAlphaBlend(device);
		   alphaAdditiveBlend = DX11::CreateAlphaAdditiveBlend(device);
		   disableBlend = DX11::CreateNoBlend(device);
		   additiveBlend = DX11::CreateAdditiveBlend(device);
		   plasmaBlend = DX11::CreatePlasmaBlend(device);

		   DX11::TextureBind fb = textureLoader.LoadAlphaBitmap("!font1.tif");
		   fontTexture = fb.texture;
		   fontBinding = fb.shaderView;

		   cstr csvName = "!font1.csv";
		   installation.LoadResource(csvName, *scratchBuffer, 256_kilobytes);
		   fonts = Fonts::LoadFontCSV(csvName, (const char*)scratchBuffer->GetData(), scratchBuffer->Length());

		   globalStateBuffer = DX11::CreateConstantBuffer<GlobalState>(device);
		   depthRenderStateBuffer = DX11::CreateConstantBuffer<DepthRenderData>(device);
		   lightStateBuffer = DX11::CreateConstantBuffer<Light>(device);
		   textureDescBuffer = DX11::CreateConstantBuffer<TextureDescState>(device);
		   ambientBuffer = DX11::CreateConstantBuffer<AmbientData>(device);

		   installation.LoadResource("!gui.vs", *scratchBuffer, 64_kilobytes);
		   idGuiVS = CreateGuiVertexShader("!gui.vs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!gui.ps", *scratchBuffer, 64_kilobytes);
		   idGuiPS = CreatePixelShader("!gui.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!object.vs", *scratchBuffer, 64_kilobytes);
		   idObjVS = CreateObjectVertexShader("!object.vs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!object.ps", *scratchBuffer, 64_kilobytes);
		   idObjPS = CreatePixelShader("!object.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!particle.vs", *scratchBuffer, 64_kilobytes);
		   idParticleVS = CreateParticleVertexShader("!particle.vs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!plasma.gs", *scratchBuffer, 64_kilobytes);
		   idPlasmaGS = CreateGeometryShader("!plasma.gs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!fog.spotlight.gs", *scratchBuffer, 64_kilobytes);
		   idFogSpotlightGS = CreateGeometryShader("!fog.spotlight.gs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!fog.ambient.gs", *scratchBuffer, 64_kilobytes);
		   idFogAmbientGS = CreateGeometryShader("!fog.ambient.gs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!obj.spotlight.no_env.ps", *scratchBuffer, 64_kilobytes);
		   idObj_Spotlight_NoEnvMap_PS = CreatePixelShader("!obj.spotlight.no_env.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!obj.ambient.no_env.ps", *scratchBuffer, 64_kilobytes);
		   idObj_Ambient_NoEnvMap_PS = CreatePixelShader("!obj.ambient.no_env.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!plasma.ps", *scratchBuffer, 64_kilobytes);
		   idPlasmaPS = CreatePixelShader("!plasma.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!fog.spotlight.ps", *scratchBuffer, 64_kilobytes);
		   idFogSpotlightPS = CreatePixelShader("!fog.spotlight.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!fog.ambient.ps", *scratchBuffer, 64_kilobytes);
		   idFogAmbientPS = CreatePixelShader("!fog.ambient.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!ambient.ps", *scratchBuffer, 64_kilobytes);
		   idObjAmbientPS = CreatePixelShader("!ambient.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!ambient.vs", *scratchBuffer, 64_kilobytes);
		   idObjAmbientVS = CreateObjectVertexShader("!ambient.vs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!shadow.vs", *scratchBuffer, 64_kilobytes);
		   idObjVS_Shadows = CreateObjectVertexShader("!shadow.vs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!shadow.ps", *scratchBuffer, 64_kilobytes);
		   idObjPS_Shadows = CreatePixelShader("!shadow.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   instanceBuffer = DX11::CreateConstantBuffer<ObjectInstance>(device);

		   lastTick = OS::CpuTicks();
		   rng.seed(123456U);
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
	   }

	   void AddFog(const ParticleVertex& p) override
	   {
		   fog.push_back(p);
	   }

	   void AddPlasma(const ParticleVertex& p) override
	   {
		   plasma.push_back(p);
	   }

	   void ClearPlasma() override
	   {
		   plasma.clear();
	   }

	   void ClearFog() override
	   {
		   fog.clear();
	   }

	   Fonts::IFont& FontMetrics() override
	   {
		   return *fonts;
	   }

	   D3D11_TEXTURE_ADDRESS_MODE From(AddressMode mode)
	   {
		   switch (mode)
		   {
		   case AddressMode_Border:
			   return D3D11_TEXTURE_ADDRESS_BORDER;
		   case AddressMode_Wrap:
			   return D3D11_TEXTURE_ADDRESS_WRAP;
		   case AddressMode_Mirror:
			   return D3D11_TEXTURE_ADDRESS_MIRROR;
		   default:
			   return D3D11_TEXTURE_ADDRESS_CLAMP;
		   }
	   }

	   void SetSampler(uint32 index, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour)
	   {
		   if (index >= 16) Throw(0, "DX11Renderer::SetSampler(%d, ...): Maximum index is 15");

		   if (samplers[index])
		   {
			   samplers[index]->Release();
		   }

		   D3D11_SAMPLER_DESC desc;

		   switch (filter)
		   {
		   case Filter_Point:
			   desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			   break;
		   default:
			   desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			   break;
		   }

		   desc.AddressU = From(u);
		   desc.AddressV = From(v);
		   desc.AddressW = From(w);
		   desc.BorderColor[0] = borderColour.red;
		   desc.BorderColor[1] = borderColour.green;
		   desc.BorderColor[2] = borderColour.blue;
		   desc.BorderColor[3] = borderColour.alpha;
		   desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		   desc.MaxAnisotropy = 1;
		   desc.MipLODBias = 0;
		   desc.MaxLOD = D3D11_FLOAT32_MAX;
		   desc.MinLOD = 0;

		   VALIDATEDX11(device.CreateSamplerState(&desc, &samplers[index]));
		   dc.PSSetSamplers(index, 1, &samplers[index]);
	   }

	   int32 cubeMaterialId[6] = { -1,-1,-1,-1,-1,-1 };

	   /* 6 material ids determine the environmental texture */
	   void SyncCubeTexture(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace) override
	   {
		   int32 newMaterialids[6] = { XMaxFace, XMinFace, YMaxFace, YMinFace, ZMaxFace, ZMinFace };

		   if (cubeTexture)
		   {
			   D3D11_TEXTURE2D_DESC desc;
			   cubeTexture->GetDesc(&desc);

			   if (desc.Width != materialArray.width)
			   {
				   cubeTexture = nullptr;
				   cubeTextureView = nullptr;
			   }
		   }

		   if (!cubeTexture)
		   {
			   D3D11_TEXTURE2D_DESC desc;
			   ZeroMemory(&desc, sizeof(desc));
			   desc.Width = materialArray.width;
			   desc.Height = materialArray.width;
			   desc.ArraySize = 6;
			   desc.SampleDesc.Count = 1;
			   desc.SampleDesc.Quality = 0;
			   desc.MipLevels = 1;
			   desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			   desc.Usage = D3D11_USAGE_DEFAULT;
			   desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			   desc.CPUAccessFlags = 0;
			   desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			   ID3D11Texture2D* pTexCube = nullptr;
			   VALIDATEDX11(device.CreateTexture2D(&desc, nullptr, &pTexCube));
			   cubeTexture = pTexCube;

			   ID3D11ShaderResourceView* view = nullptr;
			   VALIDATEDX11(device.CreateShaderResourceView(pTexCube, nullptr, &view));

			   cubeTextureView = view;
		   }

		   for (UINT i = 0; i < 6; ++i)
		   {
			   if (cubeMaterialId[i] != newMaterialids[i])
			   {
				   cubeMaterialId[i] = newMaterialids[i];

				   D3D11_BOX srcbox;
				   srcbox.left = 0;
				   srcbox.top = 0;
				   srcbox.front = 0;
				   srcbox.right = materialArray.width;
				   srcbox.bottom = materialArray.width;
				   srcbox.back = 1;

				   dc.CopySubresourceRegion(cubeTexture, i, 0, 0, 0, materialArray.tb.texture, cubeMaterialId[i], &srcbox);
			   }
		   }
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

		   D3D11_TEXTURE2D_DESC edesc;
		   t.texture->GetDesc(&edesc);

		   desc.width = edesc.Width;
		   desc.height = edesc.Height;
		   
		   switch(edesc.Format)
		   {
		   case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			   desc.format = TextureFormat_RGBA_32_BIT;
			   break;
		   case DXGI_FORMAT_R32_TYPELESS:
			   desc.format = TextureFormat_32_BIT_FLOAT;
			   break;
		   default:
			   desc.format = TextureFormat_UNKNOWN;
		   }

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

		   for (size_t i = 0; i < materialArray.TextureCount(); ++i)
		   {	   
			   char name[64];
			   SafeFormat(name, 64, "MatId %u", i);
			   visitor.ShowSelectableString("overlay.select.material", name, "  %s", idToMaterialName[i].c_str());
		   }
	   }

	   virtual cstr GetMaterialTextureName(MaterialId id) const
	   {
		   size_t index = (size_t)id;
		   if (index >= materialArray.TextureCount()) return nullptr;
		   return idToMaterialName[index].c_str();
	   }

	   virtual void GetMaterialArrayMetrics(MaterialArrayMetrics& metrics) const
	   {
		   metrics.NumberOfElements = (int32) materialArray.TextureCount();
		   metrics.Width = materialArray.MaxWidth();
	   }

	   virtual void GetMeshDesc(char desc[256], ID_SYS_MESH id)
	   {
		   if (!id || id.value >= meshBuffers.size())
		   {
			   SafeFormat(desc, 256, "invalid id");
		   }
		   else
		   {
			   auto& m = meshBuffers[id.value];

			   if (m.dx11Buffer)
			   {
				   D3D11_BUFFER_DESC bdesc;
				   m.dx11Buffer->GetDesc(&bdesc);
				   SafeFormat(desc, 256, " %p %6d vertices. %6u bytes", m.dx11Buffer, m.numberOfVertices, bdesc.ByteWidth);
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

		   UINT flags = device.GetCreationFlags();

		   if (flags & D3D11_CREATE_DEVICE_SINGLETHREADED)
		   {
			   visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_SINGLETHREADED");
		   }

		   if (flags & D3D11_CREATE_DEVICE_DEBUG)
		   {
			   visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_DEBUG");
		   }

		   if (flags & D3D11_CREATE_DEVICE_SWITCH_TO_REF)
		   {
			   visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_SWITCH_TO_REF");
		   }

		   if (flags & D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS)
		   {
			   visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS");
		   }

		   if (flags & D3D11_CREATE_DEVICE_BGRA_SUPPORT)
		   {
			   visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_BGRA_SUPPORT");
		   }

		   if (flags & D3D11_CREATE_DEVICE_DEBUGGABLE)
		   {
			   visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_DEBUGGABLE");
		   }

		   if (flags & D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY)
		   {
			   visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY");
		   }

		   if (flags & D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT)
		   {
			   visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT");
		   }

		   if (flags & D3D11_CREATE_DEVICE_VIDEO_SUPPORT)
		   {
			   visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_VIDEO_SUPPORT");
		   }

		   D3D11_RENDER_TARGET_VIEW_DESC desc;
		   mainBackBufferView->GetDesc(&desc);

		   visitor.ShowString("BackBuffer format", "%u", desc.Format);

		   D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc;
		   GetTexture(mainDepthBufferId).depthView->GetDesc(&dsDesc);

		   visitor.ShowString("DepthStencil format", "%u", dsDesc.Format);

		   visitor.ShowDecimal("Number of textures", (int64)textures.size());
		   visitor.ShowDecimal("Number of meshes", (int64)meshBuffers.size());
		   visitor.ShowDecimal("Mesh updates", meshUpdateCount);

		   visitor.ShowString("Cursor bitmap", "Id: %d. {%d,%d}-{%d,%d}", cursor.sprite.textureIndex, cursor.sprite.txUV.left, cursor.sprite.txUV.top, cursor.sprite.txUV.right, cursor.sprite.txUV.bottom );
		   visitor.ShowString("Cursor hotspot delta", "(%+d %+d)", cursor.hotspotOffset.x, cursor.hotspotOffset.y);

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
		   ANON::ShowWindowVenue(window, visitor);
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
					   SafeFormat(lastError, sizeof(lastError), "device.CreatePixelShader for %s returned 0x%X", pingPath, hr);
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

	   virtual ITextureArrayBuilder& SpriteBuilder()
	   {
		   return *spriteArrayBuilder;
	   }

	   virtual void ClearMeshes()
	   {
		   for (auto& x : meshBuffers)
		   {
			   if (x.dx11Buffer) x.dx11Buffer->Release();
		   }

		   meshBuffers.clear();
	   }

	   virtual void DeleteMesh(ID_SYS_MESH id)
	   {
		   if (id.value < 0 || id.value >= meshBuffers.size()) Throw(0, "DX11AppRenderer::DeleteMesh(...): Bad ID_SYS_MESH");

		   if (meshBuffers[id.value].dx11Buffer)
		   {
			   meshBuffers[id.value].dx11Buffer->Release();
			   meshBuffers[id.value].dx11Buffer = nullptr;
			   meshBuffers[id.value].numberOfVertices = 0;
		   }
	   }

	   virtual void BuildEnvironmentalMap(int32 topIndex, int32 bottomIndex, int32 leftIndex, int32 rightIndex, int frontIndex, int32 backIndex)
	   {
		   envMap = nullptr;
	   }

	   virtual IInstallation& Installation()
	   {
		   return installation;
	   }

	   virtual IRenderer& Renderer()
	   {
		   return *this;
	   }

	   virtual ID_TEXTURE LoadTexture(IBuffer& buffer, cstr uniqueName)
	   {
		   auto i = mapNameToTexture.find(uniqueName);
		   if (i != mapNameToTexture.end())
		   {
			   return i->second;
		   }

		   auto bind = textureLoader.LoadColourBitmap(uniqueName);
		   textures.push_back(bind);

		   auto id = ID_TEXTURE(textures.size());
		   mapNameToTexture.insert(std::make_pair(uniqueName, id));

		   orderedTextureList.clear();

		   return id;
	   }

	   virtual void LoadMaterialTextureArray(IMaterialTextureArrayBuilder& builder)
	   {
		   int32 txWidth = builder.TexelWidth();
		   materialArray.ResetWidth(builder.TexelWidth());
		   materialArray.Resize(builder.Count());
		   nameToMaterialId.clear();
		   idToMaterialName.clear();

		   idToMaterialName.resize(builder.Count());

		   for (size_t i = 0; i < builder.Count(); ++i)
		   {
			   struct ANON : IEventCallback<MaterialTextureArrayBuilderArgs>, Imaging::IImageLoadEvents
			   {
				   DX11AppRenderer* This;
				   size_t i;
				   int32 txWidth;
				   cstr name; // valid for duration of OnEvent callback

				   virtual void OnEvent(MaterialTextureArrayBuilderArgs& args)
				   {
					   name = args.name;
					   
					   auto ext = GetFileExtension(args.name);
					   if (EqI(ext, ".tif") || EqI(ext, ".tiff"))
					   {
						   Rococo::Imaging::DecompressTiff(*this, args.buffer.GetData(), args.buffer.Length());
					   }
					   else if (EqI(ext, ".jpg") || EqI(ext, ".jpeg"))
					   {
						   Rococo::Imaging::DecompressJPeg(*this, args.buffer.GetData(), args.buffer.Length());
					   }
					   else
					   {
						   Throw(0, "Error loading material texture: %s: Only extensions allowed are tif, tiff, jpg and jpeg", name);
					   }

					   This->nameToMaterialId[args.name] = (MaterialId)i;
					   auto t = This->nameToMaterialId.find(args.name);
					   This->idToMaterialName[i] = t->first;
				   }

				   virtual void OnError(const char* message)
				   {
					   Throw(0, "Error loading material texture: %s: %s", name, message);
				   }

				   virtual void OnRGBAImage(const Vec2i& span, const RGBAb* pixels)
				   {
					   if (span.x != txWidth || span.y != txWidth)
					   {
						   Throw(0, "Error loading texture %s. Only %d x %d dimensions supported", name, txWidth, txWidth);
					   }

					   DX11TextureArray& m = This->materialArray;
					   m.WriteSubImage(i, pixels, GuiRect{ 0, 0, txWidth, txWidth });
				   }

				   virtual void OnAlphaImage(const Vec2i& span, const uint8* data)
				   {
					   Throw(0, "Error loading texture %s. Only RGB and ARGB formats supported", name);
				   }

			   } onTexture;
			   onTexture.This = this;
			   onTexture.i = i;
			   onTexture.txWidth = txWidth;

			   builder.LoadTextureForIndex(i, onTexture);
		   }
	   }

	   MaterialId GetMaterialId(cstr name) const
	   {
		   auto i = nameToMaterialId.find(name);
		   return i != nameToMaterialId.end() ? i->second : -1.0f;
	   }

	   enum CBUFFER_INDEX
	   { 
		   CBUFFER_INDEX_GLOBAL_STATE = 0,
		   CBUFFER_INDEX_CURRENT_SPOTLIGHT = 1,
		   CBUFFER_INDEX_AMBIENT_LIGHT = 2,
		   CBUFFER_INDEX_DEPTH_RENDER_DESC = 3,
		   CBUFFER_INDEX_INSTANCE_BUFFER = 4,
		   CBUFFER_INDEX_SELECT_TEXTURE_DESC = 5
	   };

	   ID_TEXTURE FindTexture(cstr name) const
	   {
		   auto i = mapNameToTexture.find(name);
		   return i != mapNameToTexture.end() ? i->second : ID_TEXTURE::Invalid();
	   }

	   virtual auto SelectTexture(ID_TEXTURE id) -> Vec2i
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
			   FlushLayer();
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

	   void SyncViewport(ID_TEXTURE depthId)
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
		   mainDepthBufferId = CreateDepthTarget(rect.right - rect.left, rect.bottom - rect.top);
		   shadowBufferId = CreateDepthTarget(1024, 1024);

		   SyncViewport(mainDepthBufferId);
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
			   Throw(e.Error(), "device.CreateInputLayout failed for shader %s: %S. %s\n", name, msg, (cstr) e.Description());
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

	   ID_VERTEX_SHADER CreateGuiVertexShader(cstr name, const byte* shaderCode, size_t shaderLength)
	   {
		   return CreateVertexShader(name, shaderCode, shaderLength, DX11::GetGuiVertexDesc(), DX11::NumberOfGuiVertexElements());
	   }

	   virtual ID_VERTEX_SHADER CreateObjectVertexShader(cstr name, const uint8* shaderCode, size_t shaderLength)
	   {
		   return CreateVertexShader(name, shaderCode, shaderLength, DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
	   }

	   virtual ID_VERTEX_SHADER CreateParticleVertexShader(cstr name, const byte* shaderCode, size_t shaderLength)
	   {
		   return CreateVertexShader(name, shaderCode, shaderLength, DX11::GetParticleVertexDesc(), DX11::NumberOfParticleVertexElements());
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

	   ID_TEXTURE CreateDepthTarget(int32 width, int32 height) override
	   {
		   ID3D11Texture2D* tex2D = nullptr;
		   ID3D11DepthStencilView* depthView = nullptr;
		   ID3D11ShaderResourceView* srv = nullptr;

		   try
		   {
			   D3D11_TEXTURE2D_DESC desc;
			   desc.ArraySize = 1;
			   desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			   desc.CPUAccessFlags = 0;
			   desc.Format = DXGI_FORMAT_R32_TYPELESS;
			   desc.Height = height;
			   desc.Width = width;
			   desc.MipLevels = 1;
			   desc.MiscFlags = 0;
			   desc.SampleDesc.Count = 1;
			   desc.SampleDesc.Quality = 0;
			   desc.Usage = D3D11_USAGE_DEFAULT;
			   VALIDATEDX11(device.CreateTexture2D(&desc, nullptr, &tex2D));

			   D3D11_DEPTH_STENCIL_VIEW_DESC sdesc;
			   sdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			   sdesc.Texture2D.MipSlice = 0;
			   sdesc.Format = DXGI_FORMAT_D32_FLOAT;
			   sdesc.Flags = 0;
			   VALIDATEDX11(device.CreateDepthStencilView(tex2D, &sdesc, &depthView));

			   D3D11_SHADER_RESOURCE_VIEW_DESC rdesc;
			   rdesc.Texture2D.MipLevels = 1;
			   rdesc.Texture2D.MostDetailedMip = 0;
			   rdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			   rdesc.Format = DXGI_FORMAT_R32_FLOAT;
			   VALIDATEDX11(device.CreateShaderResourceView(tex2D, &rdesc, &srv));
		   }
		   catch (IException&)
		   {
			   if (tex2D) tex2D->Release();
			   if (depthView) depthView->Release();
			   if (srv) srv->Release();
			   throw;
		   }

		   textures.push_back(DX11::TextureBind{ tex2D, srv, nullptr, depthView });
		   auto id = ID_TEXTURE(textures.size());

		   char name[64];
		   SafeFormat(name, sizeof(name), "DepthTarget_%llu", id.value);

		   mapNameToTexture[name] = id;

		   return id;
	   }

	   ID_TEXTURE CreateRenderTarget(int32 width, int32 height) override
	   {
		   ID3D11Texture2D* tex2D = nullptr;
		   ID3D11ShaderResourceView* srv = nullptr;
		   ID3D11RenderTargetView* rtv = nullptr;

		   try
		   {
			   D3D11_TEXTURE2D_DESC desc;
			   desc.ArraySize = 1;
			   desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			   desc.CPUAccessFlags = 0;
			   desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
			   desc.Height = height;
			   desc.Width = width;
			   desc.MipLevels = 1;
			   desc.MiscFlags = 0;
			   desc.SampleDesc.Count = 1;
			   desc.SampleDesc.Quality = 0;
			   desc.Usage = D3D11_USAGE_DEFAULT;
			   VALIDATEDX11(device.CreateTexture2D(&desc, nullptr, &tex2D));

			   D3D11_SHADER_RESOURCE_VIEW_DESC rdesc;
			   rdesc.Texture2D.MipLevels = 1;
			   rdesc.Texture2D.MostDetailedMip = 0;
			   rdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			   rdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			   VALIDATEDX11(device.CreateShaderResourceView(tex2D, &rdesc, &srv));

			   D3D11_RENDER_TARGET_VIEW_DESC rtdesc;
			   rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			   rtdesc.Texture2D.MipSlice = 0;
			   rtdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			   VALIDATEDX11(device.CreateRenderTargetView(tex2D, &rtdesc, &rtv));
		   }
		   catch (IException&)
		   {
			   if (tex2D) tex2D->Release();
			   if (rtv) rtv->Release();
			   if (srv) srv->Release();
			   throw;
		   }

		   textures.push_back(DX11::TextureBind{ tex2D, srv, rtv });
		   auto id = ID_TEXTURE(textures.size());

		   char name[64];
		   SafeFormat(name, sizeof(name), "RenderTarget_%llu", id.value);

		   mapNameToTexture[name] = id;

		   return id;
	   }

	   ID_GEOMETRY_SHADER CreateGeometryShader(cstr name, const byte* shaderCode, size_t shaderLength)
	   {
		   if (name == nullptr || rlen(name) > 1024) Throw(0, "Bad <name> for geometry shader");
		   if (shaderCode == nullptr || shaderLength < 4 || shaderLength > 65536) Throw(0, "Bad shader code for geometry shader %s", name);

		   DX11GeometryShader* shader = new DX11GeometryShader;
		   HRESULT hr = device.CreateGeometryShader(shaderCode, shaderLength, nullptr, &shader->gs);
		   if FAILED(hr)
		   {
			   delete shader;
			   Throw(hr, "device.CreateGeometryShader failed with shader %s", name);
			   return ID_GEOMETRY_SHADER::Invalid();
		   }

		   shader->name = name;
		   geometryShaders.push_back(shader);
		   return ID_GEOMETRY_SHADER(geometryShaders.size() - 1);
	   }

	   void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect)
	   {
		   char stackBuffer[128];
		   Fonts::IGlyphRenderPipeline* pipeline = Fonts::CreateGlyphRenderPipeline(stackBuffer, sizeof(stackBuffer), *this);

		   GuiRectf qrect(-10000.0f, -10000.0f, 10000.0f, 10000.0f);

		   if (clipRect != nullptr)
		   {
			   qrect.left = (float)clipRect->left;
			   qrect.right = (float)clipRect->right;
			   qrect.top = (float)clipRect->top;
			   qrect.bottom = (float)clipRect->bottom;
		   }
		   RouteDrawTextBasic(pos, job, *fonts, *pipeline, qrect);
	   }

	   virtual void AddOverlay(int zorder, IUIOverlay* overlay)
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

		   std::sort(overlays.begin(), overlays.end());
	   }

	   virtual void RemoveOverlay(IUIOverlay* overlay)
	   {
		   auto i = std::remove(overlays.begin(), overlays.end(), overlay);
		   overlays.erase(i, overlays.end());
	   }

	   Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect)
	   {
		   char stackBuffer[128];
		   SpanEvaluator spanEvaluator;
		   Fonts::IGlyphRenderPipeline* pipeline = Fonts::CreateGlyphRenderPipeline(stackBuffer, sizeof(stackBuffer), spanEvaluator);

		   GuiRectf qrect(-10000.0f, -10000.0f, 10000.0f, 10000.0f);

		   if (clipRect != nullptr)
		   {
			   qrect.left = (float)clipRect->left;
			   qrect.right = (float)clipRect->right;
			   qrect.top = (float)clipRect->top;
			   qrect.bottom = (float)clipRect->bottom;
		   }
		   RouteDrawTextBasic(pos, job, *fonts, *pipeline, qrect);

		   Vec2i span = Quantize(spanEvaluator.Span());
		   if (span.x < 0) span.x = 0;
		   if (span.y < 0) span.y = 0;
		   return span;
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

	   bool UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid)
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

	   void SwitchToWindowMode()
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

	   virtual void GetGuiMetrics(GuiMetrics& metrics) const
	   {
		   POINT p;
		   GetCursorPos(&p);
		   ScreenToClient(window, &p);

		   metrics.cursorPosition = Vec2i{ p.x, p.y };
		   metrics.screenSpan = screenSpan;
	   }

	   struct MeshBuffer
	   {
		   ID3D11Buffer* dx11Buffer;
		   UINT numberOfVertices;
		   D3D_PRIMITIVE_TOPOLOGY topology;
		   ID_PIXEL_SHADER psSpotlightShader;
		   ID_PIXEL_SHADER psAmbientShader;
		   bool alphaBlending;
		   bool disableShadowCasting;
	   };

	   std::vector<MeshBuffer> meshBuffers;
	   int64 meshUpdateCount = 0;

	   ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices) override
	   {
		   ID3D11Buffer* meshBuffer = vertices ? DX11::CreateImmutableVertexBuffer(device, vertices, nVertices) : nullptr;
		   meshBuffers.push_back(MeshBuffer{ meshBuffer, nVertices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ID_PIXEL_SHADER(), ID_PIXEL_SHADER(), false, false});
		   int32 index = (int32)meshBuffers.size();
		   return ID_SYS_MESH(index - 1);
	   }

	   void SetSpecialShader(ID_SYS_MESH id, cstr psSpotlightPingPath, cstr psAmbientPingPath, bool alphaBlending) override
	   {
		   if (id.value < 0 || id.value >= meshBuffers.size())
		   {
			   Throw(E_INVALIDARG, "renderer.SetSpecialShader(ID_MESH id, ....) - Bad id ");
		   }

		   auto& m = meshBuffers[id.value];
		   
		   auto i = nameToPixelShader.find(psSpotlightPingPath);
		   if (i == nameToPixelShader.end())
		   {
			   installation.LoadResource(psSpotlightPingPath, *scratchBuffer, 64_kilobytes);
			   auto pxId = CreatePixelShader(psSpotlightPingPath, scratchBuffer->GetData(), scratchBuffer->Length());
			   i = nameToPixelShader.insert(std::make_pair(std::string(psSpotlightPingPath), pxId)).first;
		   }

		   m.psSpotlightShader = i->second;
		   
		   i = nameToPixelShader.find(psAmbientPingPath);
		   if (i == nameToPixelShader.end())
		   {
			   installation.LoadResource(psAmbientPingPath, *scratchBuffer, 64_kilobytes);
			   auto pxId = CreatePixelShader(psAmbientPingPath, scratchBuffer->GetData(), scratchBuffer->Length());
			   i = nameToPixelShader.insert(std::make_pair(std::string(psAmbientPingPath), pxId)).first;
		   }

		   m.psAmbientShader = i->second;
		   m.alphaBlending = alphaBlending;
	   }
	   
	   void UpdateMesh(ID_SYS_MESH id, const ObjectVertex* vertices, uint32 nVertices) override
	   {
		   if (id.value < 0 || id.value >= meshBuffers.size())
		   {
			   Throw(E_INVALIDARG, "renderer.UpdateMesh(ID_MESH id, ....) - Bad id ");
		   }

		   meshUpdateCount++;

		   ID3D11Buffer* newMesh = vertices != nullptr ? DX11::CreateImmutableVertexBuffer(device, vertices, nVertices) : nullptr;
		   meshBuffers[id.value].numberOfVertices = nVertices;

		   if (meshBuffers[id.value].dx11Buffer) meshBuffers[id.value].dx11Buffer->Release();
		   meshBuffers[id.value].dx11Buffer = newMesh;
	   }

	   enum RenderPhase
	   {
		   RenderPhase_None,
		   RenderPhase_DetermineShadowVolumes,
		   RenderPhase_DetermineSpotlight,
		   RenderPhase_DetermineAmbient
	   };

	   RenderPhase phase = RenderPhase_None;

	   virtual void Draw(ID_SYS_MESH id, const ObjectInstance* instances, uint32 nInstances)
	   {
		   if (id == ID_SYS_MESH::Invalid()) return;
		   if (id.value < 0 || id.value >= meshBuffers.size()) Throw(E_INVALIDARG, "renderer.DrawObject(ID_MESH id) - Bad id ");

		   auto& m = meshBuffers[id.value];

		   if (!m.dx11Buffer)
			   return;

		   if (phase == RenderPhase_DetermineShadowVolumes && m.disableShadowCasting)
			   return;

		   ID3D11Buffer* buffers[] = { m.dx11Buffer };

		   entitiesThisFrame += (int64) nInstances;

		   bool overrideShader = false;

		   if (m.psSpotlightShader && phase == RenderPhase_DetermineSpotlight)
		   {
			   UseShaders(currentVertexShaderId, m.psSpotlightShader);
			   overrideShader = true;
		   }
		   else if (m.psAmbientShader && phase == RenderPhase_DetermineAmbient)
		   {
			   UseShaders(currentVertexShaderId, m.psAmbientShader);
			   overrideShader = true;
		   }

		   FLOAT blendFactorUnused[] = { 0,0,0,0 };
		   if (m.alphaBlending)
		   {
			   dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			   dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
		   }

		   UINT strides[] = { sizeof(ObjectVertex) };
		   UINT offsets[]{ 0 };
		   dc.IASetPrimitiveTopology(m.topology);
		   dc.IASetVertexBuffers(0, 1, buffers, strides, offsets);

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
		   if (cursor.sprite.textureIndex >= 0)
		   {
			   GuiMetrics metrics;
			   GetGuiMetrics(metrics);

			   Vec2i span = Span(cursor.sprite.txUV);

			   float u0 = (float)cursor.sprite.txUV.left;
			   float u1 = (float)cursor.sprite.txUV.right;
			   float v0 = (float)cursor.sprite.txUV.top;
			   float v1 = (float)cursor.sprite.txUV.bottom;

			   Vec2 p{ (float)metrics.cursorPosition.x, (float)metrics.cursorPosition.y };

			   float x0 = p.x + cursor.hotspotOffset.x;
			   float x1 = x0 + (float)span.x;
			   float y0 = p.y + cursor.hotspotOffset.y;
			   float y1 = y0 + (float)span.y;

			   BaseVertexData noFont{ { 0,0 }, 0 };
			   SpriteVertexData solidColour{ 1.0f, 0.0f, 0.0f, 0.0f };

			   SpriteVertexData drawSprite{ 0, (float) cursor.sprite.textureIndex, 0, 0 };

			   GuiVertex quad[6]
			   {
				   GuiVertex{ {x0, y0}, {{u0, v0}, 0.0f}, drawSprite, RGBAb(128,0,0)},
				   GuiVertex{ {x1, y0}, {{u1, v0}, 0.0f}, drawSprite, RGBAb(0,128,0)},
				   GuiVertex{ {x1, y1}, {{u1, v1}, 0.0f}, drawSprite, RGBAb(0,0,128)},
				   GuiVertex{ {x1, y1}, {{u1, v1}, 0.0f}, drawSprite, RGBAb(128,0,0)},
				   GuiVertex{ {x0, y1}, {{u0, v1}, 0.0f}, drawSprite, RGBAb(0,128,0)},
				   GuiVertex{ {x0, y0}, {{u0, v0}, 0.0f}, drawSprite, RGBAb(0,0,128)}
			   };

			   AddTriangle(quad);
			   AddTriangle(quad + 3);

			   SetCursor(0);
		   }
		   else
		   {
			   SetCursor(LoadCursor(nullptr, IDC_ARROW));
		   }
	   }

	   int64 AIcost = 0;
	   int64 guiCost = 0;
	   int64 objCost = 0;
	   int64 presentCost = 0;
	   int64 frameTime = 0;

	   virtual void DrawCustomTexturedMesh(const GuiRect& absRect, ID_TEXTURE id, cstr pixelShader, const GuiVertex* vertices, size_t nCount)
	   {
		   if (nCount > GUI_BUFFER_VERTEX_CAPACITY) Throw(0, "DX11AppRenderer.DrawCustomTexturedRect - too many triangles. Max vertices: %d", GUI_BUFFER_VERTEX_CAPACITY);

		   FlushLayer();

		   auto i = nameToPixelShader.find(pixelShader);
		   if (i == nameToPixelShader.end())
		   {
			   installation.LoadResource(pixelShader, *scratchBuffer, 64_kilobytes);
			   auto pxId = CreatePixelShader(pixelShader, scratchBuffer->GetData(), scratchBuffer->Length());
			   i = nameToPixelShader.insert(std::make_pair(std::string(pixelShader), pxId)).first;
		   }

		   auto pxId = i->second;

		   UseShaders(idGuiVS, pxId);

		   SelectTexture(id);

		   D3D11_MAPPED_SUBRESOURCE x;
		   VALIDATEDX11(dc.Map(guiBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &x));
		   memcpy(x.pData, vertices, nCount * sizeof(GuiVertex));
		   dc.Unmap(guiBuffer, 0);

		   UINT stride = sizeof(GuiVertex);
		   UINT offset = 0;
		   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		   dc.IASetVertexBuffers(0, 1, &guiBuffer, &stride, &offset);
		   dc.Draw((UINT)nCount, 0);

		   UseShaders(idGuiVS, idGuiPS);
	   }

	   bool builtFirstPass = false;

	   void RenderParticles(std::vector<ParticleVertex>& particles, ID_PIXEL_SHADER psID, ID_VERTEX_SHADER vsID, ID_GEOMETRY_SHADER gsID)
	   {
		   if (particles.empty()) return;
		   if (!UseShaders(vsID, psID)) return;
		   if (!UseGeometryShader(gsID)) return;

		   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		   dc.RSSetState(particleRaterizering);
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

	   void RenderSpotlightLitScene(const Light& lightSubset, IScene& scene)
	   {
		   Light light = lightSubset;

		   DepthRenderData drd;
		   if (PrepareDepthRenderFromLight(light, drd))
		   {
			   float f = 1.0f / rng.max();
			   drd.randoms.x = rng() * f;
			   drd.randoms.y = rng() * f;
			   drd.randoms.z = rng() * f;
			   drd.randoms.w = rng() * f;

			   auto shadowBind = GetTexture(phaseConfig.shadowBuffer);

			   dc.OMSetRenderTargets(0, nullptr, shadowBind.depthView);

			   UseShaders(idObjVS_Shadows, idObjPS_Shadows);

			   DX11::CopyStructureToBuffer(dc, depthRenderStateBuffer, drd);
			   dc.VSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);
			   dc.PSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);

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

			   dc.RSSetState(shadowRaterizering);

			   phase = RenderPhase_DetermineShadowVolumes;
			   scene.RenderShadowPass(drd, *this);

			   RenderTarget rt = GetCurrentRenderTarget();
			   dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

			   phase = RenderPhase_DetermineSpotlight;

			   ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase);
			   UseShaders(idObjVS, idPS);

			   SyncViewport(phaseConfig.depthTarget);

			   light.randoms = drd.randoms;
			   light.time = drd.time;
			   light.right = drd.right;
			   light.up = drd.up;
			   light.worldToShadowBuffer = drd.worldToScreen;

			   DX11::CopyStructureToBuffer(dc, lightStateBuffer, light);
			   dc.VSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
			   dc.PSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
			   dc.GSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);

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

			   dc.PSSetShaderResources(2, 1, &shadowBind.shaderView);
			   dc.RSSetState(objectRaterizering);

			   scene.RenderObjects(*this);

			   dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			   RenderParticles(fog, idFogSpotlightPS, idParticleVS, idFogSpotlightGS);
			   phase = RenderPhase_None;

			   dc.OMSetDepthStencilState(objDepthState, 0);
		   }
	   }

	   void RenderGui(IScene& scene)
	   {
		   OS::ticks now = OS::CpuTicks();

		   dc.RSSetState(spriteRaterizering);

		   FLOAT blendFactorUnused[] = { 0,0,0,0 };
		   dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);
		   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		   dc.OMSetDepthStencilState(guiDepthState, 0);

		   if (!UseShaders(idGuiVS, idGuiPS))
		   {
			   Throw(0, "Error setting Gui shaders");
		   }

		   scene.RenderGui(*this);

		   if (!phaseConfig.renderTarget)
		   {
			   for (auto& o : overlays)
			   {
				   o.overlay->Render(*this);
			   }
		   }

		   FlushLayer();

		   if (!phaseConfig.renderTarget)
		   {
			   DrawCursor();
			   FlushLayer();
		   }

		   guiCost = OS::CpuTicks() - now;
	   }

	   void RenderAmbient(IScene& scene, const Light& ambientLight)
	   {
		   phase = RenderPhase_DetermineAmbient;

		   ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase);
		   if (UseShaders(idObjAmbientVS, idPS))
		   {
			   FLOAT blendFactorUnused[] = { 0,0,0,0 };
			   SyncViewport(phaseConfig.depthTarget);

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

			   dc.RSSetState(objectRaterizering);

			   AmbientData ad;
			   ad.localLight = ambientLight.ambient;
			   ad.fogConstant = ambientLight.fogConstant;

			   DX11::CopyStructureToBuffer(dc, ambientBuffer, ad);
			   dc.PSSetConstantBuffers(CBUFFER_INDEX_AMBIENT_LIGHT, 1, &ambientBuffer);

			   scene.RenderObjects(*this);
			   dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			   RenderParticles(fog, idFogAmbientPS, idParticleVS, idFogAmbientGS);
		   }
		   
		   phase = RenderPhase_None;
	   }

	   void SetAndClearRenderBuffers(const RGBA& clearColour)
	   {
		   RenderTarget rt = GetCurrentRenderTarget();
		   dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

		   dc.ClearDepthStencilView(rt.depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		   if (clearColour.alpha > 0)
		   {
			   dc.ClearRenderTargetView(rt.renderTargetView, (const FLOAT*)&clearColour);
		   }
	   }

	   void InitInvariantTextureViews()
	   {
		   dc.PSSetShaderResources(0, 1, &fontBinding);
		   dc.PSSetShaderResources(3, 1, &cubeTextureView);

		   ID3D11ShaderResourceView* materials[1] = { materialArray.View() };
		   dc.PSSetShaderResources(6, 1, materials);

		   ID3D11ShaderResourceView* spriteviews[1] = { spriteArray.View() };
		   dc.PSSetShaderResources(7, 1, spriteviews);

		   dc.PSSetSamplers(0, 16, samplers);
		   dc.GSSetSamplers(0, 16, samplers);
		   dc.VSSetSamplers(0, 16, samplers);
	   }

	   void UpdateGlobalState(IScene& scene)
	   {
		   GlobalState g;
		   scene.GetCamera(g.worldMatrixAndProj, g.worldMatrix, g.eye, g.viewDir);

		   float aspectRatio = screenSpan.y / (float)screenSpan.x;
		   g.aspect = { aspectRatio,0,0,0 };

		   g.guiScale.OOScreenWidth = 1.0f / screenSpan.x;
		   g.guiScale.OOScreenHeight = 1.0f / screenSpan.y;
		   g.guiScale.OOFontWidth = fonts->TextureSpan().z;
		   g.guiScale.OOSpriteWidth = 1.0f / spriteArray.width;

		   DX11::CopyStructureToBuffer(dc, globalStateBuffer, g);

		   dc.VSSetConstantBuffers(CBUFFER_INDEX_GLOBAL_STATE, 1, &globalStateBuffer);
		   dc.PSSetConstantBuffers(CBUFFER_INDEX_GLOBAL_STATE, 1, &globalStateBuffer);
		   dc.GSSetConstantBuffers(CBUFFER_INDEX_GLOBAL_STATE, 1, &globalStateBuffer);
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

	   void Render(Graphics::RenderPhaseConfig& config, IScene& scene) override
	   {
		   phaseConfig = config;

		   if (!phaseConfig.shadowBuffer)
		   {
			   phaseConfig.shadowBuffer = shadowBufferId;
			   if (GetTexture(phaseConfig.shadowBuffer).depthView == nullptr)
			   {
				   Throw(0, "No shadow depth buffer set for DX1AppRenderer::Render(...)");
			   }
		   }

		   if (!phaseConfig.depthTarget)
		   {
			   phaseConfig.depthTarget = mainDepthBufferId;
		   }

		   trianglesThisFrame = 0;
		   entitiesThisFrame = 0;

		   auto now = OS::CpuTicks();
		   AIcost = now - lastTick;

		   if (mainBackBufferView.IsNull()) return;

		   lastTextureId = ID_TEXTURE::Invalid();

		   SyncViewport(phaseConfig.depthTarget);

		   SetAndClearRenderBuffers(scene.GetClearColour());
	
		   InitInvariantTextureViews();

		   now = OS::CpuTicks();

		   dc.RSSetState(objectRaterizering);
		   dc.OMSetDepthStencilState(objDepthState, 0);

		   builtFirstPass = false;

		   UpdateGlobalState(scene);

		   RenderTarget rt = GetCurrentRenderTarget();
		   dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

		   size_t nLights = 0;
		   const Light* lights = scene.GetLights(nLights);
		   for (size_t i = 0; i < nLights; ++i)
		   {
			   RenderSpotlightLitScene(lights[i], scene);
		   }

		   RenderAmbient(scene, lights[0]);

		   objCost = OS::CpuTicks() - now;
	   
		   FLOAT blendFactorUnused[] = { 0,0,0,0 };
		   dc.OMSetBlendState(plasmaBlend, blendFactorUnused, 0xffffffff);
		   RenderParticles(plasma, idPlasmaPS, idParticleVS, idPlasmaGS);

		   RenderGui(scene);

		   now = OS::CpuTicks();

		   if (!phaseConfig.renderTarget)  mainSwapChain->Present(1, 0);

		   presentCost = OS::CpuTicks() - now;

		   DetachContext();

		   now = OS::CpuTicks();
		   frameTime = now - lastTick;
		   lastTick = now;
	   }

	   void AddTriangle(const GuiVertex triangle[3])
	   {
		   guiVertices.push_back(triangle[0]);
		   guiVertices.push_back(triangle[1]);
		   guiVertices.push_back(triangle[2]);
	   }

	   RGBAb FontColourToSysColour(Fonts::FontColour colour)
	   {
		   RGBAb* pCol = (RGBAb*)&colour;
		   return *pCol;
	   }

	   virtual void DrawGlyph(const Vec2& uvTopLeft, const Vec2& posTopLeft, float dx, float dy, Fonts::FontColour fcolour)
	   {
		   float x = posTopLeft.x;
		   float y = posTopLeft.y;

		   RGBAb colour = FontColourToSysColour(fcolour);

		   SpriteVertexData drawFont{ 1.0f, 0.0f, 0.0f, 1.0f };

		   guiVertices.push_back(GuiVertex{ {x,           y}, {{ uvTopLeft.x,      uvTopLeft.y},      1 }, drawFont, (RGBAb)colour }); // topLeft
		   guiVertices.push_back(GuiVertex{ {x,      y + dy}, {{ uvTopLeft.x,      uvTopLeft.y + dy}, 1 }, drawFont, (RGBAb)colour }); // bottomLeft
		   guiVertices.push_back(GuiVertex{ {x + dx, y + dy}, {{ uvTopLeft.x + dx, uvTopLeft.y + dy}, 1 }, drawFont, (RGBAb)colour }); // bottomRigh
		   guiVertices.push_back(GuiVertex{ {x,           y}, {{ uvTopLeft.x,      uvTopLeft.y},      1 }, drawFont, (RGBAb)colour }); // topLeft
		   guiVertices.push_back(GuiVertex{ {x + dx,      y}, {{ uvTopLeft.x + dx, uvTopLeft.y},      1 }, drawFont, (RGBAb)colour }); // TopRight
		   guiVertices.push_back(GuiVertex{ {x + dx, y + dy}, {{ uvTopLeft.x + dx, uvTopLeft.y + dy}, 1 }, drawFont, (RGBAb)colour }); // bottomRight
	   }

	   void FlushLayer()
	   {
		   size_t nVerticesLeftToRender = guiVertices.size();
		   while (nVerticesLeftToRender > 0)
		   {
			   size_t startIndex = guiVertices.size() - nVerticesLeftToRender;
			   size_t chunk = min(nVerticesLeftToRender, (size_t)GUI_BUFFER_VERTEX_CAPACITY);

			   D3D11_MAPPED_SUBRESOURCE x;
			   VALIDATEDX11(dc.Map(guiBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &x));
			   memcpy(x.pData, &guiVertices[startIndex], chunk * sizeof(GuiVertex));
			   dc.Unmap(guiBuffer, 0);

			   UINT stride = sizeof(GuiVertex);
			   UINT offset = 0;
			   dc.IASetVertexBuffers(0, 1, &guiBuffer, &stride, &offset);
			   dc.Draw((UINT)chunk, 0);

			   nVerticesLeftToRender -= chunk;
		   };

		   guiVertices.clear();
	   }

	   void ResizeBuffers(const Vec2i& span)
	   {
		   mainBackBufferView.Detach();

		   VALIDATEDX11(mainSwapChain->ResizeBuffers(1, span.x, span.y, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

		   AutoRelease<ID3D11Texture2D> backBuffer;
		   VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));
		   VALIDATEDX11(device.CreateRenderTargetView(backBuffer, nullptr, &mainBackBufferView));

		   RECT rect;
		   GetClientRect(window, &rect);

		   ID_TEXTURE newDepthBufferId = CreateDepthTarget(rect.right - rect.left, rect.bottom - rect.top);
		   auto& theOld = GetTexture(mainDepthBufferId);
		   auto& theNew = GetTexture(newDepthBufferId);
		   std::swap(theOld, theNew);
		   theNew.depthView->Release();
		   theNew.shaderView->Release();
		   theNew.texture->Release();
		   textures.pop_back();
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
					   SetCapture(window);
					   return;
				   }
			   }
		   }
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

