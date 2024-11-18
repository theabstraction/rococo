#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <vector>
#include "rococo.fonts.h"
#include <rococo.subsystems.h>
#include <rococo.strings.h>
#include <rococo.reflector.h>

#include <unordered_map>

#include <allocators/rococo.allocator.template.h>

namespace
{
   D3D11_INPUT_ELEMENT_DESC guiVertexDesc[] =
   {
      { "position",	0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "texcoord",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "texcoord",	1, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "color",	0, DXGI_FORMAT_R8G8B8A8_UNORM,      0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
   };
}

using namespace Rococo::Graphics;
using namespace Rococo::DX11;
using namespace Rococo::Reflection;

RGBAb FontColourToSysColour(Fonts::FontColour colour)
{
    RGBAb* pCol = (RGBAb*)&colour;
    return *pCol;
}

class SpanEvaluator : public Fonts::IGlyphRenderer
{
public:
    GuiRectf renderZone;

    SpanEvaluator() : renderZone(10000, 10000, -10000, -10000)
    {

    }

    void DrawGlyph(cr_vec2 t0, cr_vec2 t1, cr_vec2 p0, cr_vec2 p1, Fonts::FontColour colour) override
    {
        UNUSED(colour);
        UNUSED(t0);
        UNUSED(t1);
        ExpandZoneToContain(renderZone, p0);
        ExpandZoneToContain(renderZone, p1);
    }

    Vec2 Span() const
    {
        return Rococo::Span(renderZone);
    }
};

namespace Rococo::DX11
{
    cstr ToString(D3D11_STENCIL_OP op)
    {
        switch (op)
        {
        case D3D11_STENCIL_OP_KEEP:
            return "Keep";
        case D3D11_STENCIL_OP_ZERO:
            return "Zero";
        case D3D11_STENCIL_OP_REPLACE:
            return "Replace";
        case D3D11_STENCIL_OP_INCR_SAT:
            return "IncSat";
        case D3D11_STENCIL_OP_DECR_SAT:
            return "DecSat";
        case D3D11_STENCIL_OP_INVERT:
            return "Invert";
        case D3D11_STENCIL_OP_INCR:
            return "Inc";
        case D3D11_STENCIL_OP_DECR:
            return "Dec";
        default:
            return "Unknown";
        }
    }

    cstr ToString(D3D11_DEPTH_WRITE_MASK mask)
    {
        switch (mask)
        {
        case D3D11_DEPTH_WRITE_MASK_ZERO:
            return "Zero";
        case D3D11_DEPTH_WRITE_MASK_ALL:
            return "All";
        default:
            return "Unknown";
        }
    }

    void Reflect(IReflectionVisitor& v, cstr context, D3D11_DEPTH_STENCILOP_DESC op)
    {
        ReflectStackFormat(v, context, "--------");
        ReflectStackFormat(v, "StenciFail", ToString(op.StencilFailOp));
        ReflectStackFormat(v, "StencilDepthFailOp", ToString(op.StencilDepthFailOp));
        ReflectStackFormat(v, "StencilPassOp", ToString(op.StencilPassOp));
        ReflectStackFormat(v, "StencilFunc", ToString(op.StencilFunc));
    }

    void Reflect(IReflectionVisitor& v, cstr section, ID3D11DepthStencilState& state)
    {
        Section depthStencilStuff(section, v);

        D3D11_DEPTH_STENCIL_DESC desc;
        state.GetDesc(&desc);

        Reflect(v, "BackFace", desc.BackFace);
        ReflectStackFormat(v, "DepthEnable", "%s", desc.DepthEnable ? "True" : "False");
        ReflectStackFormat(v, "DepthFunc", ToString(desc.DepthFunc));
        ReflectStackFormat(v, "DepthWriteMask", ToString(desc.DepthWriteMask));
        Reflect(v, "FrontFace", desc.FrontFace);
        ReflectStackFormat(v, "StencilEnable", "%s", desc.StencilEnable ? "True" : "False");
        ReflectStackFormat(v, "StencilReadMask", "0x%X", desc.StencilReadMask);
        ReflectStackFormat(v, "StencilWriteMask", "0x%X", desc.StencilWriteMask);
    }

    using TFormatToName = std::unordered_map<DXGI_FORMAT, const char*, std::hash<DXGI_FORMAT>, std::equal_to<DXGI_FORMAT>, AllocatorWithMalloc<std::pair<const DXGI_FORMAT, cstr>>>;

    TFormatToName* formatToName = nullptr;

#define ADD_FORMAT(format) { (*formatToName)[format] = #format + sizeof("DXGI_FORMAT"); }

    void BuildFormatToName()
    {
        ADD_FORMAT(DXGI_FORMAT_UNKNOWN);
        ADD_FORMAT(DXGI_FORMAT_R32G32B32A32_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_R32G32B32A32_FLOAT);
        ADD_FORMAT(DXGI_FORMAT_R32G32B32A32_UINT);
        ADD_FORMAT(DXGI_FORMAT_R32G32B32A32_SINT);
        ADD_FORMAT(DXGI_FORMAT_R32G32B32_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_R32G32B32_FLOAT);
        ADD_FORMAT(DXGI_FORMAT_R32G32B32_UINT);
        ADD_FORMAT(DXGI_FORMAT_R32G32B32_SINT);
        ADD_FORMAT(DXGI_FORMAT_R16G16B16A16_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_R16G16B16A16_FLOAT);
        ADD_FORMAT(DXGI_FORMAT_R16G16B16A16_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R16G16B16A16_UINT);
        ADD_FORMAT(DXGI_FORMAT_R16G16B16A16_SNORM);
        ADD_FORMAT(DXGI_FORMAT_R16G16B16A16_SINT);
        ADD_FORMAT(DXGI_FORMAT_R32G32_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_R32G32_FLOAT);
        ADD_FORMAT(DXGI_FORMAT_R32G32_UINT);
        ADD_FORMAT(DXGI_FORMAT_R32G32_SINT);
        ADD_FORMAT(DXGI_FORMAT_R32G8X24_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
        ADD_FORMAT(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
        ADD_FORMAT(DXGI_FORMAT_R10G10B10A2_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_R10G10B10A2_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R10G10B10A2_UINT);
        ADD_FORMAT(DXGI_FORMAT_R11G11B10_FLOAT);
        ADD_FORMAT(DXGI_FORMAT_R8G8B8A8_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_R8G8B8A8_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        ADD_FORMAT(DXGI_FORMAT_R8G8B8A8_UINT);
        ADD_FORMAT(DXGI_FORMAT_R8G8B8A8_SNORM);
        ADD_FORMAT(DXGI_FORMAT_R8G8B8A8_SINT);
        ADD_FORMAT(DXGI_FORMAT_R16G16_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_R16G16_FLOAT);
        ADD_FORMAT(DXGI_FORMAT_R16G16_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R16G16_UINT);
        ADD_FORMAT(DXGI_FORMAT_R16G16_SNORM);
        ADD_FORMAT(DXGI_FORMAT_R16G16_SINT);
        ADD_FORMAT(DXGI_FORMAT_R32_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_D32_FLOAT);
        ADD_FORMAT(DXGI_FORMAT_R32_FLOAT);
        ADD_FORMAT(DXGI_FORMAT_R32_UINT);
        ADD_FORMAT(DXGI_FORMAT_R32_SINT);
        ADD_FORMAT(DXGI_FORMAT_R24G8_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_D24_UNORM_S8_UINT);
        ADD_FORMAT(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_X24_TYPELESS_G8_UINT);
        ADD_FORMAT(DXGI_FORMAT_R8G8_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_R8G8_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R8G8_UINT);
        ADD_FORMAT(DXGI_FORMAT_R8G8_SNORM);
        ADD_FORMAT(DXGI_FORMAT_R8G8_SINT);
        ADD_FORMAT(DXGI_FORMAT_R16_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_R16_FLOAT);
        ADD_FORMAT(DXGI_FORMAT_D16_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R16_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R16_UINT);
        ADD_FORMAT(DXGI_FORMAT_R16_SNORM);
        ADD_FORMAT(DXGI_FORMAT_R16_SINT);
        ADD_FORMAT(DXGI_FORMAT_R8_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_R8_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R8_UINT);
        ADD_FORMAT(DXGI_FORMAT_R8_SNORM);
        ADD_FORMAT(DXGI_FORMAT_R8_SINT);
        ADD_FORMAT(DXGI_FORMAT_A8_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R1_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
        ADD_FORMAT(DXGI_FORMAT_R8G8_B8G8_UNORM);
        ADD_FORMAT(DXGI_FORMAT_G8R8_G8B8_UNORM);
        ADD_FORMAT(DXGI_FORMAT_BC1_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_BC1_UNORM);
        ADD_FORMAT(DXGI_FORMAT_BC1_UNORM_SRGB);
        ADD_FORMAT(DXGI_FORMAT_BC2_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_BC2_UNORM);
        ADD_FORMAT(DXGI_FORMAT_BC2_UNORM_SRGB);
        ADD_FORMAT(DXGI_FORMAT_BC3_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_BC3_UNORM);
        ADD_FORMAT(DXGI_FORMAT_BC3_UNORM_SRGB);
        ADD_FORMAT(DXGI_FORMAT_BC4_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_BC4_UNORM);
        ADD_FORMAT(DXGI_FORMAT_BC4_SNORM);
        ADD_FORMAT(DXGI_FORMAT_BC5_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_BC5_UNORM);
        ADD_FORMAT(DXGI_FORMAT_BC5_SNORM);
        ADD_FORMAT(DXGI_FORMAT_B5G6R5_UNORM);
        ADD_FORMAT(DXGI_FORMAT_B5G5R5A1_UNORM);
        ADD_FORMAT(DXGI_FORMAT_B8G8R8A8_UNORM);
        ADD_FORMAT(DXGI_FORMAT_B8G8R8X8_UNORM);
        ADD_FORMAT(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM);
        ADD_FORMAT(DXGI_FORMAT_B8G8R8A8_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
        ADD_FORMAT(DXGI_FORMAT_B8G8R8X8_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);
        ADD_FORMAT(DXGI_FORMAT_BC6H_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_BC6H_UF16);
        ADD_FORMAT(DXGI_FORMAT_BC6H_SF16);
        ADD_FORMAT(DXGI_FORMAT_BC7_TYPELESS);
        ADD_FORMAT(DXGI_FORMAT_BC7_UNORM);
        ADD_FORMAT(DXGI_FORMAT_BC7_UNORM_SRGB);
        ADD_FORMAT(DXGI_FORMAT_AYUV);
        ADD_FORMAT(DXGI_FORMAT_Y410);
        ADD_FORMAT(DXGI_FORMAT_Y416);
        ADD_FORMAT(DXGI_FORMAT_NV12);
        ADD_FORMAT(DXGI_FORMAT_P010);
        ADD_FORMAT(DXGI_FORMAT_P016);
        ADD_FORMAT(DXGI_FORMAT_420_OPAQUE);
        ADD_FORMAT(DXGI_FORMAT_YUY2);
        ADD_FORMAT(DXGI_FORMAT_Y210);
        ADD_FORMAT(DXGI_FORMAT_Y216);
        ADD_FORMAT(DXGI_FORMAT_NV11);
        ADD_FORMAT(DXGI_FORMAT_AI44);
        ADD_FORMAT(DXGI_FORMAT_IA44);
        ADD_FORMAT(DXGI_FORMAT_P8);
        ADD_FORMAT(DXGI_FORMAT_A8P8);
        ADD_FORMAT(DXGI_FORMAT_B4G4R4A4_UNORM);
        ADD_FORMAT(DXGI_FORMAT_P208);
        ADD_FORMAT(DXGI_FORMAT_V208);
        ADD_FORMAT(DXGI_FORMAT_V408);
        ADD_FORMAT(DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE);
        ADD_FORMAT(DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE);
    }

    void ClearFormatToName()
    {
        delete formatToName;
        formatToName = nullptr;
    }

    cstr ToString(DXGI_FORMAT format)
    {
        if (formatToName == nullptr)
        {
            formatToName = new TFormatToName();
            Memory::GetDX11Allocator().AtRelease(ClearFormatToName);
            BuildFormatToName();
        }

        auto i = formatToName->find(format);

        if (i == formatToName->end())
        {
            return i->second;
        }

        return "Unknown";
    }

    cstr ToString(D3D11_USAGE usage)
    {
        switch (usage)
        {
        case D3D11_USAGE_DEFAULT:
            return "default";
        case D3D11_USAGE_IMMUTABLE:
            return "immutable";
        case D3D11_USAGE_DYNAMIC:
            return "dynamic";
        case D3D11_USAGE_STAGING:
            return "staging";
        default:
            return "unknown";
        }
    }

    void Reflect(IReflectionVisitor& v, cstr section, ID3D11Texture2D& texture)
    {
        Section texturing(section, v);

        D3D11_TEXTURE2D_DESC desc;
        texture.GetDesc(&desc);

        ReflectStackFormat(v, "Span", "%dx%d", desc.Width, desc.Height);
        ReflectStackFormat(v, "ArraySize", "%u", desc.ArraySize);
        ReflectStackFormat(v, "Mip Levels", "%u", desc.MipLevels);
        ReflectStackFormat(v, "Format", ToString(desc.Format));
        ReflectStackFormat(v, "Usage", ToString(desc.Usage));
        ReflectStackFormat(v, "SampleDesc", "Count: %u Quality: %u", desc.SampleDesc.Count, desc.SampleDesc.Quality);
        ReflectStackFormat(v, "BindFlags", "%u", desc.BindFlags);
        ReflectStackFormat(v, "MiscFlag", "%u", desc.MiscFlags);
        ReflectStackFormat(v, "CPUAccessFlags", "%u", desc.CPUAccessFlags);
    }

    cstr ToString(D3D11_BLEND blend)
    {
        switch (blend)
        {
        case D3D11_BLEND_ZERO:
            return "Zero";
        case D3D11_BLEND_ONE:
            return "One";
        case D3D11_BLEND_SRC_COLOR:
            return "SrcColor";
        case D3D11_BLEND_INV_SRC_COLOR:
            return "InvSrcColor";
        case D3D11_BLEND_SRC_ALPHA:
            return "SrcAlpha";
        case D3D11_BLEND_INV_SRC_ALPHA:
            return "InvSrcAlpha";
        case D3D11_BLEND_DEST_ALPHA:
            return "DestAlpha";
        case D3D11_BLEND_INV_DEST_ALPHA:
            return "InvDestAlpha";
        case D3D11_BLEND_DEST_COLOR:
            return "DestColor";
        case D3D11_BLEND_INV_DEST_COLOR:
            return "InvDestColor";
        case D3D11_BLEND_SRC_ALPHA_SAT:
            return "SrcAlphaSat";
        case D3D11_BLEND_BLEND_FACTOR:
            return "BlendFactor";
        case D3D11_BLEND_INV_BLEND_FACTOR:
            return "InvBlendFactor";
        case D3D11_BLEND_SRC1_COLOR:
            return "Src1Color";
        case D3D11_BLEND_INV_SRC1_COLOR:
            return "InvSrc1Color";
        case D3D11_BLEND_SRC1_ALPHA:
            return "Src1Alpha";
        case D3D11_BLEND_INV_SRC1_ALPHA:
            return "InvSrc1Alpha";
        default:
            return "Unknown";
        }
    }

    cstr ToString(D3D11_BLEND_OP op)
    {
        switch (op)
        {
        case D3D11_BLEND_OP_ADD:
            return "Add";
        case D3D11_BLEND_OP_SUBTRACT:
            return "Subtract";
        case D3D11_BLEND_OP_MIN:
            return "Min";
        case D3D11_BLEND_OP_MAX:
            return "Max";
        default:
            return "Unknown";
        }
    }

    cstr ToString(D3D11_CULL_MODE mode)
    {
        switch (mode)
        {
        case D3D11_CULL_NONE:
            return "None";
        case D3D11_CULL_FRONT:
            return "Front";
        case D3D11_CULL_BACK:
            return "Back";
        default:
            return "Unknown";
        }
    }

    void Reflect(IReflectionVisitor& v, const D3D11_RENDER_TARGET_BLEND_DESC& target)
    {
        ReflectStackFormat(v, "BlendEnable", "%s", target.BlendEnable ? "True" : "False");
        ReflectStackFormat(v, "BlendOp", ToString(target.BlendOp));
        ReflectStackFormat(v, "BlendOpAlpha", ToString(target.BlendOpAlpha));
        ReflectStackFormat(v, "DestBlend", ToString(target.DestBlend));
        ReflectStackFormat(v, "DestBlendAlpha", ToString(target.DestBlendAlpha));
        ReflectStackFormat(v, "RenderTargetWriteMask", "0x%X", target.RenderTargetWriteMask);
        ReflectStackFormat(v, "SrcBlend", ToString(target.SrcBlend));
        ReflectStackFormat(v, "SrcBlendAlpha", ToString(target.SrcBlendAlpha));
    }

    cstr ToString(D3D11_FILL_MODE mode)
    {
        switch (mode)
        {
        case D3D11_FILL_WIREFRAME:
            return "Wireframe";
        case D3D11_FILL_SOLID:
            return "Solid";
        default:
            return "Unknown";
        }
    }

    void Reflect(IReflectionVisitor& v, cstr section, ID3D11BlendState& state)
    {
        Section blending(section, v);

        D3D11_BLEND_DESC desc;
        state.GetDesc(&desc);

        ReflectStackFormat(v, "AlphaToCoverageEnable", "%s", desc.AlphaToCoverageEnable ? "True" : "False");
        ReflectStackFormat(v, "IndependentBlendEnable", "%s", desc.IndependentBlendEnable ? "True" : "False");

        v.EnterContainer("Render Targets");

        for (int i = 0; i < 8; i++)
        {
            EnterElement(v, "Element %d", i);
            auto& target = desc.RenderTarget[i];
            Reflect(v, target);
            v.LeaveElement();
        }

        v.LeaveContainer();
    }

    void Reflect(IReflectionVisitor& v, cstr section, ID3D11RasterizerState& state)
    {
        Section rasterizer(section, v);

        D3D11_RASTERIZER_DESC desc;
        state.GetDesc(&desc);

        ReflectStackFormat(v, "AntialiasedLineEnable", "%s", desc.AntialiasedLineEnable ? "True" : "False");
        ReflectStackFormat(v, "CullMode",                   ToString(desc.CullMode));
        ReflectStackFormat(v, "DepthBias", "%d",            desc.DepthBias);
        ReflectStackFormat(v, "DepthBiasClamp", "%f",       desc.DepthBiasClamp);
        ReflectStackFormat(v, "DepthClipEnable",            desc.DepthClipEnable ? "True" : "False");
        ReflectStackFormat(v, "FillMode",                   ToString(desc.FillMode));
        ReflectStackFormat(v, "FrontCounterClockwise",      desc.FrontCounterClockwise ? "True" : "False");
        ReflectStackFormat(v, "MultisampleEnable",          desc.MultisampleEnable ? "True" : "False");
        ReflectStackFormat(v, "ScissorEnable",              desc.ScissorEnable ? "True" : "False");
        ReflectStackFormat(v, "SlopeScaledDepthBias", "%f", desc.SlopeScaledDepthBias);
    }
}

struct DX11Gui : IDX11Gui, IDX11FontRenderer, Fonts::IGlyphRenderer, IGuiResources, ISubsystem, IReflectionTarget
{
    ID3D11Device& device;
    ID3D11DeviceContext& dc;

    std::vector<GuiVertex> guiVertices;

    ID_VERTEX_SHADER idGuiVS;
    ID_PIXEL_SHADER idGuiPS;
    ID_VERTEX_SHADER idGuiRawVS;

    ID_VERTEX_SHADER idHQVS;
    ID_PIXEL_SHADER idHQPS;

    ID_PIXEL_SHADER idSelectedTexturePS;

    AutoRelease<ID3D11Buffer> guiBuffer;
    AutoFree<Fonts::IFontSupervisor> fonts;
    AutoFree<IDX11BitmapArray> spriteArray;
    AutoRelease<ID3D11DepthStencilState> guiDepthState;
    AutoRelease<ID3D11BlendState> alphaBlend;
    AutoRelease<ID3D11RasterizerState> spriteRasterizering;
    AutoFree<Textures::IBitmapArrayBuilderSupervisor> spriteArrayBuilder;
    AutoFree<IDX11HQFontResource> hqFonts;

    AutoRelease<ID3D11Texture2D> fontTexture;
    AutoRelease<ID3D11ShaderResourceView> fontBinding;

    IDX11ResourceLoader& loader;
    IShaders& shaders;
    IRendererMetrics& metrics;
    IDX11TextureManager& textures;

    AutoRelease<ID3D11Buffer> textureDescBuffer;

    IRenderingResources& resources;

    struct Cursor
    {
        Textures::BitmapLocation sprite;
        Vec2i hotspotOffset;
    } cursor { Textures::BitmapLocation { {0,0,0,0}, -1 }, Vec2i { 0,0 } };

    enum { GUI_BUFFER_VERTEX_CAPACITY = 3 * 512 };

    DX11Gui(DX11::RenderBundle& bundle):
        device(bundle.device), dc(bundle.dc), metrics(bundle.metrics), textures(bundle.textures), shaders(bundle.shaders), loader(bundle.loader), resources(bundle.resources)
    {
        static_assert(GUI_BUFFER_VERTEX_CAPACITY % 3 == 0, "Capacity must be divisible by 3");

        textureDescBuffer = DX11::CreateConstantBuffer<TextureDescState>(device);

        idGuiVS = loader.DX11Shaders().CreateVertexShader("!shaders/compiled/gui.vs", DX11::GetGuiVertexDesc(), DX11::NumberOfGuiVertexElements());
        idGuiRawVS = loader.DX11Shaders().CreateVertexShader("!shaders/compiled/gui.raw.vs", DX11::GetGuiVertexDesc(), DX11::NumberOfGuiVertexElements());
        idGuiPS = loader.DX11Shaders().CreatePixelShader("!shaders/compiled/gui.ps");
        idHQVS = loader.DX11Shaders().CreateVertexShader("!shaders/compiled/HQ-font.vs", DX11::GetGuiVertexDesc(), DX11::NumberOfGuiVertexElements());
        idHQPS = loader.DX11Shaders().CreatePixelShader("!shaders/compiled/HQ-font.ps");
        idSelectedTexturePS = loader.DX11Shaders().CreatePixelShader("!shaders/compiled/gui.texture.select.ps");

        guiBuffer = DX11::CreateDynamicVertexBuffer<GuiVertex>(device, GUI_BUFFER_VERTEX_CAPACITY);
        alphaBlend = DX11::CreateAlphaBlend(device);
        spriteRasterizering = DX11::CreateSpriteRasterizer(device);
        guiDepthState = DX11::CreateGuiDepthStencilState(device);

        spriteArray = CreateDX11BitmapArray(device, dc);
        spriteArrayBuilder = CreateBitmapArrayBuilder(loader, *spriteArray);

        cstr csvName = "!font1.csv";

        loader.LoadTextFile(csvName, [csvName, this](const fstring& text)
            {
                fonts = Fonts::LoadFontCSV(csvName, text, text.length);
            }
        );

        hqFonts = CreateDX11HQFonts(FontRenderer(), device, dc);

        DX11::TextureBind fb = textures.Loader().LoadAlphaBitmap("!font1.tif");
        fontTexture = fb.texture;
        fontBinding = fb.shaderView;
    }

    IGuiResources& Resources() override
    {
        return *this;
    }

    ITextureManager& Textures() override
    {
        return textures;
    }

    GuiScale GetGuiScale() const override
    {
        float fontWidth = clamp(1.0f, fonts->TextureSpan().x, 1000000.0f);
        float spriteWidth = clamp(1.0f, (float) spriteArray->Width(), 1000000.0f);
        return GuiScale{ 1.0f / lastSpan.x, 1.0f / lastSpan.y, 1.0f / fontWidth, 1.0f / spriteWidth };
    }

    Fonts::IFont& FontMetrics() override
    {
        return *fonts;
    }

    void AddTriangle(const GuiVertex triangle[3]) override
    {
        guiVertices.push_back(triangle[0]);
        guiVertices.push_back(triangle[1]);
        guiVertices.push_back(triangle[2]);
    }

    bool ApplyGuiShader() override
    {
        return shaders.UseShaders(idGuiVS, idGuiPS);
    }

    bool ApplyHQFontsShader() override
    {
        return shaders.UseShaders(idHQVS, idHQPS);
    }

    bool ApplyGuiShader(ID_PIXEL_SHADER idGuiOverrideShader) override
    {
        return shaders.UseShaders(idGuiVS, idGuiOverrideShader);
    }

    bool ApplyGuiShader(ID_VERTEX_SHADER idVertexShader, ID_PIXEL_SHADER idGuiOverrideShader)
    {
        return shaders.UseShaders(idVertexShader, idGuiOverrideShader);
    }

    void AssignShaderResourcesToDC()
    {
        dc.PSSetShaderResources(TXUNIT_FONT, 1, &fontBinding);

        auto* envMap = textures.DX11CubeTextures().GetShaderView(resources.GetEnvMapId());
        dc.PSSetShaderResources(TXUNIT_ENV_MAP, 1, &envMap);

        ID3D11ShaderResourceView* materialViews[1] = { textures.Materials().Textures().View() };
        dc.PSSetShaderResources(TXUNIT_MATERIALS, 1, materialViews);

        ID3D11ShaderResourceView* spriteviews[1] = { SpriteView() };
        dc.PSSetShaderResources(TXUNIT_SPRITES, 1, spriteviews);
    }

    void DrawCustomTexturedMesh(const GuiRect& absRect, ID_TEXTURE id, cstr shaderName, const GuiVertex* vertices, size_t nCount) override
    {
        UNUSED(absRect);

        if (nCount > GUI_BUFFER_VERTEX_CAPACITY) Throw(0, "%s - too many triangles. Max vertices: %d", __FUNCTION__, GUI_BUFFER_VERTEX_CAPACITY);

        FlushLayer();

        ID_PIXEL_SHADER idPixelShader = loader.DX11Shaders().CreatePixelShader(shaderName);

        shaders.UseShaders(idGuiVS, idPixelShader);

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

        shaders.UseShaders(idGuiVS, idGuiPS);
    }

    void DrawGlyph(cr_vec2 uvTopLeft, cr_vec2 uvBottomRight, cr_vec2 posTopLeft, cr_vec2 posBottomRight, Fonts::FontColour fcolour) override
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

        GuiTriangle a
        {
             GuiVertex{ {x0, y0}, {{ u0, v0}, 1 }, drawFont, (RGBAb)colour }, // topLeft
             GuiVertex{ {x0, y1}, {{ u0, v1}, 1 }, drawFont, (RGBAb)colour }, // bottomLeft
             GuiVertex{ {x1, y1}, {{ u1, v1}, 1 }, drawFont, (RGBAb)colour } // topRight
        };

        GuiTriangle b
        {
             GuiVertex{ {x0, y0}, {{ u0, v0}, 1 }, drawFont, (RGBAb)colour }, // topLeft
             GuiVertex{ {x1, y0}, {{ u1, v0}, 1 }, drawFont, (RGBAb)colour }, // TopRight
             GuiVertex{ {x1, y1}, {{ u1, v1}, 1 }, drawFont, (RGBAb)colour } // bottomRight
        };

        AddTriangle(&a.a);
        AddTriangle(&b.a);
    }

    void FlushLayer() override
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

    IDX11FontRenderer& FontRenderer()
    {
        return *this;
    }

    virtual ~DX11Gui()
    {

    }

    void Free() override
    {
        delete this;
    }

    Vec2i lastSpan{ 1,1 };

    void RenderGui(IScene& scene, const GuiMetrics& metrics) override
    {
        if (metrics.screenSpan.x == 0 || metrics.screenSpan.y == 0) return;

        dc.RSSetState(spriteRasterizering);

        D3D11_RECT rect = { 0, 0, metrics.screenSpan.x, metrics.screenSpan.y };
        dc.RSSetScissorRects(1, &rect);

        FLOAT blendFactorUnused[] = { 0,0,0,0 };
        dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);
        dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        dc.OMSetDepthStencilState(guiDepthState, 0);

        if (!shaders.UseShaders(idGuiVS, idGuiPS))
        {
            Throw(0, "Error setting Gui shaders");
        }

        if (lastSpan != metrics.screenSpan)
        {
            lastSpan = metrics.screenSpan;
            scene.OnGuiResize(lastSpan);
        }

        scene.RenderGui(*this);

        FlushLayer();
        DrawCursor(metrics);
        FlushLayer();
    }

    IMaterials& Materials() override
    {
        return textures.Materials();
    };

    IRendererMetrics& Renderer() override
    {
        return metrics;
    }

    void SetNormalBitmapRendering() override
    {
        dc.PSSetShaderResources(TXUNIT_SELECT, 0, nullptr);
        ApplyGuiShader();
    }

    void SetVolatileBitmapRendering(ID_VOLATILE_BITMAP bitmapId) override
    {
        auto* tx2D = textures.GetVolatileBitmap(bitmapId);
        if (tx2D)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            ZeroMemory(&desc, sizeof(desc));

            desc.Texture2D.MipLevels = 1;          
            desc.Texture2D.MostDetailedMip = 0;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

            AutoRelease<ID3D11ShaderResourceView> view;
            VALIDATEDX11(device.CreateShaderResourceView(tx2D, &desc, &view));
            dc.PSSetShaderResources(TXUNIT_SELECT, 1, &view);
        }

        ApplyGuiShader(idGuiRawVS, idSelectedTexturePS);
    }

    Textures::IBitmapArrayBuilder& SpriteBuilder() override
    {
        return *spriteArrayBuilder;
    }

    ID3D11ShaderResourceView* SpriteView() override
    {
        return spriteArray->View();
    }

    void SetGuiShader(cstr pixelShaderName)
    {
        FlushLayer();

        if (pixelShaderName == nullptr || pixelShaderName[0] == 0)
        {
            ApplyGuiShader();
            return;
        }

        auto id = loader.DX11Shaders().CreatePixelShader(pixelShaderName);
        if (id)
        {
            ApplyGuiShader(id);
        }
    }

    Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect) override
    {
        char stackBuffer[128];
        SpanEvaluator spanEvaluator;
        Fonts::IGlyphRenderPipeline* pipeline = Fonts::CreateGlyphRenderPipeline(stackBuffer, sizeof(stackBuffer), spanEvaluator);

        GuiRectf qrect(-10000.0f, -10000.0f, 10000.0f, 10000.0f);

        if (clipRect != nullptr)
        {
            qrect = Dequantize(*clipRect);
        }
        RouteDrawTextBasic(pos, job, *fonts, *pipeline, qrect);

        Vec2i span = Quantize(spanEvaluator.Span());
        if (span.x < 0) span.x = 0;
        if (span.y < 0) span.y = 0;
        return span;
    }

    void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect)
    {
        char stackBuffer[128];
        Fonts::IGlyphRenderPipeline* pipeline = Fonts::CreateGlyphRenderPipeline(stackBuffer, sizeof(stackBuffer), *this);

        GuiRectf qrect(-10000.0f, -10000.0f, 10000.0f, 10000.0f);

        if (clipRect != nullptr)
        {
            qrect = Dequantize(*clipRect);
        }
        RouteDrawTextBasic(pos, job, *fonts, *pipeline, qrect);
    }

    void SetCursorBitmap(const Textures::BitmapLocation& sprite, Vec2i hotspotOffset) override
    {
        cursor.sprite = sprite;
        cursor.hotspotOffset = hotspotOffset;
    }

    void ShowVenue(IMathsVisitor& visitor) override
    {
        visitor.ShowString("Cursor bitmap", "Id: %d. {%d,%d}-{%d,%d}", cursor.sprite.textureIndex, cursor.sprite.txUV.left, cursor.sprite.txUV.top, cursor.sprite.txUV.right, cursor.sprite.txUV.bottom);
        visitor.ShowString("Cursor hotspot delta", "(%+d %+d)", cursor.hotspotOffset.x, cursor.hotspotOffset.y);
    }

    EWindowCursor cursorId = EWindowCursor_Default;

    void SetSysCursor(EWindowCursor id) override
    {
        cursorId = id;
    }

    void DrawCursor(const GuiMetrics& metrics) override
    {
        if (cursor.sprite.textureIndex >= 0)
        {
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

            SpriteVertexData drawSprite{ 0, (float)cursor.sprite.textureIndex, 0, 0 };

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
            cstr sysId;

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

            HCURSOR hCursor = LoadCursorA(nullptr, sysId);
            SetCursor(hCursor);
        }
    }

    void SetScissorRect(const Rococo::GuiRect& rect) override
    {
        D3D11_RECT d11Rect;
        d11Rect.left = rect.left;
        d11Rect.top = rect.top;
        d11Rect.right = rect.right;
        d11Rect.bottom = rect.bottom;
        dc.RSSetScissorRects(1, &d11Rect);
    }

    void ClearScissorRect() override
    {
        D3D11_RECT rect = { 0, 0, lastSpan.x, lastSpan.y };
        dc.RSSetScissorRects(1, &rect);
    }

    const Fonts::ArrayFontMetrics& GetFontMetrics(ID_FONT idFont) override
    {
        return hqFonts->GetFontMetrics(idFont);
    }

    void RenderHQText(ID_FONT id, Fonts::IHQTextJob& job, IGuiRenderContext::EMode mode, const GuiRect& clipRect) override
    {
        return hqFonts->RenderHQText(id, job, mode, dc, clipRect);
    }

    IHQFontResource& HQFontsResources() override
    {
        return *hqFonts;
    }

    ID_TEXTURE lastTextureId;

    Vec2i SelectTexture(ID_TEXTURE id)
    {
        TextureBind t = textures.GetTexture(id);

        D3D11_TEXTURE2D_DESC desc;
        t.texture->GetDesc(&desc);

        D3D11_SHADER_RESOURCE_VIEW_DESC vdesc;
        t.shaderView->GetDesc(&vdesc);

        if (id != lastTextureId)
        {
            FlushLayer();
            lastTextureId = id;
            dc.PSSetShaderResources(4, 1, &t.shaderView);

            float dx = (float)desc.Width;
            float dy = (float)desc.Height;

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

    void RegisterSubsystem(ISubsystemMonitor& monitor, ID_SUBSYSTEM parentId) override
    {
        auto guiId = monitor.Register(*this, parentId);
    }

    [[nodiscard]] cstr SubsystemName() const override
    {
        return "DX11Gui";
    }

    [[nodiscard]] Reflection::IReflectionTarget* ReflectionTarget() override
    {
        return this;
    }

    void Visit(IReflectionVisitor& v) override
    {
        ReflectStackFormat(v, "GuiVertexCapacity", "%llu", guiVertices.size());

        v.EnterContainer("Fonts");
        for (int i = 0; i < fonts->NumberOfGlyphSets(); i++)
        {
            EnterElement(v, "GlyphSet %d", i);
            const auto& glyphSet = fonts->operator[](i);
            ReflectStackFormat(v, "Name", "%s", glyphSet.Name());
            ReflectStackFormat(v, "Ascent", "%d", glyphSet.FontAscent());
            ReflectStackFormat(v, "Height", "%f", glyphSet.FontHeight());
            v.LeaveElement();
        }
        v.LeaveContainer();

        Reflect(v, "fontTexture", *fontTexture);
        ReflectStackFormat(v, "SpriteArray", "%d textures of width %dx%d", spriteArray->TextureCount(), spriteArray->Width(), spriteArray->Width());

        Reflect(v, "guiDepthState", *guiDepthState);
        
        Reflect(v, "alphaBlend", *alphaBlend);
        Reflect(v, "spriteRasterizer", *spriteRasterizering);
    }
}; // DX11Gui

namespace Rococo::DX11
{
    const D3D11_INPUT_ELEMENT_DESC* const GetGuiVertexDesc()
    {
        static_assert(sizeof(GuiVertex) == 40, "Gui vertex data was not 40 bytes wide");
        return guiVertexDesc;
    }

    const uint32 NumberOfGuiVertexElements()
    {
        static_assert(sizeof(guiVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC) == 4, "Vertex data was not 4 fields");
        return sizeof(guiVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);
    }
      
    ID3D11RasterizerState* CreateSpriteRasterizer(ID3D11Device& device)
    {
        D3D11_RASTERIZER_DESC spriteRenderingDesc;
        spriteRenderingDesc.FillMode = D3D11_FILL_SOLID;
        spriteRenderingDesc.CullMode = D3D11_CULL_NONE;
        spriteRenderingDesc.FrontCounterClockwise = TRUE;
        spriteRenderingDesc.DepthBias = 0;
        spriteRenderingDesc.DepthBiasClamp = 0.0f;
        spriteRenderingDesc.SlopeScaledDepthBias = 0.0f;
        spriteRenderingDesc.DepthClipEnable = FALSE;
        spriteRenderingDesc.ScissorEnable = TRUE;
        spriteRenderingDesc.MultisampleEnable = FALSE;

        ID3D11RasterizerState* sr = nullptr;
        VALIDATEDX11(device.CreateRasterizerState(&spriteRenderingDesc, &sr));
        return sr;
    }

    ID3D11BlendState* CreateAlphaBlend(ID3D11Device& device)
    {
        D3D11_BLEND_DESC alphaBlendDesc;
        ZeroMemory(&alphaBlendDesc, sizeof(alphaBlendDesc));

        auto& blender = alphaBlendDesc.RenderTarget[0];

        blender.SrcBlendAlpha = D3D11_BLEND_ZERO;
        blender.SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blender.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blender.DestBlendAlpha = D3D11_BLEND_ZERO;
        blender.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blender.BlendEnable = TRUE;
        blender.BlendOp = D3D11_BLEND_OP_ADD;
        blender.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        ID3D11BlendState* alphaBlend = nullptr;
        VALIDATEDX11(device.CreateBlendState(&alphaBlendDesc, &alphaBlend));
        return alphaBlend;
    }

    IDX11Gui* CreateDX11Gui(RenderBundle& bundle)
    {
        return new DX11Gui(bundle);
    }
} // Rococo::DX11
