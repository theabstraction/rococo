#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <vector>
#include "rococo.fonts.h"

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

using namespace Rococo::DX11;

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

struct DX11Gui : IDX11Gui, IDX11FontRenderer, Fonts::IGlyphRenderer, IGuiResources
{
    ID3D11Device& device;
    ID3D11DeviceContext& dc;

    std::vector<GuiVertex> guiVertices;

    ID_VERTEX_SHADER idGuiVS;
    ID_PIXEL_SHADER idGuiPS;

    ID_VERTEX_SHADER idHQVS;
    ID_PIXEL_SHADER idHQPS;

    AutoRelease<ID3D11Buffer> guiBuffer;
    AutoFree<Fonts::IFontSupervisor> fonts;
    AutoFree<IDX11TextureArray> spriteArray;
    AutoRelease<ID3D11DepthStencilState> guiDepthState;
    AutoRelease<ID3D11BlendState> alphaBlend;
    AutoRelease<ID3D11RasterizerState> spriteRasterizering;
    AutoFree<Textures::ITextureArrayBuilderSupervisor> spriteArrayBuilder;
    AutoFree<IDX11HQFontResource> hqFonts;

    AutoRelease<ID3D11Texture2D> fontTexture;
    AutoRelease<ID3D11ShaderResourceView> fontBinding;

    IDX11ResourceLoader& loader;
    IShaders& shaders;
    IRendererMetrics& metrics;
    IDX11TextureManager& textures;

    AutoFree<IOverlaySupervisor> overlays;
    AutoRelease<ID3D11Buffer> textureDescBuffer;

    struct Cursor
    {
        Textures::BitmapLocation sprite;
        Vec2i hotspotOffset;
    } cursor { Textures::BitmapLocation { {0,0,0,0}, -1 }, Vec2i { 0,0 } };

    enum { GUI_BUFFER_VERTEX_CAPACITY = 3 * 512 };

    DX11Gui(ID3D11Device& _device, ID3D11DeviceContext& _dc, IDX11TextureManager& _textures, IRendererMetrics& _metrics, IDX11ResourceLoader& _loader, IShaders& _shaders):
        device(_device), dc(_dc), metrics(_metrics), textures(_textures), shaders(_shaders), loader(_loader)
    {
        static_assert(GUI_BUFFER_VERTEX_CAPACITY % 3 == 0, "Capacity must be divisible by 3");

        textureDescBuffer = DX11::CreateConstantBuffer<TextureDescState>(device);

        idGuiVS = loader.DX11Shaders().CreateVertexShader("!gui.vs", DX11::GetGuiVertexDesc(), DX11::NumberOfGuiVertexElements());
        idGuiPS = loader.DX11Shaders().CreatePixelShader("!gui.ps");
        idHQVS = loader.DX11Shaders().CreateVertexShader("!HQ-font.vs", DX11::GetGuiVertexDesc(), DX11::NumberOfGuiVertexElements());
        idHQPS = loader.DX11Shaders().CreatePixelShader("!HQ-font.ps");
        guiBuffer = DX11::CreateDynamicVertexBuffer<GuiVertex>(device, GUI_BUFFER_VERTEX_CAPACITY);
        alphaBlend = DX11::CreateAlphaBlend(device);
        spriteRasterizering = DX11::CreateSpriteRasterizer(device);
        guiDepthState = DX11::CreateGuiDepthStencilState(device);

        spriteArray = CreateDX11TextureArray(device, dc);
        spriteArrayBuilder = CreateTextureArrayBuilder(loader, *spriteArray);

        cstr csvName = "!font1.csv";

        loader.LoadTextFile("!font1.csv", [csvName, this](const fstring& text)
            {
                fonts = Fonts::LoadFontCSV(csvName, text, text.length);
            }
        );

        overlays = CreateOverlays();

        hqFonts = CreateDX11HQFonts(FontRenderer(), device, dc);

        DX11::TextureBind fb = textures.Loader().LoadAlphaBitmap("!font1.tif");
        fontTexture = fb.texture;
        fontBinding = fb.shaderView;
    }

    IGuiResources& Gui() override
    {
        return *this;
    }

    GuiScale GetGuiScale() const
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

    void AssignShaderResourcesToDC()
    {
        dc.PSSetShaderResources(TXUNIT_FONT, 1, &fontBinding);
        auto* view = textures.GetCubeShaderResourceView();
        dc.PSSetShaderResources(TXUNIT_ENV_MAP, 1, &view);

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

    void RenderGui(IScene& scene, const GuiMetrics& metrics, bool renderOverlays) override
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

        if (renderOverlays)
        {
            overlays->Render(*this);
        }
    }

    IMaterials& Materials() override
    {
        return textures.Materials();
    };

    IRendererMetrics& Renderer() override
    {
        return metrics;
    }

    Textures::ITextureArrayBuilder& SpriteBuilder() override
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

    IDX11Gui* CreateDX11Gui(ID3D11Device& device, ID3D11DeviceContext& dc, IDX11TextureManager& textures, IRendererMetrics& metrics, IDX11ResourceLoader& loader, IShaders& shaders)
    {
        return new DX11Gui(device, dc, textures, metrics, loader, shaders);
    }
} // Rococo::DX11