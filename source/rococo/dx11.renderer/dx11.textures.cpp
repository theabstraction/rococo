#include "dx11.renderer.h"
#include "dx11helpers.inl"

#include <rococo.imaging.h>
#include <rococo.io.h>
#include <rococo.renderer.formats.h>

#include <string>

using namespace Rococo::Strings;
using namespace Rococo::Graphics;

namespace
{
   using namespace Rococo;

   class TextureParser : public Imaging::IImageLoadEvents
   {
      ID3D11Device& device;
      ID3D11DeviceContext& dc;
      ID3D11Texture2D* texture;
      std::string name;
      bool allowAlpha;
      bool allowColour;

      virtual void OnError(const char* message)
      {
         Throw(0, "Could not load %s\n%s", name.c_str(), message);
      }

      virtual void OnRGBAImage(const Vec2i& span, const RGBAb* data)
      {
         if (!allowColour)
         {
            OnError("Only 8-bit alpha (greyscale) formats allowed.");
         }

         if (span.x > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || span.y > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
         {
            Throw(0, "Image %s exceeded maximum span allowed by DX11.", name.c_str());
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
            Throw(0, "Image %s exceeded maximum span allowed by DX11.", name.c_str());
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
      TextureParser(ID3D11Device& _device, ID3D11DeviceContext& _dc, cstr filename, bool _allowAlpha, bool _allowColour) :
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
      ID3D11Texture2D* LoadAlphaBitmap(ID3D11Device& device, ID3D11DeviceContext& dc, const uint8* buffer, size_t nBytes, cstr resourceName)
      {
         TextureParser parser(device, dc, resourceName, true, false);
         Rococo::Imaging::DecompressTiff(parser, buffer, nBytes);
         return parser.Texture();
      }

      ID3D11Texture2D* LoadColourBitmap(ID3D11Device& device, ID3D11DeviceContext& dc, const uint8* buffer, size_t nBytes, cstr resourceName)
      {
         TextureParser parser(device, dc, resourceName, false, true);

         auto* ext = GetFileExtension(resourceName);
         if (Eq(ext, ".tiff") || Eq(ext, ".tif"))
         {
            Rococo::Imaging::DecompressTiff(parser, buffer, nBytes);
         }
         else
         {
            Rococo::Imaging::DecompressJPeg(parser, buffer, nBytes);
         }
         
         return parser.Texture();
      }

      TextureLoader::TextureLoader(IO::IInstallation& _installation, ID3D11Device& _device, ID3D11DeviceContext& _dc, IExpandingBuffer& _scratchBuffer):
         installation(_installation),
         device(_device), 
         dc(_dc),
         scratchBuffer(_scratchBuffer)
      {

      }

      ID3D11Texture2D* TextureLoader::LoadColourBitmapDirect(cstr resourceName)
      {
          auto* ext = GetFileExtension(resourceName);
          if (!Eq(ext, ".tiff") && !Eq(ext, ".tif") && !Eq(ext, ".jpg") && !Eq(ext, ".jpg"))
          {
              Throw(0, "%s\nOnly Tiff and Jpeg files can be used for colour bitmaps", resourceName);
          }

          installation.LoadResource(resourceName, scratchBuffer, 64_megabytes);

          if (scratchBuffer.Length() == 0) Throw(0, "The image file %s was blank", resourceName);

          auto* tx = DX11::LoadColourBitmap(device, dc, scratchBuffer.GetData(), scratchBuffer.Length(), resourceName);
          return tx;
      }

      TextureBind TextureLoader::LoadColourBitmap(cstr resourceName)
      {
         auto* ext = GetFileExtension(resourceName);
         if (!Eq(ext, ".tiff") && !Eq(ext, ".tif") && !Eq(ext, ".jpg") && !Eq(ext, ".jpg"))
         {
            Throw(0, "%s\nOnly Tiff and Jpeg files can be used for colour bitmaps", resourceName);
         }

         installation.LoadResource(resourceName, scratchBuffer, 64_megabytes);

         if (scratchBuffer.Length() == 0) Throw(0, "The image file %s was blank", resourceName);

         auto* tx = DX11::LoadColourBitmap(device, dc, scratchBuffer.GetData(), scratchBuffer.Length(), resourceName);

         D3D11_SHADER_RESOURCE_VIEW_DESC desc;
         ZeroMemory(&desc, sizeof(desc));

         desc.Texture2D.MipLevels = (UINT) - 1;
         desc.Texture2D.MostDetailedMip = 0;

         desc.Format = DXGI_FORMAT_UNKNOWN;
         desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

         ID3D11ShaderResourceView* view = nullptr;
         HRESULT hr = device.CreateShaderResourceView(tx, &desc, &view);

         if FAILED(hr)
         {
            tx->Release();
            Throw(hr, "Failed to CreateShaderResourceVie for %s", resourceName);
         }

         dc.GenerateMips(view);

         return{ tx, view };
      }

	  void TextureLoader::LoadColourBitmapIntoAddress(cstr resourceName, IColourBitmapLoadEvent& onLoad)
	  {
		  auto* ext = GetFileExtension(resourceName);
		  if (!Eq(ext, ".tiff") && !Eq(ext, ".tif") && !Eq(ext, ".jpg") && !Eq(ext, ".jpg"))
		  {
			  Throw(0, "%s\nOnly Tiff and Jpeg files can be used for colour bitmaps", resourceName);
		  }

		  installation.LoadResource(resourceName, scratchBuffer, 64_megabytes);

		  if (scratchBuffer.Length() == 0) Throw(0, "The image file %s was blank", resourceName);

		  struct ANON : public Rococo::Imaging::IImageLoadEvents
		  {
			  IColourBitmapLoadEvent* onLoad;

			  void OnError(const char* message) override
			  {
				  Throw(0, "Error loading image file. %s", message);
			  }

			  void OnRGBAImage(const Vec2i& span, const RGBAb* data) override
			  {
				  onLoad->OnLoad(data, span);
			  }

			  void OnAlphaImage(const Vec2i&, const uint8*) override
			  {
				  Throw(0, "The image file was alpha. 32-bit colour image expected");
			  }
		  } loadHandler;

		  loadHandler.onLoad = &onLoad;

		  if (Eq(ext, ".tiff") || Eq(ext, ".tif"))
		  {
			  Rococo::Imaging::DecompressTiff(loadHandler, scratchBuffer.GetData(), scratchBuffer.Length());
		  }
		  else
		  {
			  Rococo::Imaging::DecompressJPeg(loadHandler, scratchBuffer.GetData(), scratchBuffer.Length());
		  }
	  }

      TextureBind TextureLoader::LoadAlphaBitmap(cstr resourceName)
      {
         auto* ext = GetFileExtension(resourceName);
         if (!Eq(ext, ".tiff") && !Eq(ext, ".tif"))
         {
            Throw(0, "%s\nOnly Tiff files can be used for alpha bitmaps", resourceName);
         }

         installation.LoadResource(resourceName, scratchBuffer, 64_megabytes);

         if (scratchBuffer.Length() == 0) Throw(0, "The image file %s was blank", resourceName);

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
            Throw(hr, "Failed to CreateShaderResourceVie for %s", resourceName);
         }

         return{ alpha, view };
      }

      void GetTextureDesc(TextureDesc& desc, ID3D11Texture2D& texture)
      {
          D3D11_TEXTURE2D_DESC edesc;
          texture.GetDesc(&edesc);

          desc.width = edesc.Width;
          desc.height = edesc.Height;

          switch (edesc.Format)
          {
          case DXGI_FORMAT_R8G8B8A8_TYPELESS:
              desc.format = TextureFormat::F_RGBA_32_BIT;
              break;
          case DXGI_FORMAT_R32_TYPELESS:
              desc.format = TextureFormat::F_32_BIT_FLOAT;
              break;
          default:
              desc.format = TextureFormat::F_UNKNOWN;
          }
      }
   } // DX11
} // Rococo