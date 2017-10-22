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

namespace
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	struct DX11Shader
	{
		std::string name;
	};

	struct DX11VertexShader : public DX11Shader
	{
		AutoRelease<ID3D11InputLayout> inputLayout;
		AutoRelease<ID3D11VertexShader> vs;
	};

	struct DX11PixelShader : public DX11Shader
	{
		AutoRelease<ID3D11PixelShader> ps;
	};

	struct IAppEventHandler
	{
		virtual void BindMainWindow(HWND hWnd) = 0;
		virtual bool OnClose() = 0;
		virtual void OnKeyboardEvent(const RAWKEYBOARD& k) = 0;
		virtual void OnMouseEvent(const RAWMOUSE& m) = 0;
		virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type) = 0;
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

	struct GuiScale
	{
		float OOScreenWidth;
		float OOScreenHeight;
		float OOFontWidth;
		float OOFontHeight;
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

      std::vector<GuiVertex> spriteTriangles;

      ID3D11ShaderResourceView* View()
      {
         if (tb.view == nullptr)
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
               tb.view = view;
            }
         }

         return tb.view;
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
         if (tb.view) tb.view->Release();
         tb.texture = nullptr;
         tb.view = nullptr;
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

      virtual void WriteSubImage(size_t index, const Imaging::F_A8R8G8B8* pixels, const GuiRect& targetLocation)
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
            const RGBAb* src = ConvertToRGBAbFormat((Imaging::F_A8R8G8B8*) pixels, span.x * span.y);
            D3D11_BOX box;
            box.left = targetLocation.left;
            box.right = targetLocation.right;
            box.back = 1;
            box.front = 0;
            box.top = targetLocation.top;
            box.bottom = targetLocation.bottom;

            UINT srcDepth = span.x * span.y * sizeof(RGBAb);
            dc.UpdateSubresource(tb.texture, subresourceIndex, &box, src, span.x * sizeof(RGBAb), srcDepth);
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

   class DX11AppRenderer :
	   public IRenderer,
	   public IRenderContext,
	   public IGuiRenderContext,
	   public Fonts::IGlyphRenderer,
	   public IResourceLoader, 
	   public IMathsVenue
   {
   private:
	   OS::ticks lastTick;
	   ID3D11Device& device;
	   ID3D11DeviceContext& dc;
	   IDXGIFactory& factory;
	   IInstallation& installation;

	   DX11TextureArray textureArray;

	   AutoFree<ITextureArrayBuilderSupervisor> textureArrayBuilder;

	   AutoRelease<IDXGISwapChain> mainSwapChain;
	   AutoRelease<ID3D11RenderTargetView> mainBackBufferView;
	   AutoRelease<ID3D11DepthStencilView> depthStencilView;

	   std::vector<DX11VertexShader*> vertexShaders;
	   std::vector<DX11PixelShader*> pixelShaders;
	   AutoRelease<ID3D11Buffer> guiBuffer;

	   std::vector<GuiVertex> guiVertices;

	   enum { GUI_BUFFER_VERTEX_CAPACITY = 3 * 512 };

	   AutoRelease<ID3D11Texture2D> fontTexture;
	   AutoRelease<ID3D11SamplerState> spriteSampler;
	   AutoRelease<ID3D11SamplerState> objectSampler;
	   AutoRelease<ID3D11ShaderResourceView> fontBinding;
	   AutoRelease<ID3D11RasterizerState> spriteRaterizering;
	   AutoRelease<ID3D11RasterizerState> objectRaterizering;
	   AutoRelease<ID3D11BlendState> alphaBlend;
	   AutoRelease<ID3D11BlendState> disableBlend;

	   Fonts::IFontSupervisor* fonts;

	   AutoRelease<ID3D11Buffer> vector4Buffer;
	   AutoRelease<ID3D11Buffer> vs_globalStateBuffer;
	   AutoRelease<ID3D11Buffer> ps_globalStateBuffer;

	   RAWMOUSE lastMouseEvent;
	   Vec2i screenSpan;

	   HWND hRenderWindow;

	   ID_VERTEX_SHADER idGuiVS;
	   ID_PIXEL_SHADER idGuiPS;

	   ID_VERTEX_SHADER idObjVS;
	   ID_PIXEL_SHADER idObjPS;
	   ID_PIXEL_SHADER idObjDepthPS;

	   ID_VERTEX_SHADER idSpriteVS;
	   ID_PIXEL_SHADER idSpritePS;

	   AutoRelease<ID3D11Buffer> instanceBuffer;

	   AutoRelease<ID3D11DepthStencilState> guiDepthState;
	   AutoRelease<ID3D11DepthStencilState> objDepthState;

	   std::vector<DX11::TextureBind> textures;
	   std::unordered_map<std::string, ID_TEXTURE> mapNameToTexture;

	   std::vector<Overlay> overlays;

	   struct Cursor
	   {
		   ID_TEXTURE bitmapId;
		   Vec2 uvTopLeft;
		   Vec2 uvBottomRight;
		   Vec2i hotspotOffset;
	   } cursor;

	   ID_TEXTURE lastTextureId;

	   AutoFree<IExpandingBuffer> scratchBuffer;
	   DX11::TextureLoader textureLoader;

	   virtual void Load(cstr name, IEventCallback<CompressedTextureBuffer>& onLoad)
	   {
		   StandardLoadFromCompressedTextureBuffer(name, onLoad, installation, *scratchBuffer);
	   }
   public:
	   Windows::IWindow* window;
	   bool isBuildingAlphaBlendedSprites{ false };

	   DX11AppRenderer(ID3D11Device& _device, ID3D11DeviceContext& _dc, IDXGIFactory& _factory, IInstallation& _installation) :
		   device(_device), dc(_dc), factory(_factory), fonts(nullptr), hRenderWindow(0),
		   window(nullptr), cursor{ ID_TEXTURE(), {0,0}, {1,1}, {0,0} }, installation(_installation),
		   textureArray(_device, _dc),
		   textureArrayBuilder(CreateTextureArrayBuilder(*this, textureArray)),
		   scratchBuffer(CreateExpandingBuffer(16_kilobytes)),
		   textureLoader(_installation, _device, _dc, *scratchBuffer)
	   {
		   static_assert(GUI_BUFFER_VERTEX_CAPACITY % 3 == 0, "Capacity must be divisible by 3");
		   guiBuffer = DX11::CreateDynamicVertexBuffer<GuiVertex>(device, GUI_BUFFER_VERTEX_CAPACITY);

		   spriteSampler = DX11::CreateSpriteSampler(device);
		   objectSampler = DX11::CreateObjectSampler(device);
		   objDepthState = DX11::CreateObjectDepthStencilState(device);
		   guiDepthState = DX11::CreateGuiDepthStencilState(device);
		   spriteRaterizering = DX11::CreateSpriteRasterizer(device);
		   objectRaterizering = DX11::CreateObjectRasterizer(device);
		   alphaBlend = DX11::CreateAlphaBlend(device);
		   disableBlend = DX11::CreateNoBlend(device);

		   DX11::TextureBind fb = textureLoader.LoadAlphaBitmap("!font1.tif");
		   fontTexture = fb.texture;
		   fontBinding = fb.view;

		   cstr csvName = "!font1.csv";
		   installation.LoadResource(csvName, *scratchBuffer, 256_kilobytes);
		   fonts = Fonts::LoadFontCSV(csvName, (const char*)scratchBuffer->GetData(), scratchBuffer->Length());

		   // Create the buffer.
		   vector4Buffer = DX11::CreateConstantBuffer<Vec4>(device);

		   GuiScale nullVector{ 0,0,0,0 };
		   DX11::CopyStructureToBuffer(dc, vector4Buffer, nullVector);

		   vs_globalStateBuffer = DX11::CreateConstantBuffer<GlobalState>(device);
		   ps_globalStateBuffer = DX11::CreateConstantBuffer<GlobalState>(device);

		   installation.LoadResource("!gui.vs", *scratchBuffer, 64_kilobytes);
		   idGuiVS = CreateGuiVertexShader("!gui.vs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!gui.ps", *scratchBuffer, 64_kilobytes);
		   idGuiPS = CreatePixelShader("!gui.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!object.vs", *scratchBuffer, 64_kilobytes);
		   idObjVS = CreateObjectVertexShader("!object.vs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!object.ps", *scratchBuffer, 64_kilobytes);
		   idObjPS = CreatePixelShader("!object.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!depth.ps", *scratchBuffer, 64_kilobytes);
		   idObjDepthPS = CreatePixelShader("!depth.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!sprite.vs", *scratchBuffer, 64_kilobytes);
		   idSpriteVS = CreateGuiVertexShader("!sprite.vs", scratchBuffer->GetData(), scratchBuffer->Length());

		   installation.LoadResource("!sprite.ps", *scratchBuffer, 64_kilobytes);
		   idSpritePS = CreatePixelShader("!sprite.ps", scratchBuffer->GetData(), scratchBuffer->Length());

		   instanceBuffer = DX11::CreateConstantBuffer<ObjectInstance>(device);

		   lastTick = OS::CpuTicks();
	   }

	   ~DX11AppRenderer()
	   {
		   ClearContext();

		   if (fonts)
		   {
			   fonts->Free();
		   }

		   ClearMeshes();

		   for (auto& x : vertexShaders)
		   {
			   delete x;
		   }

		   for (auto& x : pixelShaders)
		   {
			   delete x;
		   }

		   for (auto& t : textures)
		   {
			   if (t.texture) t.texture->Release();
			   if (t.view) t.view->Release();
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

	   void ShowTextureVenue(IMathsVisitor& visitor)
	   {
		   for (auto& t : mapNameToTexture)
		   {
			   auto& tx = textures[t.second.value-1];

			   D3D11_TEXTURE2D_DESC desc;
			   tx.texture->GetDesc(&desc);
			   visitor.ShowString(t.first.c_str(), " #%4llu - 0x%p. %d x %d texels. %d levels", t.second.value, (const void*) tx.texture, desc.Width, desc.Height, desc.MipLevels);
		   }
	   }

	   virtual void GetMeshDesc(char desc[256], ID_SYS_MESH id)
	   {
		   if (!id || id.value >= meshBuffers.size())
		   {
			   SafeFormat(desc, 256, "invalid id");
		   }
		   else
		   {
			   auto& m = meshBuffers[id.value - 1];

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
		   depthStencilView->GetDesc(&dsDesc);

		   visitor.ShowString("DepthStencil format", "%u", dsDesc.Format);

		   visitor.ShowDecimal("Number of textures", (int64)textures.size());
		   visitor.ShowDecimal("Number of meshes", (int64)meshBuffers.size());
		   visitor.ShowDecimal("Mesh updates", meshUpdateCount);

		   visitor.ShowDecimal("Cursor texture Id ", cursor.bitmapId);
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
	   }

	   IMathsVenue* Venue()
	   {
		   return this;
	   }

	   void ShowWindowVenue(IMathsVisitor& visitor)
	   {
		   ::ShowWindowVenue(hRenderWindow, visitor);
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
		   return *textureArrayBuilder;
	   }

	   virtual void AddSpriteTriangle(bool alphaBlend, const GuiVertex triangle[3])
	   {
		   if (this->isBuildingAlphaBlendedSprites != alphaBlend || !guiVertices.empty())
		   {
			   FlushLayer();
			   this->isBuildingAlphaBlendedSprites = alphaBlend;
		   }

		   textureArray.spriteTriangles.push_back(triangle[0]);
		   textureArray.spriteTriangles.push_back(triangle[1]);
		   textureArray.spriteTriangles.push_back(triangle[2]);

		   if (textureArray.spriteTriangles.size() > 8192)
		   {
			   FlushLayer();
		   }
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
		   if (id.value >= meshBuffers.size()) Throw(0, "DX11AppRenderer::DeleteMesh(...): Bad ID_SYS_MESH");

		   if (meshBuffers[id.value].dx11Buffer)
		   {
			   meshBuffers[id.value].dx11Buffer->Release();
			   meshBuffers[id.value].dx11Buffer = nullptr;
			   meshBuffers[id.value].numberOfVertices = 0;
		   }
	   }

	   virtual void SetMeshTexture(ID_TEXTURE textureId, int textureIndex)
	   {
		   size_t index = textureId.value - 1;
		   if (index >= textures.size())
		   {
			   Throw(0, "Bad texture id");
		   }

		   auto& t = textures[index];

		   if (textureId != lastTextureId)
		   {
			   if (t.view == nullptr)
			   {
				   D3D11_SHADER_RESOURCE_VIEW_DESC desc;
				   ZeroMemory(&desc, sizeof(desc));

				   desc.Texture2D.MipLevels = -1;
				   desc.Texture2D.MostDetailedMip = 0;

				   desc.Format = DXGI_FORMAT_UNKNOWN;
				   desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

				   VALIDATEDX11(device.CreateShaderResourceView(t.texture, &desc, &t.view));
			   }
			   lastTextureId = textureId;
			   dc.PSSetShaderResources(textureIndex, 1, &t.view);
		   }
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
		   return id;
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

		   if (id != lastTextureId)
		   {
			   FlushLayer();
			   lastTextureId = id;
			   dc.PSSetShaderResources(1, 1, &t.view);
		   }

		   return Vec2i{ (int32)desc.Width, (int32)desc.Height };
	   }

	   void SyncViewport()
	   {
		   RECT rect;
		   GetClientRect(hRenderWindow, &rect);

		   D3D11_VIEWPORT viewport;
		   ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

		   viewport.TopLeftX = 0;
		   viewport.TopLeftY = 0;
		   viewport.Width = FLOAT(rect.right - rect.left);
		   viewport.Height = FLOAT(rect.bottom - rect.top);
		   viewport.MinDepth = 0.0f;
		   viewport.MaxDepth = 1.0f;

		   dc.RSSetViewports(1, &viewport);

		   screenSpan.x = (int32)viewport.Width;
		   screenSpan.y = (int32)viewport.Height;
	   }

	   void BindMainWindow(HWND hRenderWindow)
	   {
		   this->hRenderWindow = hRenderWindow;

		   DXGI_SWAP_CHAIN_DESC swapChainDesc = DX11::GetSwapChainDescription(hRenderWindow);
		   VALIDATEDX11(factory.CreateSwapChain((ID3D11Device*)&device, &swapChainDesc, &mainSwapChain));

		   AutoRelease<ID3D11Texture2D> backBuffer;
		   VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

		   VALIDATEDX11(device.CreateRenderTargetView(backBuffer, nullptr, &mainBackBufferView));

		   auto depthDesc = DX11::GetDepthDescription(hRenderWindow);

		   AutoRelease<ID3D11Texture2D> depthBuffer;
		   VALIDATEDX11(device.CreateTexture2D(&depthDesc, nullptr, &depthBuffer));
		   VALIDATEDX11(device.CreateDepthStencilView(depthBuffer, nullptr, &depthStencilView));

		   SyncViewport();
	   }

	   ID_VERTEX_SHADER CreateVertexShader(cstr name, const byte* shaderCode, size_t shaderLength, const D3D11_INPUT_ELEMENT_DESC* vertexDesc, UINT nElements)
	   {
		   if (name == nullptr || rlen(name) > 1024) Throw(0, "Bad <name> for vertex shader");
		   if (shaderCode == nullptr || shaderLength < 4 || shaderLength > 65536) Throw(0, "Bad shader code for vertex shader %s", name);

		   DX11VertexShader* shader = new DX11VertexShader;
		   HRESULT hr = device.CreateInputLayout(vertexDesc, nElements, shaderCode, shaderLength, &shader->inputLayout);
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

	   bool UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid)
	   {
		   if (vid.value >= vertexShaders.size()) Throw(0, "Bad vertex shader Id in call to UseShaders");
		   if (pid.value >= pixelShaders.size()) Throw(0, "Bad pixel shader Id in call to UseShaders");

		   auto& vs = *vertexShaders[vid.value];
		   auto& ps = *pixelShaders[pid.value];

		   if (vs.vs == nullptr)
		   {
			   if (lastError[0] == 0)
			   {
				   SafeFormat(lastError, sizeof(lastError), "Vertex Shader null for %s", vs.name.c_str());
			   }
		   }

		   if (ps.ps == nullptr)
		   {
			   if (lastError[0] == 0)
			   {
				   SafeFormat(lastError, sizeof(lastError), "Pixel Shader null for %s", ps.name.c_str());
			   }
		   }

		   if (vs.vs == nullptr || ps.ps == nullptr)
		   {
			   dc.IASetInputLayout(nullptr);
			   dc.VSSetShader(nullptr, nullptr, 0);
			   dc.PSSetShader(nullptr, nullptr, 0);
			   return false;
		   }
		   else
		   {
			   dc.IASetInputLayout(vs.inputLayout);
			   dc.VSSetShader(vs.vs, nullptr, 0);
			   dc.PSSetShader(ps.ps, nullptr, 0);
			   return true;
		   }
	   }

	   void ClearContext()
	   {
		   dc.IASetInputLayout(nullptr);
		   dc.VSSetShader(nullptr, nullptr, 0);
		   dc.PSSetShader(nullptr, nullptr, 0);
		   dc.PSSetShaderResources(0, 0, nullptr);
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
		   ScreenToClient(hRenderWindow, &p);

		   metrics.cursorPosition = Vec2i{ p.x, p.y };
		   metrics.screenSpan = screenSpan;
	   }

	   struct MeshBuffer
	   {
		   ID3D11Buffer* dx11Buffer;
		   UINT numberOfVertices;
		   D3D_PRIMITIVE_TOPOLOGY topology;
	   };

	   std::vector<MeshBuffer> meshBuffers;
	   int64 meshUpdateCount = 0;

	   virtual ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices)
	   {
		   ID3D11Buffer* meshBuffer = vertices ? DX11::CreateImmutableVertexBuffer(device, vertices, nVertices) : nullptr;
		   meshBuffers.push_back(MeshBuffer{ meshBuffer, nVertices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST });
		   int32 index = (int32)meshBuffers.size();
		   meshUpdateCount++;
		   return ID_SYS_MESH(index - 1);
	   }

	   virtual void UpdateMesh(ID_SYS_MESH rendererId, const ObjectVertex* vertices, uint32 nVertices)
	   {
		   if (rendererId.value < 0 || rendererId.value >= meshBuffers.size())
		   {
			   Throw(E_INVALIDARG, "renderer.UpdateMesh(ID_MESH id, ....) - Bad id ");
		   }

		   meshUpdateCount++;

		   ID3D11Buffer* newMesh = vertices != nullptr ? DX11::CreateImmutableVertexBuffer(device, vertices, nVertices) : nullptr;
		   meshBuffers[rendererId.value].numberOfVertices = nVertices;

		   if (meshBuffers[rendererId.value].dx11Buffer) meshBuffers[rendererId.value].dx11Buffer->Release();
		   meshBuffers[rendererId.value].dx11Buffer = newMesh;
	   }

	   virtual void Draw(ID_SYS_MESH id, const ObjectInstance* instances, uint32 nInstances)
	   {
		   if (id.value < 0 || id.value >= meshBuffers.size()) Throw(E_INVALIDARG, "renderer.DrawObject(ID_MESH id) - Bad id ");

		   auto& buffer = meshBuffers[id.value];

		   if (!buffer.dx11Buffer)
			   return;

		   ID3D11Buffer* buffers[] = { buffer.dx11Buffer };

		   dc.IASetPrimitiveTopology(buffer.topology);

		   UINT strides[] = { sizeof(ObjectVertex) };
		   UINT offsets[]{ 0 };
		   dc.IASetVertexBuffers(0, 1, buffers, strides, offsets);

		   for (uint32 i = 0; i < nInstances; i++)
		   {
			   // dc.DrawInstances crashed the debugger, replace with single instance render call for now
			   DX11::CopyStructureToBuffer(dc, instanceBuffer, instances + i, sizeof(ObjectInstance));
			   dc.VSSetConstantBuffers(1, 1, &instanceBuffer);
			   dc.Draw(buffer.numberOfVertices, 0);
		   }

		   dc.VSSetConstantBuffers(0, 0, nullptr);
	   }

	   virtual Windows::IWindow& Window()
	   {
		   return *window;
	   }

	   GlobalState g;

	   virtual void SetGlobalState(const GlobalState& gs)
	   {
		   DX11::CopyStructureToBuffer(dc, vs_globalStateBuffer, gs);
		   dc.VSSetConstantBuffers(0, 1, &vs_globalStateBuffer);

		   DX11::CopyStructureToBuffer(dc, ps_globalStateBuffer, gs);
		   dc.PSSetConstantBuffers(0, 1, &ps_globalStateBuffer);
	   }

	   void DrawCursor()
	   {
		   if (cursor.bitmapId != ID_TEXTURE::Invalid())
		   {
			   GuiMetrics metrics;
			   GetGuiMetrics(metrics);

			   Vec2i span = SelectTexture(cursor.bitmapId);

			   float u0 = cursor.uvTopLeft.x;
			   float u1 = cursor.uvBottomRight.x;
			   float v0 = cursor.uvTopLeft.y;
			   float v1 = cursor.uvBottomRight.y;

			   Vec2 p{ (float)metrics.cursorPosition.x, (float)metrics.cursorPosition.y };

			   float x0 = p.x + cursor.hotspotOffset.x;
			   float x1 = x0 + (float)span.x;
			   float y0 = p.y + cursor.hotspotOffset.y;
			   float y1 = y0 + (float)span.y;

			   GuiVertex quad[6]
			   {
				   GuiVertex{ x0, y0, 0.0f, 0.0f, RGBAb(128,0,0), u0, v0, 0.0f },
				   GuiVertex{ x1, y0, 0.0f, 0.0f, RGBAb(0,128,0), u1, v0, 0.0f },
				   GuiVertex{ x1, y1, 0.0f, 0.0f, RGBAb(0,0,128), u1, v1, 0.0f },
				   GuiVertex{ x1, y1, 0.0f, 0.0f, RGBAb(128,0,0), u1, v1, 0.0f },
				   GuiVertex{ x0, y1, 0.0f, 0.0f, RGBAb(0,128,0), u0, v1, 0.0f },
				   GuiVertex{ x0, y0, 0.0f, 0.0f, RGBAb(0,0,128), u0, v0, 0.0f }
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

	   virtual void Render(IScene& scene)
	   {
		   auto now = OS::CpuTicks();
		   AIcost = now - lastTick;

		   if (mainBackBufferView.IsNull()) return;

		   lastTextureId = ID_TEXTURE::Invalid();

		   dc.OMSetRenderTargets(1, &mainBackBufferView, depthStencilView);

		   dc.ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		   FLOAT blendFactorUnused[] = { 0,0,0,0 };
		   dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);

		   if (UseShaders(idObjVS, idObjPS))
		   {
			   dc.PSSetSamplers(0, 1, &objectSampler);
			   dc.RSSetState(objectRaterizering);
			   dc.OMSetDepthStencilState(objDepthState, 0);

			   RGBA clearColour = scene.GetClearColour();
			   if (clearColour.alpha > 0)
			   {
				   dc.ClearRenderTargetView(mainBackBufferView, (const FLOAT*)&clearColour);
			   }
		   
			   now = OS::CpuTicks();

			   scene.RenderObjects(*this);

			   objCost = OS::CpuTicks() - now;

			   now = OS::CpuTicks();

			   scene.RenderGui(*this);

			   for (auto& o : overlays)
			   {
				   o.overlay->Render(*this);
			   }
		   }
		   else
		   {
			   FLOAT errorColour[4] = { 0.25f, 0.0f, 0.0f, 1.0f };
			   dc.ClearRenderTargetView(mainBackBufferView, errorColour);
			   Graphics::RenderTopLeftAlignedText(*this, lastError, RGBAb(255, 255, 255, 255), 8, { 0,0 });

			   now = OS::CpuTicks();
			   objCost = 0;
		   }

		   FlushLayer();

		   DrawCursor();

		   FlushLayer();

		   guiCost = OS::CpuTicks() - now;

		   renderState = RenderState_None;

		   ClearContext();

		   dc.PSSetShaderResources(0, 0, nullptr);
		   dc.PSSetSamplers(0, 0, nullptr);

		   now = OS::CpuTicks();

		   mainSwapChain->Present(1, 0);

		   presentCost = OS::CpuTicks() - now;

		   dc.OMSetBlendState(nullptr, nullptr, 0);
		   dc.RSSetState(nullptr);
		   dc.OMSetDepthStencilState(nullptr, 0);
		   dc.OMSetRenderTargets(0, nullptr, nullptr);
		   dc.IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

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

		   guiVertices.push_back(GuiVertex{ x,           y, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x,      uvTopLeft.y,      0.0f }); // topLeft
		   guiVertices.push_back(GuiVertex{ x,      y + dy, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x,      uvTopLeft.y + dy, 0.0f }); // bottomLeft
		   guiVertices.push_back(GuiVertex{ x + dx, y + dy, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x + dx, uvTopLeft.y + dy, 0.0f }); // bottomRigh
		   guiVertices.push_back(GuiVertex{ x,           y, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x,      uvTopLeft.y,      0.0f }); // topLeft
		   guiVertices.push_back(GuiVertex{ x + dx,      y, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x + dx, uvTopLeft.y,      0.0f }); // TopRight
		   guiVertices.push_back(GuiVertex{ x + dx, y + dy, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x + dx, uvTopLeft.y + dy, 0.0f }); // bottomRight
	   }

	   enum RenderState
	   {
		   RenderState_None,
		   RenderState_Gui,
		   RenderState_Sprites
	   } renderState{ RenderState_None };

	   void FlushLayer()
	   {
		   size_t nVerticesLeftToRender = guiVertices.size();
		   while (nVerticesLeftToRender > 0)
		   {
			   if (renderState != RenderState_Gui)
			   {
				   if (!UseShaders(idGuiVS, idGuiPS))
				   {
					   Throw(0, "Error setting Gui shaders");
				   }

				   renderState = RenderState_Gui;

				   dc.PSSetSamplers(0, 1, &spriteSampler);
				   dc.PSSetShaderResources(0, 1, &fontBinding);
				   dc.RSSetState(spriteRaterizering);

				   FLOAT blendFactorUnused[] = { 0,0,0,0 };
				   dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);

				   GuiScale guiScaleVector;
				   guiScaleVector.OOScreenWidth = 1.0f / screenSpan.x;
				   guiScaleVector.OOScreenHeight = 1.0f / screenSpan.y;
				   guiScaleVector.OOFontWidth = fonts->TextureSpan().z;
				   guiScaleVector.OOFontHeight = fonts->TextureSpan().w;

				   DX11::CopyStructureToBuffer(dc, vector4Buffer, &guiScaleVector, sizeof(GuiScale));

				   dc.VSSetConstantBuffers(0, 1, &vector4Buffer);
				   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				   dc.OMSetDepthStencilState(guiDepthState, 0);
			   }

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

		   size_t nSpriteVerticesLeftToRender = textureArray.spriteTriangles.size();
		   while (nSpriteVerticesLeftToRender > 0)
		   {
			   if (isBuildingAlphaBlendedSprites)
			   {
				   FLOAT blendFactorUnused[] = { 0,0,0,0 };
				   dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);
			   }
			   else
			   {
				   FLOAT blendFactorUnused[] = { 0,0,0,0 };
				   dc.OMSetBlendState(this->disableBlend, blendFactorUnused, 0xffffffff);
			   }

			   if (renderState != RenderState_Sprites)
			   {
				   renderState = RenderState_Sprites;

				   if (!UseShaders(idSpriteVS, idSpritePS))
				   {
					   Throw(0, "Could not set sprite shaders");
				   }

				   dc.PSSetSamplers(0, 1, &spriteSampler);
				   ID3D11ShaderResourceView* views[1] = { textureArray.View() };
				   dc.PSSetShaderResources(0, 1, views);
				   dc.RSSetState(spriteRaterizering);

				   Vec4 guiScaleVector;
				   guiScaleVector.x = 1.0f / screenSpan.x;
				   guiScaleVector.y = 1.0f / screenSpan.y;
				   guiScaleVector.z = 1.0f / (float32)textureArray.width;
				   guiScaleVector.w = 0.0f;

				   DX11::CopyStructureToBuffer(dc, vector4Buffer, &guiScaleVector, sizeof(Vec4));

				   dc.VSSetConstantBuffers(0, 1, &vector4Buffer);
				   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				   dc.OMSetDepthStencilState(guiDepthState, 0);
			   }

			   size_t startIndex = textureArray.spriteTriangles.size() - nSpriteVerticesLeftToRender;
			   size_t chunk = min(nSpriteVerticesLeftToRender, (size_t)GUI_BUFFER_VERTEX_CAPACITY);

			   D3D11_MAPPED_SUBRESOURCE x;
			   VALIDATEDX11(dc.Map(guiBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &x));
			   memcpy(x.pData, &textureArray.spriteTriangles[startIndex], chunk * sizeof(GuiVertex));
			   dc.Unmap(guiBuffer, 0);

			   UINT stride = sizeof(GuiVertex);
			   UINT offset = 0;
			   dc.IASetVertexBuffers(0, 1, &guiBuffer, &stride, &offset);
			   dc.Draw((UINT)chunk, 0);

			   nSpriteVerticesLeftToRender -= chunk;

			   textureArray.spriteTriangles.clear();
		   }
	   }

	   void ResizeBuffers(const Vec2i& span)
	   {
		   mainBackBufferView.Detach();

		   VALIDATEDX11(mainSwapChain->ResizeBuffers(1, span.x, span.y, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

		   AutoRelease<ID3D11Texture2D> backBuffer;
		   VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));
		   VALIDATEDX11(device.CreateRenderTargetView(backBuffer, nullptr, &mainBackBufferView));

		   depthStencilView.Detach();

		   AutoRelease<ID3D11Texture2D> depthBuffer;

		   auto depthDesc = DX11::GetDepthDescription(hRenderWindow);
		   VALIDATEDX11(device.CreateTexture2D(&depthDesc, nullptr, &depthBuffer));
		   VALIDATEDX11(device.CreateDepthStencilView(depthBuffer, nullptr, &depthStencilView));

		   SyncViewport();
	   }

	   virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
	   {
		   if (span.x > 0 && span.y > 0)
		   {
			   D3D11_TEXTURE2D_DESC desc;
			   {
				   AutoRelease<ID3D11Texture2D> backBuffer;
				   VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));
				   backBuffer->GetDesc(&desc);
			   }

			   if (desc.Width != span.x || desc.Height != span.y)
			   {
				   ResizeBuffers(span);
			   }

			   BOOL isFullScreen;
			   IDXGIOutput* output = nullptr;
			   mainSwapChain->GetFullscreenState(&isFullScreen, &output);
			   if (isFullScreen)
			   {
				   output->Release();
			   }
			   else
			   {
				   // Style can ben lost when dialog invoked in full screen mode. So add it here
				   DWORD style = GetWindowLong(hWnd, GWL_STYLE);
				   if ((style & WS_CAPTION) == 0)
				   {
					   SetWindowLong(hWnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
				   }
			   }
		   }
	   }

	   void SetCursorBitmap(ID_TEXTURE idBitmap, Vec2i hotspotOffset, Vec2 uvTopLeft, Vec2 uvBottomRight) override
	   {
		   cursor.bitmapId = idBitmap;
		   cursor.uvTopLeft = uvTopLeft;
		   cursor.uvBottomRight = uvBottomRight;
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
					   RECT rect;
					   GetClientRect(hRenderWindow, &rect);
					   ClipCursor(&rect);
					   SetCapture(hRenderWindow);
					   return;
				   }
			   }
		   }
	   }
   };

   struct DX11Host
   {
	   DX11Host()
	   {

	   }

	   ~DX11Host()
	   {
		   device = nullptr;
		   dc = nullptr;
		   adapter = nullptr;
		   factory = nullptr;
		   if (debug)
		   {
			   //   debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
			   debug = nullptr;
		   }
	   }
	   AutoRelease<IDXGIAdapter> adapter;
	   AutoRelease<ID3D11DeviceContext> dc;
	   AutoRelease<ID3D11Device> device;
	   AutoRelease<IDXGIFactory> factory;
	   AutoRelease<ID3D11Debug> debug;
   };

	class MainWindowHandler : public StandardWindowHandler
	{
	private:
		AutoFree<IDialogSupervisor> window;
      AutoFree<IExpandingBuffer> eventBuffer;

		IAppEventHandler& eventHandler;
		bool hasFocus;

		MainWindowHandler(IAppEventHandler& _eventHandler) :
         eventHandler(_eventHandler), 
         hasFocus(false),
         eventBuffer(CreateExpandingBuffer(128))
		{

		}

		~MainWindowHandler()
		{
		}

		void OnGetMinMaxInfo(HWND hWnd, MINMAXINFO& info)
		{
			enum { DEFAULT_MIN_WIDTH = 1024, DEFAULT_MIN_HEIGHT = 640 };

			info.ptMinTrackSize.x = DEFAULT_MIN_WIDTH;
			info.ptMinTrackSize.y = DEFAULT_MIN_HEIGHT;
		}

		void PostConstruct()
		{
			WindowConfig config;
			SetOverlappedWindowConfig(config, Vec2i{ 1024, 640 }, SW_SHOWMAXIMIZED, nullptr, "DX11 64-bit Rococo API Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);
			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler
			eventHandler.BindMainWindow(*window);

			RegisterRawInput();
			hasFocus = true;
		}

		void RegisterRawInput()
		{
			RAWINPUTDEVICE mouseDesc;
			mouseDesc.hwndTarget = *window;
			mouseDesc.dwFlags = 0;
			mouseDesc.usUsage = 0x02;
			mouseDesc.usUsagePage = 0x01;
			if (!RegisterRawInputDevices(&mouseDesc, 1, sizeof(mouseDesc)))
			{
				Throw(GetLastError(), "RegisterRawInputDevices(&mouseDesc, 1, sizeof(mouseDesc) failed");
			}

			RAWINPUTDEVICE keyboardDesc;
			keyboardDesc.hwndTarget = *window;
			keyboardDesc.dwFlags = 0;
			keyboardDesc.usUsage = 0x06;
			keyboardDesc.usUsagePage = 0x01;
			if (!RegisterRawInputDevices(&keyboardDesc, 1, sizeof(keyboardDesc)))
			{
				Throw(GetLastError(), "RegisterRawInputDevices(&keyboardDesc, 1, sizeof(keyboardDesc)) failed");
			}
		}
	public:
		// This is our post construct pattern. Allow the constructor to return to initialize the v-tables, then call PostConstruct to create the window 
		static MainWindowHandler* Create(IAppEventHandler& _eventHandler)
		{
			auto m = new MainWindowHandler(_eventHandler);
			m->PostConstruct();
			return m;
		}

		void Free()
		{
			delete this;
		}

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case WM_SETFOCUS:
				hasFocus = true;
				RegisterRawInput();
				break;
			case WM_KILLFOCUS:
				hasFocus = false;
				break;
			}

			return StandardWindowHandler::OnMessage(hWnd, uMsg, wParam, lParam);
		}

		virtual void OnMenuCommand(HWND hWnd, DWORD id)
		{

		}

		LRESULT OnInput(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			UINT sizeofBuffer;
			if (NO_ERROR != GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &sizeofBuffer, sizeof(RAWINPUTHEADER)))
			{
				return DefWindowProc(hWnd, WM_INPUT, wParam, lParam);
			}


         eventBuffer->Resize(sizeofBuffer);

         char* buffer = (char*) eventBuffer->GetData();

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &sizeofBuffer, sizeof(RAWINPUTHEADER)) == (UINT)-1)
			{
				return DefWindowProc(hWnd, WM_INPUT, wParam, lParam);
			}

			RAWINPUT& raw = *((RAWINPUT*)buffer);
			RAWINPUT* pRaw = &raw;

			if (hasFocus)
			{
				if (raw.header.dwType == RIM_TYPEMOUSE)
				{
					eventHandler.OnMouseEvent(raw.data.mouse);
				}
				else if (raw.header.dwType == RIM_TYPEKEYBOARD)
				{
					eventHandler.OnKeyboardEvent(raw.data.keyboard);
				}

				return 0;
			}
			else
			{
				return DefRawInputProc(&pRaw, 1, sizeof(RAWINPUTHEADER));
			}
		}

		virtual void OnClose(HWND hWnd)
		{
			rchar text[256];
			GetWindowTextA(hWnd, text, 255);
			text[255] = 0;
			if (eventHandler.OnClose())
			{
				AutoFree<DX11::ICountdownConfirmationDialog> confirmDialog(DX11::CreateCountdownConfirmationDialog());
				if (IDOK == confirmDialog->DoModal(hWnd, text, "Quitting application", 8))
				{
					PostQuitMessage(0);
				}
			}
		}

		virtual IDialogSupervisor& Window()
		{
			return *window;
		}

		virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
		{
			eventHandler.OnSize(hWnd, span, type);
		}
	};

	enum { IDSHOWMODAL = 1001 };

	void Create_DX11_0_Host(UINT adapterIndex, DX11Host& host)
	{
		VALIDATEDX11(CreateDXGIFactory(IID_IDXGIFactory, (void**)&host.factory));
		VALIDATEDX11(host.factory->EnumAdapters(adapterIndex, &host.adapter));

		D3D_FEATURE_LEVEL featureLevelNeeded[] = { D3D_FEATURE_LEVEL_11_0 };
		D3D_FEATURE_LEVEL featureLevelFound;

      UINT flags;

#ifdef _DEBUG
      flags = D3D11_CREATE_DEVICE_DEBUG;
#else
      flags = 0;
#endif
      flags |= D3D11_CREATE_DEVICE_SINGLETHREADED;

		VALIDATEDX11(D3D11CreateDevice(host.adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
			featureLevelNeeded, 1, D3D11_SDK_VERSION, &host.device, &featureLevelFound, &host.dc
			));

		if (featureLevelFound != D3D_FEATURE_LEVEL_11_0)
		{
			Throw(0, "DX 11.0 is required for this application");
		}

      host.device->QueryInterface(IID_PPV_ARGS(&host.debug));
	}

   struct UltraClock : public IUltraClock
   {
      OS::ticks start;
      OS::ticks frameStart;
      OS::ticks frameDelta;
      Seconds dt;

      virtual OS::ticks FrameStart() const
      {
         return frameStart;
      }
      
      virtual OS::ticks Start() const
      {
         return start;
      }

      virtual OS::ticks FrameDelta() const
      {
         return frameDelta;
      }

      virtual Seconds DT() const
      {
         return dt;
      }
   };

   void MainLoop(MainWindowHandler& mainWindow, HANDLE hInstanceLock, IApp& app)
   {
	   UltraClock uc;
	   OS::ticks lastTick = uc.start;
	   OS::ticks frameCost = 0;

	   float hz = (float)OS::CpuHz();

	   uint32 sleepMS = 5;
	   MSG msg = { 0 };
	   while (msg.message != WM_QUIT)
	   {
		   int64 msCost = frameCost / (OS::CpuHz() / 1000);

		   int64 iSleepMS = sleepMS - msCost;

		   if (iSleepMS < 0)
		   {
			   sleepMS = 0;
		   }
		   else if (sleepMS > (uint32) iSleepMS)
		   {
			   sleepMS = (uint32) iSleepMS;
		   }
		   DWORD status = MsgWaitForMultipleObjectsEx(1, &hInstanceLock, sleepMS, QS_ALLEVENTS, MWMO_ALERTABLE);

		   OS::ticks now = OS::CpuTicks();

		   if (status == WAIT_OBJECT_0)
		   {
			   ResetEvent(hInstanceLock);
			   SetForegroundWindow(mainWindow.Window());
		   }

		   while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		   {
			   TranslateMessage(&msg);
			   DispatchMessage(&msg);

			   if (msg.message == WM_QUIT)
			   {
				   return;
			   }
		   }

		   RECT rect;
		   GetClientRect(mainWindow.Window(), &rect);

		   if ((rect.right - rect.left) == 0)
		   {
			   sleepMS = 1000;
			   continue;
		   }

		   QueryPerformanceCounter((LARGE_INTEGER*)&uc.frameStart);

		   uc.frameDelta = uc.frameStart - lastTick;

		   float dt0 = uc.frameDelta / (float)hz;
		   dt0 = max(0.0f, dt0);
		   dt0 = min(dt0, 0.05f);
		   uc.dt = Seconds{ dt0 };

		   sleepMS = app.OnFrameUpdated(uc);

		   frameCost = OS::CpuTicks() - now;

		   lastTick = uc.frameStart;
	   }
   }
}

namespace
{
	class DX11Window : public IDX11Window, public IAppEventHandler, public IEventCallback<SysUnstableArgs>
	{
		IInstallation& installation;
		DX11Host host;
		IApp* app{ nullptr };
		DX11AppRenderer* renderer{ nullptr };
		AutoFree<MainWindowHandler> mainWindowHandler;

		IWindow& Window()
		{
			return mainWindowHandler->Window();
		}

		virtual void BindMainWindow(HWND hWnd)
		{
			renderer->BindMainWindow(hWnd);
		}

		virtual bool OnClose()
		{
			renderer->SwitchToWindowMode();
			return true;
		}

		virtual void OnKeyboardEvent(const RAWKEYBOARD& k)
		{
			if (app) app->OnKeyboardEvent((const KeyboardEvent&)k);
		}

		virtual void OnMouseEvent(const RAWMOUSE& m)
		{
			renderer->OnMouseEvent(m);

			MouseEvent me;
			memcpy(&me, &m, sizeof(m));

			POINT p;
			GetCursorPos(&p);
			ScreenToClient(renderer->Window(), &p);

			me.cursorPos.x = p.x;
			me.cursorPos.y = p.y;

			if (app) app->OnMouseEvent(me);
		}

		virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
		{
			renderer->OnSize(hWnd, span, type);
		}

		virtual void OnEvent(SysUnstableArgs& arg)
		{
			renderer->SwitchToWindowMode();
		}

		virtual IRenderer& Renderer()
		{
			return *renderer;
		}
	public:
		DX11Window(IInstallation& _installation) : installation(_installation)
		{
			Create_DX11_0_Host(0, host);
			renderer = new  DX11AppRenderer(*host.device, *host.dc, *host.factory, installation);
			mainWindowHandler = MainWindowHandler::Create(*this);
			VALIDATEDX11(host.factory->MakeWindowAssociation(mainWindowHandler->Window(), 0));
			renderer->window = &mainWindowHandler->Window();
			installation.OS().SetUnstableHandler(this);
		}

		virtual void Free()
		{
			installation.OS().SetUnstableHandler(nullptr);
			renderer->Free();
			delete this;
		}

		void Run(HANDLE hInstanceLock, IApp& app)
		{
			this->app = &app;
			MainLoop(*mainWindowHandler, hInstanceLock, app);
			this->app = nullptr;
		}
	};
}

namespace Rococo
{ 
	IDX11Window* CreateDX11Window(IInstallation& installation)
	{
		return new DX11Window(installation);
	}
}

