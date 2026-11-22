#include "dx11.renderer.h"
#include <rococo.renderer.h>
#include "rococo.dx11.api.h"
#include "rococo.textures.h"
#include "dx11helpers.inl"
#include "rococo.imaging.h"
#include "rococo.dds.win32.h"

using namespace Rococo;
using namespace Rococo::DX11;
using namespace Rococo::Strings;
using namespace Rococo::Graphics;
using namespace Rococo::Graphics::Textures;

struct DX11BitmapArray : public IDX11BitmapArray
{
    DX11::TextureBind tb = { 0 };
    DXGI_FORMAT format = DXGI_FORMAT_NV11;
    int32 width{ 0 };
    int32 height{ 0 };
    ID3D11Device& device;
    bool isMipMapped = false;

    ID3D11DeviceContext* activeDC = nullptr;

    size_t arrayCapacity{ 0 };
    size_t count{ 0 };

    bool genMipMaps;

    DX11BitmapArray(ID3D11Device& _device, ID3D11DeviceContext& dc, bool _genMipMaps) :
        device(_device), activeDC(&dc), genMipMaps(_genMipMaps)
    {
    }

    ~DX11BitmapArray()
    {
        Clear();
    }

    DX11::TextureBind& Binding()
    {
        return tb;
    }

    void Free() override
    {
        delete this;
    }

    void GenMipMaps() override
    {
        if (!genMipMaps)
        {
            return;
        }

        if (!tb.texture)
        {
            return;
        }

        if (isMipMapped)
        {
            return;
        }

        if (!activeDC)
        {
            Throw(0, "No active DC for GenMipMaps");
        }

        ID3D11ShaderResourceView* srv = CreateSRV();
        activeDC->GenerateMips(srv);
        srv->Release();

        isMipMapped = true;
    }

    void Clear()
    {
        if (tb.texture) tb.texture->Release();
        if (tb.shaderView) tb.shaderView->Release();
        tb.desc = { 0 };
        tb.texture = nullptr;
        tb.shaderView = nullptr;
        count = 0;
        arrayCapacity = 0;
        isMipMapped = false;
    }

    void AddTexture() override
    {
        if (arrayCapacity != 0)
        {
            Throw(0, "DX11TextureArray texture is already defined. You cannot add more textures.");
        }
        count++;
    }

    void ResetWidth(int32 width) override
    {
        Clear();
        this->width = width;
        this->height = width;
    }

    void ResetWidth(int32 width, int32 height) override
    {
        Clear();
        this->width = width;
        this->height = height;
    }

    void Resize(size_t nElements) override
    {
        Clear();
        arrayCapacity = count = nElements;
    }

    void WriteSubImage(size_t index, const RGBAb* pixels, const GuiRect& targetLocation) override
    {
        if (!activeDC)
        {
            Throw(0, "%s: no active DC", __ROCOCO_FUNCTION__);
        }

        if (width <= 0)
        {
            Throw(0, "Cannot WriteSubImage into texture, the texture width was zero.");
        }

        if (width > 0 && tb.texture == nullptr)
        {
            arrayCapacity = count;
            format = DXGI_FORMAT_R8G8B8A8_UNORM;

            D3D11_TEXTURE2D_DESC colourSpriteArray;
            colourSpriteArray.Width = width;
            colourSpriteArray.Height = height;
            colourSpriteArray.MipLevels = genMipMaps ? 0 : 1;
            colourSpriteArray.ArraySize = (UINT)arrayCapacity;
            colourSpriteArray.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            colourSpriteArray.SampleDesc.Count = 1;
            colourSpriteArray.SampleDesc.Quality = 0;
            colourSpriteArray.Usage = D3D11_USAGE_DEFAULT;
            colourSpriteArray.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            colourSpriteArray.CPUAccessFlags = 0;
            colourSpriteArray.MiscFlags = 0;
            if (genMipMaps) colourSpriteArray.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
            VALIDATEDX11(device.CreateTexture2D(&colourSpriteArray, nullptr, &tb.texture));
            tb.texture->GetDesc(&tb.desc);
        }

        if (width > 0)
        {
            if (format != DXGI_FORMAT_R8G8B8A8_UNORM)
            {
                Throw(0, "DX11TextureArray::format is not RGBA but image passed was RGBA");
            }

            UINT subresourceIndex = D3D11CalcSubresource(0, (UINT)index, tb.desc.MipLevels);
            Vec2i span = Span(targetLocation);
            D3D11_BOX box;
            box.left = targetLocation.left;
            box.right = targetLocation.right;
            box.back = 1;
            box.front = 0;
            box.top = targetLocation.top;
            box.bottom = targetLocation.bottom;

            UINT srcDepth = span.x * span.y * sizeof(RGBAb);
            if (tb.texture) activeDC->UpdateSubresource(tb.texture, subresourceIndex, &box, pixels, span.x * sizeof(RGBAb), srcDepth);
        }
    }

    void WriteSubImage(size_t index, const uint8* grayScalePixels, Vec2i span) override
    {
        if (!activeDC)
        {
            Throw(0, "%s: no active DC", __ROCOCO_FUNCTION__);
        }

        if (width > 0 && tb.texture == nullptr)
        {
            arrayCapacity = count;
            format = DXGI_FORMAT_R8_UNORM;

            D3D11_TEXTURE2D_DESC alphArray;
            alphArray.Width = width;
            alphArray.Height = height;
            alphArray.MipLevels = genMipMaps ? 0 : 1;
            alphArray.ArraySize = (UINT)arrayCapacity;
            alphArray.Format = DXGI_FORMAT_R8_UNORM;
            alphArray.SampleDesc.Count = 1;
            alphArray.SampleDesc.Quality = 0;
            alphArray.Usage = D3D11_USAGE_DEFAULT;
            alphArray.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            alphArray.CPUAccessFlags = 0;
            alphArray.MiscFlags = 0;
            if (genMipMaps) alphArray.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
            VALIDATEDX11(device.CreateTexture2D(&alphArray, nullptr, &tb.texture));
            tb.texture->GetDesc(&tb.desc);
        }

        if (width > 0)
        {
            if (format != DXGI_FORMAT_R8_UNORM)
            {
                Throw(0, "DX11TextureArray::format is not A8 but image passed was A8");
            }

            UINT subresourceIndex = D3D11CalcSubresource(0, (UINT)index, tb.desc.MipLevels);
            D3D11_BOX box;
            box.left = 0;
            box.right = span.x;
            box.back = 1;
            box.front = 0;
            box.top = 0;
            box.bottom = span.y;

            UINT srcDepth = span.x * span.y * sizeof(uint8);
            if (tb.texture) activeDC->UpdateSubresource(tb.texture, subresourceIndex, &box, grayScalePixels, span.x * sizeof(uint8), srcDepth);
        }
    }

    static bool IsBlockCompressed(DXGI_FORMAT f)
    {
        switch (f)
        {
        case DXGI_FORMAT_BC1_TYPELESS: case DXGI_FORMAT_BC1_UNORM: case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_TYPELESS: case DXGI_FORMAT_BC2_UNORM: case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS: case DXGI_FORMAT_BC3_UNORM: case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS: case DXGI_FORMAT_BC4_UNORM: case DXGI_FORMAT_BC4_SNORM:
        case DXGI_FORMAT_BC5_TYPELESS: case DXGI_FORMAT_BC5_UNORM: case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS: case DXGI_FORMAT_BC6H_UF16: case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS: case DXGI_FORMAT_BC7_UNORM: case DXGI_FORMAT_BC7_UNORM_SRGB:
            return true;
        default:
            return false;
        }
    }

    // Number of mips in a full chain down to 1x1, which is what texconv -m 0 writes
    static UINT FullMipChainLength(UINT width, UINT height)
    {
        UINT levels = 1;
        UINT span = width > height ? width : height;
        while (span > 1)
        {
            span >>= 1;
            levels++;
        }
        return levels;
    }

    // Copies every mip of a loaded DDS texture into array slice [index]. The copy is GPU side (CopySubresourceRegion),
    // so block compressed data is never touched by the CPU and no row pitch arithmetic is required.
    // The array is created on first use with the format of the first DDS file and a full mip chain.
    // Every subsequent file must match that format and span, and supply at least that many mips.
    void WriteSubImage(size_t index, Vec2i span, IDDSTextureSupervisor* tx) override
    {
        if (!activeDC)
        {
            Throw(0, "%s: no active DC", __ROCOCO_FUNCTION__);
        }

        if (!tx)
        {
            Throw(0, "%s: null DDS texture", __ROCOCO_FUNCTION__);
        }

        if (width <= 0 || height <= 0)
        {
            Throw(0, "%s: cannot write into texture array, the array span was %d x %d", __ROCOCO_FUNCTION__, width, height);
        }

        if (span.x != width || span.y != height)
        {
            Throw(0, "%s: DDS span %d x %d does not match the texture array span %d x %d", __ROCOCO_FUNCTION__, span.x, span.y, width, height);
        }

        ID3D11Resource& src = tx->SysTexture().Resource();

        D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        src.GetType(&dimension);
        if (dimension != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        {
            Throw(0, "%s: DDS resource was not a 2D texture (dimension %d)", __ROCOCO_FUNCTION__, (int)dimension);
        }

        // GetType guarantees this is an ID3D11Texture2D, so no QueryInterface (and no AddRef/Release) is needed
        D3D11_TEXTURE2D_DESC srcDesc;
        static_cast<ID3D11Texture2D&>(src).GetDesc(&srcDesc);

        if ((int32)srcDesc.Width != width || (int32)srcDesc.Height != height)
        {
            Throw(0, "%s: DDS texture is %u x %u but the texture array is %d x %d", __ROCOCO_FUNCTION__, srcDesc.Width, srcDesc.Height, width, height);
        }

        if (srcDesc.ArraySize != 1 || (srcDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0)
        {
            Throw(0, "%s: DDS texture must be a single 2D image, not an array or cube map (ArraySize %u)", __ROCOCO_FUNCTION__, srcDesc.ArraySize);
        }

        if (srcDesc.SampleDesc.Count != 1)
        {
            Throw(0, "%s: DDS texture is multisampled", __ROCOCO_FUNCTION__);
        }

        if (tb.texture == nullptr)
        {
            if (IsBlockCompressed(srcDesc.Format) && ((width % 4) != 0 || (height % 4) != 0))
            {
                Throw(0, "%s: block compressed format %d requires a span that is a multiple of 4, but it was %d x %d", __ROCOCO_FUNCTION__, (int)srcDesc.Format, width, height);
            }

            arrayCapacity = count;
            format = srcDesc.Format;

            D3D11_TEXTURE2D_DESC ddsArray;
            ddsArray.Width = width;
            ddsArray.Height = height;
            ddsArray.MipLevels = FullMipChainLength(width, height);
            ddsArray.ArraySize = (UINT)arrayCapacity;
            ddsArray.Format = srcDesc.Format;
            ddsArray.SampleDesc.Count = 1;
            ddsArray.SampleDesc.Quality = 0;
            ddsArray.Usage = D3D11_USAGE_DEFAULT;
            ddsArray.BindFlags = D3D11_BIND_SHADER_RESOURCE; // BC formats cannot be render targets
            ddsArray.CPUAccessFlags = 0;
            ddsArray.MiscFlags = 0; // GENERATE_MIPS is illegal for BC formats, the mips come from the file
            VALIDATEDX11(device.CreateTexture2D(&ddsArray, nullptr, &tb.texture));
            tb.texture->GetDesc(&tb.desc);

            // Stops GenMipMaps from calling GenerateMips on a resource that lacks D3D11_RESOURCE_MISC_GENERATE_MIPS
            isMipMapped = true;
        }

        if (format != srcDesc.Format)
        {
            Throw(0, "%s: DDS format %d does not match the texture array format %d", __ROCOCO_FUNCTION__, (int)srcDesc.Format, (int)format);
        }

        if (index >= (size_t)tb.desc.ArraySize)
        {
            Throw(0, "%s: index %llu out of range. The texture array has %u elements", __ROCOCO_FUNCTION__, (unsigned long long)index, tb.desc.ArraySize);
        }

        if (srcDesc.MipLevels < tb.desc.MipLevels)
        {
            Throw(0, "%s: DDS texture has %u mip levels but the texture array needs %u. Re-cook it with a full mip chain (texconv -m 0)", __ROCOCO_FUNCTION__, srcDesc.MipLevels, tb.desc.MipLevels);
        }

        for (UINT mip = 0; mip < tb.desc.MipLevels; mip++)
        {
            UINT dstSubresource = D3D11CalcSubresource(mip, (UINT)index, tb.desc.MipLevels);
            UINT srcSubresource = D3D11CalcSubresource(mip, 0, srcDesc.MipLevels);
            activeDC->CopySubresourceRegion(tb.texture, dstSubresource, 0, 0, 0, &src, srcSubresource, nullptr);
        }
    }

    int32 MaxWidth() const override
    {
        return 4096;
    }

    size_t TextureCount() const override
    {
        return count;
    }

    int32 Width() const override
    {
        return width;
    }

    int32 Height() const override
    {
        return height;
    }

    void SetActiveDC(ID3D11DeviceContext* dc) override
    {
        activeDC = dc;
    }

    ID3D11ShaderResourceView* CreateSRV()
    {
        if (tb.texture == nullptr)
        {
            Throw(0, "No texture for CreateSRV");
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        ZeroMemory(&desc, sizeof(desc));

        desc.Format = tb.desc.Format;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.MostDetailedMip = 0;
        desc.Texture2DArray.MipLevels = (UINT)-1;   // all
        desc.Texture2DArray.FirstArraySlice = 0;
        desc.Texture2DArray.ArraySize = tb.desc.ArraySize;
      
        ID3D11ShaderResourceView* view = nullptr;
        VALIDATEDX11(device.CreateShaderResourceView(tb.texture, &desc, &view));

        if (view == nullptr)
        {
            Throw(0, "CreateSRV - CreateShaderResourceView yielded nullptr");
        }
        return view;
    }

    ID3D11ShaderResourceView* View() override
    {
        if (tb.shaderView == nullptr)
        {
            if (tb.texture != nullptr)
            {
                tb.shaderView = CreateSRV();
            }
        }

        return tb.shaderView;
    }
};

namespace Rococo::DX11
{
    IDX11BitmapArray* CreateDX11BitmapArray(ID3D11Device& device, ID3D11DeviceContext& dc, bool genMipMaps)
    {
        return new DX11BitmapArray(device, dc, genMipMaps);
    }

    IDX11BitmapArray* LoadAlphaBitmapArray(ID3D11Device& device, Vec2i span, int32 nElements, ITextureLoadEnumerator& enumerator, ID3D11DeviceContext& dc, bool genMipMaps)
    {
        IDX11BitmapArray* array = CreateDX11BitmapArray(device, dc, genMipMaps);

        try
        {
            array->ResetWidth(span.x, span.y);
            array->Resize(nElements);

            struct : IEventCallback<TextureLoadData>
            {
                int index = 0;
                IDX11BitmapArray* array;
                void OnEvent(TextureLoadData& data) override
                {
                    struct : Rococo::Imaging::IImageLoadEvents
                    {
                        cstr filename = nullptr;
                        IDX11BitmapArray* array = nullptr;
                        int32 index = 0;

                        void OnError(const char* message) override
                        {
                            Throw(0, "IRenderer.LoadAlphaTextureArray(...) - Error loading %s\n %s", filename, message);
                        }

                        void OnRGBAImage(const Vec2i&, const RGBAb*) override
                        {
                            Throw(0, "IRenderer.LoadAlphaTextureArray(...) - Tiff was RGBA.\n %s", filename);
                        }

                        void OnAlphaImage(const Vec2i& span, const uint8* data) override
                        {
                            if (array->Width() != span.x || array->Height() != span.y)
                            {
                                Throw(0, "IRenderer.LoadAlphaTextureArray(...) - Tiff was of incorrect span.\n %s", filename);
                            }
                            array->WriteSubImage(index, data, span);
                        }
                    } toArray;
                    toArray.filename = data.filename;
                    toArray.array = array;
                    toArray.index = index;

                    if (EndsWith(data.filename, ".tiff"))
                    {
                        Rococo::Imaging::DecompressTiff(toArray, data.pData, data.nBytes);
                        index++;
                    }
                }
            } insertImageIntoArray;

            insertImageIntoArray.array = array;

            enumerator.ForEachElement(insertImageIntoArray, true);

            return array;
        }
        catch (IException&)
        {
            array->Free();
            throw;
        }
    }
}