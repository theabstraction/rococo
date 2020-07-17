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

		void DrawGlyph(cr_vec2 t0, cr_vec2 t1, cr_vec2 p0, cr_vec2 p1, Fonts::FontColour colour) override
		{
			ExpandZoneToContain(renderZone, p0 );
			ExpandZoneToContain(renderZone, p1 );
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
	   DX11::TextureBind tb = { 0 };
	  DXGI_FORMAT format = DXGI_FORMAT_NV11;
      int32 width{ 0 };
	  int32 height{ 0 };
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

      void ResetWidth(int32 width)
      {
         Clear();
         this->width = width;
		 this->height = width;
      }

	  void ResetWidth(int32 width, int32 height)
	  {
		  Clear();
		  this->width = width;
		  this->height = height;
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
			format = DXGI_FORMAT_R8G8B8A8_UNORM;

            D3D11_TEXTURE2D_DESC colourSpriteArray;
            colourSpriteArray.Width = width;
            colourSpriteArray.Height = height;
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
			if (format != DXGI_FORMAT_R8G8B8A8_UNORM)
			{
				Throw(0, "DX11TextureArray::format is not RGBA but image passed was RGBA");
			}

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

	  virtual void WriteSubImage(size_t index, const uint8* grayScalePixels, Vec2i span)
	  {
		  if (width > 0 && tb.texture == nullptr)
		  {
			  arrayCapacity = count;

			  D3D11_TEXTURE2D_DESC alphArray;
			  alphArray.Width = width;
			  alphArray.Height = height;
			  alphArray.MipLevels = 1;
			  alphArray.ArraySize = (UINT)arrayCapacity;
			  alphArray.Format = DXGI_FORMAT_A8_UNORM;
			  alphArray.SampleDesc.Count = 1;
			  alphArray.SampleDesc.Quality = 0;
			  alphArray.Usage = D3D11_USAGE_DEFAULT;
			  alphArray.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			  alphArray.CPUAccessFlags = 0;
			  alphArray.MiscFlags = 0;
			  VALIDATEDX11(device.CreateTexture2D(&alphArray, nullptr, &tb.texture));
		  }

		  if (width > 0)
		  {
			  if (format != DXGI_FORMAT_A8_UNORM)
			  {
				  Throw(0, "DX11TextureArray::format is not A8 but image passed was A8");
			  }

			  UINT subresourceIndex = D3D11CalcSubresource(0, (UINT)index, 1);
			  D3D11_BOX box;
			  box.left = 0;
			  box.right = 1;
			  box.back = 1;
			  box.front = 0;
			  box.top = 0;
			  box.bottom = 1;

			  UINT srcDepth = span.x * span.y * sizeof(uint8);
			  dc.UpdateSubresource(tb.texture, subresourceIndex, &box, grayScalePixels, span.x * sizeof(uint8), srcDepth);
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
	   std::unordered_map<ID_TEXTURE, DX11TextureArray*, ID_TEXTURE> genericTextureArray;
	   std::unordered_map<std::string, ID_TEXTURE> nameToGenericTextureId;

	   DX11TextureArray spriteArray;
	   DX11TextureArray materialArray;

	   std::vector<DX11::TextureBind> cubeTextureArray;
	   std::unordered_map<std::string, ID_CUBE_TEXTURE> nameToCubeTexture;

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

	   AutoRelease<ID3D11RasterizerState> spriteRasterizering;
	   AutoRelease<ID3D11RasterizerState> objectRasterizering;
	   AutoRelease<ID3D11RasterizerState> particleRasterizering;
	   AutoRelease<ID3D11RasterizerState> skyRasterizering;
	   AutoRelease<ID3D11RasterizerState> shadowRasterizering;

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

	   AutoRelease<ID3D11Buffer> lightConeBuffer;

	   RAWMOUSE lastMouseEvent;
	   Vec2i screenSpan;

	   ID_VERTEX_SHADER idGuiVS;
	   ID_PIXEL_SHADER idGuiPS;

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
	   ID_PIXEL_SHADER idObjPS_Shadows;

	   ID_VERTEX_SHADER idObjSkyVS;
	   ID_PIXEL_SHADER idObjSkyPS;

	   AutoRelease<ID3D11Buffer> instanceBuffer;

	   AutoRelease<ID3D11DepthStencilState> guiDepthState;
	   AutoRelease<ID3D11DepthStencilState> objDepthState;
	   AutoRelease<ID3D11DepthStencilState> objDepthState_NoWrite;
	   AutoRelease<ID3D11DepthStencilState> noDepthTestOrWrite;

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

	   void Load(cstr name, IEventCallback<CompressedTextureBuffer>& onLoad) override
	   {
		   StandardLoadFromCompressedTextureBuffer(name, onLoad, installation, *scratchBuffer);
	   }

	   ID_TEXTURE LoadAlphaTextureArray(cstr uniqueName, Vec2i span, int32 nElements, ITextureLoadEnumerator& enumerator) override
	   {
		   auto i = nameToGenericTextureId.find(uniqueName);
		   if (i != nameToGenericTextureId.end()) return i->second;
		  
		   DX11TextureArray* array = new DX11TextureArray(device, dc);

		   try
		   {
			   array->ResetWidth(span.x, span.y);
			   array->Resize(nElements);

			   struct : IEventCallback<TextureLoadData>
			   {
				   int index = 0;
				   DX11TextureArray* array;
				   void OnEvent(TextureLoadData& data) override
				   {
					   struct : Rococo::Imaging::IImageLoadEvents
					   {
						   const wchar_t* filename;
						   DX11TextureArray* array;
						   int32 index;

						   void OnError(const char* message) override
						   {
							   Throw(0, "IRenderer.LoadAlphaTextureArray(...) - Error loading\n %S", filename);
						   }

						   void OnRGBAImage(const Vec2i& span, const RGBAb* data) override
						   {
							   Throw(0, "IRenderer.LoadAlphaTextureArray(...) - Tiff was RGBA.\n %S", filename);
						   }

						   void OnAlphaImage(const Vec2i& span, const uint8* data) override
						   {
							   if (array->width != span.x || array->height != span.y)
							   {
								   Throw(0, "IRenderer.LoadAlphaTextureArray(...) - Tiff was of incorrect span.\n % S", filename);
							   }
							   array->WriteSubImage(index, data, span);
						   }
					   } toArray;
					   toArray.filename = data.filename;
					   toArray.array = array;
					   toArray.index = index;

					   if (EndsWith(data.filename, L".tff"))
					   {
						   Rococo::Imaging::DecompressTiff(toArray, data.pData, data.nBytes);
						   index++;
					   }
				   }
			   } insertImageIntoArray;

			   insertImageIntoArray.array = array;

			   enumerator.ForEachElement(insertImageIntoArray, true);

			   auto id = ID_TEXTURE{ (size_t)array | 0x8000000000000000LL };
			   nameToGenericTextureId[uniqueName] = id;
			   genericTextureArray[id] = array;
			   return id;
		   }
		   catch (IException&)
		   {
			   delete array;
			   throw;
		   }
	   }

	   std::unordered_map<std::string, ID_PIXEL_SHADER> nameToPixelShader;

	   std::vector<ParticleVertex> fog;
	   std::vector<ParticleVertex> plasma;

	   ID3D11SamplerState* samplers[16] = { 0 };
	   AutoRelease<ID3D11SamplerState> skySampler;

	   ID_SYS_MESH skyMeshId;

	   enum TXUNIT // The enum values must match the tXXX registers specified in mplat.api.hlsl
	   {
		   TXUNIT_FONT = 0,
		   TXUNIT_SHADOW = 2,
		   TXUNIT_ENV_MAP = 3,
		   TXUNIT_SELECT = 4,
		   TXUNIT_MATERIALS = 6,
		   TXUNIT_SPRITES = 7
	   };
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

		   installation.LoadResource("!light_cone.ps", *scratchBuffer, 64_kilobytes);
		   idLightConePS = CreatePixelShader("!light_cone.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!light_cone.vs", *scratchBuffer, 64_kilobytes);
		   idLightConeVS = CreateVertexShader("!light_cone.vs", scratchBuffer->GetData(), scratchBuffer->Length(), DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());

		   instanceBuffer = DX11::CreateConstantBuffer<ObjectInstance>(device);

		   installation.LoadResource("!skybox.vs", *scratchBuffer, 64_kilobytes);
		   idObjSkyVS = CreateVertexShader("!skybox.vs", scratchBuffer->GetData(), scratchBuffer->Length(), DX11::GetSkyVertexDesc(), DX11::NumberOfSkyVertexElements());

		   installation.LoadResource("!skybox.ps", *scratchBuffer, 64_kilobytes);
		   idObjSkyPS = CreatePixelShader("!skybox.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   lastTick = OS::CpuTicks();
		   rng.seed(123456U);

		   D3D11_SAMPLER_DESC desc;
		   desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		   desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		   desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		   desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		   desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		   desc.MaxAnisotropy = 1;
		   desc.MipLODBias = 0;
		   desc.MaxLOD = D3D11_FLOAT32_MAX;
		   desc.MinLOD = 0;

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

		   SetSampler(TXUNIT_FONT,	 Filter_Linear, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		   SetSampler(TXUNIT_SHADOW, Filter_Linear, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		   SetSampler(TXUNIT_ENV_MAP,   Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		   SetSampler(TXUNIT_SELECT, Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		   SetSampler(TXUNIT_MATERIALS, Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		   SetSampler(TXUNIT_SPRITES,   Filter_Point, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);

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

		   for (auto& t : cubeTextureArray)
		   {
			   t.shaderView->Release();
			   t.texture->Release();
		   }


		   for (auto& t : genericTextureArray)
		   {
			   delete t.second;
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
	   }

	   struct CubeLoader : public DX11::IColourBitmapLoadEvent
	   {
		   size_t face;
		   int width;
		   std::vector<RGBAb> expandedBuffer;

		   void OnLoad(const RGBAb* pixels, const Vec2i& span) override
		   {
			   if (width == 0)
			   {
				   width = span.x;
				   if (span.x != span.y)
				   {
					   Throw(0, "CubeLoader::OnLoad: image was not sqaure");
				   }

				   expandedBuffer.resize(6 * width * width);
			   }

			   if (span.x != width || span.y != width)
			   {
				   Throw(0, "Image span %d x %d did not match the cube texture face width [%d]", span.x, span.y, width);
			   }

			   size_t sizeOfImageBytes = width * width * sizeof(RGBAb);
			   char* dest = (char*) expandedBuffer.data();
			   memcpy(dest + face * sizeOfImageBytes, pixels, sizeOfImageBytes);
		   }
	   };

	   enum { CUBE_ID_BASE = 100000000 };

	   ID_CUBE_TEXTURE CreateCubeTexture(cstr path, cstr extension)
	   {
		   auto i = nameToCubeTexture.find(path);
		   if (i != nameToCubeTexture.end())
		   {
			   return i->second;
		   }

		   const char* short_filenames[6] = { "posx", "negx", "posy", "negy", "posz", "negz" };

		   CubeLoader cubeloader;
		   cubeloader.width = 0;
		   cubeloader.face = 0;

		   D3D11_SUBRESOURCE_DATA initData[6];

		   for (auto f : short_filenames)
		   {
			   char fullpath[Rococo::IO::MAX_PATHLEN];
			   SecureFormat(fullpath, Rococo::IO::MAX_PATHLEN, "%s%s.%s", path, f, extension);
			   textureLoader.LoadColourBitmapIntoAddress(fullpath, cubeloader);

			   size_t sizeofImage = cubeloader.width * cubeloader.width;

			   initData[cubeloader.face].pSysMem = cubeloader.expandedBuffer.data() + sizeofImage * cubeloader.face;
			   initData[cubeloader.face].SysMemPitch = cubeloader.width * sizeof(RGBAb);
			   initData[cubeloader.face].SysMemSlicePitch = 0;

			   cubeloader.face++;
		   }

		   D3D11_TEXTURE2D_DESC desc;
		   ZeroMemory(&desc, sizeof(desc));
		   desc.Width = cubeloader.width;
		   desc.Height = cubeloader.width;
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
		   
		   VALIDATEDX11(device.CreateTexture2D(&desc, &initData[0], &pTexCube));

		   ID3D11ShaderResourceView* view = nullptr;
		   HRESULT hr = device.CreateShaderResourceView(pTexCube, nullptr, &view);
		   if FAILED(hr)
		   {
			   pTexCube->Release();
			   Throw(hr, "DX11Renderer::CreateCubeTexture: Error creating shader resource view for cube texture");
		   }

		   DX11::TextureBind tb;
		   tb.shaderView = view;
		   tb.texture = pTexCube;

		   cubeTextureArray.push_back(tb);

		   size_t index = cubeTextureArray.size() + CUBE_ID_BASE;
		   auto id = ID_CUBE_TEXTURE{ index };

		   nameToCubeTexture[path] = id;

		   return id;
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
			   HRESULT hr = device.CreateShaderResourceView(pTexCube, nullptr, &view);
			   if FAILED(hr)
			   {
					pTexCube->Release();
					Throw(hr, "DX11Renderer::SyncCubeTexture: Error creating shader resource view for cube texture");
			   }

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

		   auto mainDepthBufferDesc = CreateDepthTarget(rect.right - rect.left, rect.bottom - rect.top);
		   textures.push_back(mainDepthBufferDesc);
		   this->mainDepthBufferId = ID_TEXTURE{ textures.size() };
		   mapNameToTexture[scdt_name] = mainDepthBufferId;

		   auto shadowBufferDesc = CreateDepthTarget(1024, 1024);
		   textures.push_back(shadowBufferDesc);

		   this->shadowBufferId = ID_TEXTURE{ textures.size() };
		   mapNameToTexture["ShadowBuffer"] = shadowBufferId;

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

	   DX11::TextureBind CreateDepthTarget(int32 width, int32 height)
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

		   return { tex2D, srv, nullptr, depthView };
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

	   ID_SYS_MESH CreateSkyMesh(const SkyVertex* vertices, uint32 nVertices)
	   {
		   ID3D11Buffer* meshBuffer = vertices ? DX11::CreateImmutableVertexBuffer(device, vertices, nVertices) : nullptr;
		   meshBuffers.push_back(MeshBuffer{ meshBuffer, nVertices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ID_PIXEL_SHADER(), ID_PIXEL_SHADER(), false, false });
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

	   void DrawLightCone(const Light& light)
	   {
		   /* our aim is to render a cross section of the light cone as a single alpha blended triangle
				   B
				 '
			   '
			 ' (cutoffTheta)
		   A   -----------------> Light direction D
			 '
			   '
				  '
					C


			  A is the position of the light facing D. We need to compute B and C
			 
			  The triangle ABC is parallel to the screen (with screem direction F), which means BC.F = 0

			  A unit vector p parallel to BC is thus the normalized cross product of F and D.

			  p = |D x F|.

			  We can now construct B and C by taking basis vectors d and p from A.

			  Given we specify a length k of the light cone, along its central axis,
			  then the radius R along the vector p to construct B and C is such that
			  R / k = tan ( cutoffTheta )

			  The component of d of D parallel to the screen is | F x p |

		   */

		   const Vec3& D = light.direction;

		   Vec3 p = Cross(D, currentGlobalState.viewDir);

		   if (LengthSq(p) < 0.15)
		   { 
			   // Too acute, do not render
			   return;
		   }

		   p = Normalize(p);

		   if (fabsf(light.cutoffCosAngle) < 0.1)
		   {
			   // Angle too obtuse, do not render
			   return;
		   }

		   float cutOffAngle = acosf(light.cutoffCosAngle);
		   float tanCutoff = tanf(cutOffAngle);

		   auto coneLength = 1.0_metres;
		   auto radius = coneLength* tanCutoff;

		   Vec3 d = Normalize(Cross(currentGlobalState.viewDir, p));

		   ObjectVertex v[3] = { 0 };
		   v[0].position = light.position;
		   v[0].material.colour = RGBAb(255, 255, 255, 128);
		   v[0].uv.x = 0;
		   v[1].uv.y = 0;

		   v[1].position = light.position + coneLength * D + radius * p;
		   v[1].material.colour = RGBAb(255, 255, 255, 0);
		   v[1].uv.x = 1.0f;
		   v[1].uv.y = -1.0f;

		   v[2].position = light.position + coneLength * D - radius * p;
		   v[2].material.colour = RGBAb(255, 255, 255, 0);
		   v[2].uv.x = 1.0f;
		   v[2].uv.y = -1.0f;

		   DX11::CopyStructureToBuffer(dc, lightConeBuffer, v, sizeof(ObjectVertex) * 3);

		   dc.Draw(3, 0);
		   trianglesThisFrame += 1;
	   }

	   void Draw(ID_SYS_MESH id, const ObjectInstance* instances, uint32 nInstances) override
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
			   const wchar_t* sysId;

			   switch (cursorId)
			   {
			   case EWindowCursor_HDrag:
				   sysId = IDC_SIZEWE;
				   break;
			   case EWindowCursor_VDrag:
				   sysId = IDC_SIZENS;
				   break;
			   case EWindowCursor_BottomRightDrag:
				   sysId = IDC_SIZENWSE;
				   break;
			   case EWindowCursor_HandDrag:
				   sysId = IDC_HAND;
				   break;
			   case EWindowCursor_Default:
			   default:
				   sysId = IDC_ARROW;
			   }

			   HCURSOR hCursor = LoadCursorW(nullptr, sysId);
			   SetCursor(hCursor);
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

			   dc.RSSetState(shadowRasterizering);

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
			   dc.RSSetState(objectRasterizering);

			   scene.RenderObjects(*this);

			   dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			   RenderParticles(fog, idFogSpotlightPS, idParticleVS, idFogSpotlightGS);
			   phase = RenderPhase_None;

			   dc.OMSetDepthStencilState(objDepthState, 0);
		   }
	   }

	   Vec2i lastSpan{ -1,-1 };

	   void SetGuiShader(cstr pixelShaderName)
	   {
		   FlushLayer();

		   if (pixelShaderName == nullptr || pixelShaderName[0] == 0)
		   {
			   // Default
			   if (!UseShaders(idGuiVS, idGuiPS))
			   {
				   Throw(0, "IGuiContext::SetGuiShader(<blank>) error setting Gui shader to default value");
			   }

			   return;
		   }

		   auto i = nameToPixelShader.find(pixelShaderName);
		   if (i == nameToPixelShader.end())
		   {
			   // Try to load it
			   try
			   {
				   installation.LoadResource(pixelShaderName, *scratchBuffer, 64_kilobytes);
				   auto pxId = CreatePixelShader(pixelShaderName, scratchBuffer->GetData(), scratchBuffer->Length());
				   i = nameToPixelShader.insert(std::make_pair(std::string(pixelShaderName), pxId)).first;
			   }
			   catch (IException& ex)
			   {
				   Throw(ex.ErrorCode(), "IGuiContext::SetGuiShader('%s') failed to load shader:\n %s", pixelShaderName, ex.Message());
			   }
		   }
		  
		   if (!UseShaders(idGuiVS, i->second))
		   {
			   Throw(0, "IGuiContext::SetGuiShader('%s') failed to use shader", pixelShaderName);
		   }
	   }

	   void RenderGui(IScene& scene)
	   {
		   OS::ticks now = OS::CpuTicks();

		   dc.RSSetState(spriteRasterizering);

		   D3D11_RECT rect = { 0, 0, screenSpan.x, screenSpan.y };
		   dc.RSSetScissorRects(1, &rect);

		   FLOAT blendFactorUnused[] = { 0,0,0,0 };
		   dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);
		   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		   dc.OMSetDepthStencilState(guiDepthState, 0);

		   if (!UseShaders(idGuiVS, idGuiPS))
		   {
			   Throw(0, "Error setting Gui shaders");
		   }

		   if (lastSpan != screenSpan)
		   {
			   lastSpan = screenSpan;
			   scene.OnGuiResize(lastSpan);
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

			   dc.RSSetState(objectRasterizering);

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
		   dc.PSSetShaderResources(TXUNIT_FONT, 1, &fontBinding);
		   dc.PSSetShaderResources(TXUNIT_ENV_MAP, 1, &cubeTextureView);

		   ID3D11ShaderResourceView* materials[1] = { materialArray.View() };
		   dc.PSSetShaderResources(TXUNIT_MATERIALS, 1, materials);

		   ID3D11ShaderResourceView* spriteviews[1] = { spriteArray.View() };
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

		   g.guiScale.OOScreenWidth = 1.0f / screenSpan.x;
		   g.guiScale.OOScreenHeight = 1.0f / screenSpan.y;
		   g.guiScale.OOFontWidth = fonts->TextureSpan().z;
		   g.guiScale.OOSpriteWidth = 1.0f / spriteArray.width;

		   DX11::CopyStructureToBuffer(dc, globalStateBuffer, g);

		   currentGlobalState = g;

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

	   void RenderSkybox(IScene& scene)
	   {
		   ID_CUBE_TEXTURE cubeId = scene.GetSkyboxCubeId();

		   if (cubeId)
		   {
			   size_t index = cubeId.value - CUBE_ID_BASE - 1;
			   if (index < cubeTextureArray.size())
			   {
				   auto* skyCubeTextureView = this->cubeTextureArray[index].shaderView;

				   if (UseShaders(idObjSkyVS, idObjSkyPS))
				   {
					   auto& mesh = meshBuffers[skyMeshId.value];
					   UINT strides[] = { sizeof(SkyVertex) };
					   UINT offsets[]{ 0 };
					   dc.IASetPrimitiveTopology(mesh.topology);
					   dc.IASetVertexBuffers(0, 1, &mesh.dx11Buffer, strides, offsets);
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
			   else
			   {
				   Throw(0, "DX11Renderer::RenderSkybox failed. Cube id was not valid");
			   }
		   }
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

		   now = OS::CpuTicks();

		   UpdateGlobalState(scene);

		   RenderTarget rt = GetCurrentRenderTarget();
		   dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

		   RenderSkybox(scene);

		   InitInvariantTextureViews();

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
				   catch (IException & ex)
				   {
					   Throw(ex.ErrorCode(), "Error lighting scene with light #%d: %s", i, ex.Message());
				   }
			   }

			   RenderAmbient(scene, lights[0]);
		   }

		   objCost = OS::CpuTicks() - now;
	   
		   FLOAT blendFactorUnused[] = { 0,0,0,0 };
		   dc.OMSetBlendState(plasmaBlend, blendFactorUnused, 0xffffffff);
		   RenderParticles(plasma, idPlasmaPS, idParticleVS, idPlasmaGS);

		   DrawLightCones(scene);

		   UpdateGlobalState(scene);
		   RenderGui(scene);

		   now = OS::CpuTicks();

		   if (!phaseConfig.renderTarget)  mainSwapChain->Present(1, 0);

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
				   if (lights[i].ambient.alpha == 0)
				   {
					   break;
				   }

				   DrawLightCone(lights[i]);
			   }

			   dc.IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
			   dc.RSSetState(nullptr);
			   dc.PSSetConstantBuffers(0, 0, nullptr);
			   dc.VSSetConstantBuffers(0, 0, nullptr);
		   }
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

	   virtual void DrawGlyph(cr_vec2 uvTopLeft, cr_vec2 uvBottomRight, cr_vec2 posTopLeft, cr_vec2 posBottomRight, Fonts::FontColour fcolour)
	   {
		   float x0 = posTopLeft.x;
		   float y0 = posTopLeft.y;
		   float x1 = posBottomRight.x;
		   float y1 = posBottomRight.y;

		   float u0 = uvTopLeft.x;
		   float v0 = uvTopLeft.y;
		   float u1 = uvBottomRight.x;
		   float v1 = uvBottomRight.y;

		   RGBAb colour = FontColourToSysColour(fcolour);

		   SpriteVertexData drawFont{ 1.0f, 0.0f, 0.0f, 1.0f };

		   guiVertices.push_back(GuiVertex{ {x0, y0}, {{ u0, v0}, 1 }, drawFont, (RGBAb)colour }); // topLeft
		   guiVertices.push_back(GuiVertex{ {x0, y1}, {{ u0, v1}, 1 }, drawFont, (RGBAb)colour }); // bottomLeft
		   guiVertices.push_back(GuiVertex{ {x1, y1}, {{ u1, v1}, 1 }, drawFont, (RGBAb)colour }); // bottomRigh
		   guiVertices.push_back(GuiVertex{ {x0, y0}, {{ u0, v0}, 1 }, drawFont, (RGBAb)colour }); // topLeft
		   guiVertices.push_back(GuiVertex{ {x1, y0}, {{ u1, v0}, 1 }, drawFont, (RGBAb)colour }); // TopRight
		   guiVertices.push_back(GuiVertex{ {x1, y1}, {{ u1, v1}, 1 }, drawFont, (RGBAb)colour }); // bottomRight
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

	   void SetScissorRect(const Rococo::GuiRectf& rect) override
	   {
		   D3D11_RECT d11Rect;
		   d11Rect.left = (LONG) rect.left;
		   d11Rect.top = (LONG)rect.top;
		   d11Rect.right = (LONG)rect.right;
		   d11Rect.bottom = (LONG)rect.bottom;
		   dc.RSSetScissorRects(1, &d11Rect);
	   }

	   void ClearScissorRect() override
	   {
		   D3D11_RECT rect = { 0, 0, screenSpan.x, screenSpan.y };
		   dc.RSSetScissorRects(1, &rect);
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

