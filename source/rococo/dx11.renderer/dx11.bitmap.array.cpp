#include "dx11.renderer.h"
#include <rococo.renderer.h>
#include "rococo.dx11.api.h"
#include "rococo.textures.h"
#include "dx11helpers.inl"
#include "rococo.imaging.h"

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

    ID3D11DeviceContext* activeDC = nullptr;

    size_t arrayCapacity{ 0 };
    size_t count{ 0 };

    DX11BitmapArray(ID3D11Device& _device, ID3D11DeviceContext& dc) :
        device(_device), activeDC(&dc)
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

    void Clear()
    {
        if (tb.texture) tb.texture->Release();
        if (tb.shaderView) tb.shaderView->Release();
        tb.texture = nullptr;
        tb.shaderView = nullptr;
        count = 0;
        arrayCapacity = 0;
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
            alphArray.MipLevels = 1;
            alphArray.ArraySize = (UINT)arrayCapacity;
            alphArray.Format = DXGI_FORMAT_R8_UNORM;
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
            if (format != DXGI_FORMAT_R8_UNORM)
            {
                Throw(0, "DX11TextureArray::format is not A8 but image passed was A8");
            }

            UINT subresourceIndex = D3D11CalcSubresource(0, (UINT)index, 1);
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

    ID3D11ShaderResourceView* View() override
    {
        if (tb.shaderView == nullptr)
        {
            if (tb.texture != nullptr)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc;
                ZeroMemory(&desc, sizeof(desc));

                desc.Texture2DArray.MipLevels = (UINT) -1;
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
};

namespace Rococo::DX11
{
    IDX11BitmapArray* CreateDX11BitmapArray(ID3D11Device& device, ID3D11DeviceContext& dc)
    {
        return new DX11BitmapArray(device, dc);
    }

    IDX11BitmapArray* LoadAlphaBitmapArray(ID3D11Device& device, Vec2i span, int32 nElements, ITextureLoadEnumerator& enumerator, ID3D11DeviceContext& dc)
    {
        IDX11BitmapArray* array = CreateDX11BitmapArray(device, dc);

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