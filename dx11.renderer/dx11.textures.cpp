#include "rococo.dx11.api.h"
#include "dx11helpers.inl"

#include <rococo.imaging.h>
#include <rococo.io.h>
#include <rococo.strings.h>

#include <string>

namespace
{
   using namespace Rococo;

   class TextureParser : public Imaging::IImageLoadEvents
   {
      ID3D11Device& device;
      ID3D11DeviceContext& dc;
      ID3D11Texture2D* texture;
      std::wstring name;
      bool allowAlpha;
      bool allowColour;

      virtual void OnError(const char* message)
      {
         Throw(0, L"Could not load %s\n%S", name.c_str(), message);
      }

      virtual void OnARGBImage(const Vec2i& span, const Imaging::F_A8R8G8B8* data)
      {
         if (!allowColour)
         {
            OnError("Only 8-bit alpha (greyscale) formats allowed.");
         }

         if (span.x > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || span.y > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
         {
            Throw(0, L"Image %s exceeded maximum span allowed by DX11.", name.c_str());
         }

         D3D11_TEXTURE2D_DESC colourSprite;
         colourSprite.Width = span.x;
         colourSprite.Height = span.y;
         colourSprite.MipLevels = 0;
         colourSprite.ArraySize = 1;
         colourSprite.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
         colourSprite.SampleDesc.Count = 1;
         colourSprite.SampleDesc.Quality = 0;
         colourSprite.Usage = D3D11_USAGE_DEFAULT; // D3D11_USAGE_IMMUTABLE;
         colourSprite.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
         colourSprite.CPUAccessFlags = 0;
         colourSprite.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

         struct ABGR8
         {
            uint8 alpha;
            uint8 blue;
            uint8 green;
            uint8 red;
            
            ABGR8() {}
            ABGR8(uint32 x) { ABGR8* pCol = (ABGR8*)&x; *this = *pCol; }
            ABGR8(uint8 _red, uint8 _green, uint8 _blue, uint8 _alpha = 255) : red(_red), green(_green), blue(_blue), alpha(_alpha) {}
         };

         RGBAb* target = (RGBAb*)data;
         for (int i = 0; i < span.x * span.y; ++i)
         {
            Imaging::F_A8R8G8B8 col = data[i];
            RGBAb twizzled(col.r, col.g, col.b, col.a);
            target[i] = twizzled;
         }
         /*
         D3D11_SUBRESOURCE_DATA level0Def;
         level0Def.pSysMem = data;
         level0Def.SysMemPitch = span.x * sizeof(RGBAb);
         level0Def.SysMemSlicePitch = 0;
         */
         VALIDATEDX11(device.CreateTexture2D(&colourSprite, nullptr, &texture));
         dc.UpdateSubresource(texture, 0, nullptr, data, span.x * sizeof(RGBAb), 0);
      }

      virtual void OnAlphaImage(const Vec2i& span, const uint8* data)
      {
         if (!allowAlpha)
         {
            OnError("Only 24-bit or 32-bit colour formats allowed.");
         }

         if (span.x > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || span.y > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
         {
            Throw(0, L"Image %s exceeded maximum span allowed by DX11.", name.c_str());
         }

         D3D11_TEXTURE2D_DESC alphaImmutableSprite;
         alphaImmutableSprite.Width = span.x;
         alphaImmutableSprite.Height = span.y;
         alphaImmutableSprite.MipLevels = 1;
         alphaImmutableSprite.ArraySize = 1;
         alphaImmutableSprite.Format = DXGI_FORMAT_R8_UNORM;
         alphaImmutableSprite.SampleDesc.Count = 1;
         alphaImmutableSprite.SampleDesc.Quality = 0;
         alphaImmutableSprite.Usage = D3D11_USAGE_IMMUTABLE;
         alphaImmutableSprite.BindFlags = D3D11_BIND_SHADER_RESOURCE;
         alphaImmutableSprite.CPUAccessFlags = 0;
         alphaImmutableSprite.MiscFlags = 0;

         D3D11_SUBRESOURCE_DATA level0Def;
         level0Def.pSysMem = data;
         level0Def.SysMemPitch = span.x;
         level0Def.SysMemSlicePitch = 0;

         VALIDATEDX11(device.CreateTexture2D(&alphaImmutableSprite, &level0Def, &texture));
      }
   public:
      TextureParser(ID3D11Device& _device, ID3D11DeviceContext& _dc, const wchar_t* filename, bool _allowAlpha, bool _allowColour) :
         device(_device), name(filename), allowAlpha(_allowAlpha), allowColour(_allowColour), dc(_dc)
      {
      }

      ID3D11Texture2D* Texture() { return texture; }
   };
}

namespace Rococo
{
   namespace DX11
   {
      ID3D11Texture2D* LoadAlphaBitmap(ID3D11Device& device, ID3D11DeviceContext& dc, const uint8* buffer, size_t nBytes, const wchar_t* resourceName)
      {
         TextureParser parser(device, dc, resourceName, true, false);
         Rococo::Imaging::DecompressTiff(parser, buffer, nBytes);
         return parser.Texture();
      }

      ID3D11Texture2D* LoadColourBitmap(ID3D11Device& device, ID3D11DeviceContext& dc, const uint8* buffer, size_t nBytes, const wchar_t* resourceName)
      {
         TextureParser parser(device, dc, resourceName, false, true);

         auto* ext = GetFileExtension(resourceName);
         if (Eq(ext, L".tiff") || Eq(ext, L".tif"))
         {
            Rococo::Imaging::DecompressTiff(parser, buffer, nBytes);
         }
         else
         {
            Rococo::Imaging::DecompressJPeg(parser, buffer, nBytes);
         }
         
         return parser.Texture();
      }

      TextureLoader::TextureLoader(IInstallation& _installation, ID3D11Device& _device, ID3D11DeviceContext& _dc, IExpandingBuffer& _scratchBuffer):
         installation(_installation),
         device(_device), 
         dc(_dc),
         scratchBuffer(_scratchBuffer)
      {

      }

      TextureBind TextureLoader::LoadColourBitmap(const wchar_t* resourceName)
      {
         auto* ext = GetFileExtension(resourceName);
         if (!Eq(ext, L".tiff") && !Eq(ext, L".tif") && !Eq(ext, L".jpg") && !Eq(ext, L".jpg"))
         {
            Throw(0, L"%s\nOnly Tiff and Jpeg files can be used for colour bitmaps", resourceName);
         }

         installation.LoadResource(resourceName, scratchBuffer, 64_megabytes);

         if (scratchBuffer.Length() == 0) Throw(0, L"The image file %s was blank", resourceName);

         auto* tx = DX11::LoadColourBitmap(device, dc, scratchBuffer.GetData(), scratchBuffer.Length(), resourceName);

         D3D11_SHADER_RESOURCE_VIEW_DESC desc;
         ZeroMemory(&desc, sizeof(desc));

         desc.Texture2D.MipLevels = -1;
         desc.Texture2D.MostDetailedMip = 0;

         desc.Format = DXGI_FORMAT_UNKNOWN;
         desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

         ID3D11ShaderResourceView* view = nullptr;
         HRESULT hr = device.CreateShaderResourceView(tx, &desc, &view);

         if FAILED(hr)
         {
            tx->Release();
            Throw(hr, L"Failed to CreateShaderResourceVie for %s", resourceName);
         }

         dc.GenerateMips(view);

         return{ tx, view };
      }

      TextureBind TextureLoader::LoadAlphaBitmap(const wchar_t* resourceName)
      {
         auto* ext = GetFileExtension(resourceName);
         if (!Eq(ext, L".tiff") && !Eq(ext, L".tif"))
         {
            Throw(0, L"%s\nOnly Tiff files can be used for alpha bitmaps", resourceName);
         }

         installation.LoadResource(resourceName, scratchBuffer, 64_megabytes);

         if (scratchBuffer.Length() == 0) Throw(0, L"The image file %s was blank", resourceName);

         auto* alpha = DX11::LoadAlphaBitmap(device, dc, scratchBuffer.GetData(), scratchBuffer.Length(), resourceName);

         D3D11_SHADER_RESOURCE_VIEW_DESC desc;
         ZeroMemory(&desc, sizeof(desc));

         desc.Texture2D.MipLevels = 1;
         desc.Texture2D.MostDetailedMip = 0;

         desc.Format = DXGI_FORMAT_UNKNOWN;
         desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

         ID3D11ShaderResourceView* view = nullptr;
         HRESULT hr = device.CreateShaderResourceView(alpha, &desc, &view);
         
         if FAILED(hr)
         {
            alpha->Release();
            Throw(hr, L"Failed to CreateShaderResourceVie for %s", resourceName);
         }

         return{ alpha, view };
      }
   } // DX11
} // Rococo