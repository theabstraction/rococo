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

struct DX11Gui : IDX11Gui, IDX11FontRenderer
{
    std::vector<GuiVertex> guiVertices;

    ID_VERTEX_SHADER idGuiVS;
    ID_PIXEL_SHADER idGuiPS;

    ID_VERTEX_SHADER idHQVS;
    ID_PIXEL_SHADER idHQPS;

    AutoRelease<ID3D11Buffer> guiBuffer;

    AutoRelease<ID3D11DepthStencilState> guiDepthState;
    AutoRelease<ID3D11BlendState> alphaBlend;
    AutoRelease<ID3D11RasterizerState> spriteRasterizering;

    enum { GUI_BUFFER_VERTEX_CAPACITY = 3 * 512 };

    DX11Gui(IDX11ResourceLoader& loader, ID3D11Device& device)
    {
        static_assert(GUI_BUFFER_VERTEX_CAPACITY % 3 == 0, "Capacity must be divisible by 3");

        idGuiVS = loader.CreateVertexShader("!gui.vs", DX11::GetGuiVertexDesc(), DX11::NumberOfGuiVertexElements());
        idGuiPS = loader.CreatePixelShader("!gui.ps");
        idHQVS = loader.CreateVertexShader("!HQ-font.vs", DX11::GetGuiVertexDesc(), DX11::NumberOfGuiVertexElements());
        idHQPS = loader.CreatePixelShader("!HQ-font.ps");
        guiBuffer = DX11::CreateDynamicVertexBuffer<GuiVertex>(device, GUI_BUFFER_VERTEX_CAPACITY);
        alphaBlend = DX11::CreateAlphaBlend(device);
        spriteRasterizering = DX11::CreateSpriteRasterizer(device);
        guiDepthState = DX11::CreateGuiDepthStencilState(device);
    }

    void AddTriangle(const GuiVertex triangle[3]) override
    {
        guiVertices.push_back(triangle[0]);
        guiVertices.push_back(triangle[1]);
        guiVertices.push_back(triangle[2]);
    }

    bool ApplyGuiShaderTo(IShaderStateControl& shaders) override
    {
        return shaders.UseShaders(idGuiVS, idGuiPS);
    }

    bool ApplyHQFontsShaderTo(IShaderStateControl& shaders) override
    {
        return shaders.UseShaders(idHQVS, idHQPS);
    }

    bool ApplyGuiShaderTo(IShaderStateControl& shaders, ID_PIXEL_SHADER idGuiOverrideShader) override
    {
        return shaders.UseShaders(idGuiVS, idGuiOverrideShader);
    }

    void DrawCustomTexturedMesh(ID3D11DeviceContext& dc, IShaderStateControl& shaders, const GuiRect& absRect, ID_TEXTURE id, ID_PIXEL_SHADER idPixelShader, const GuiVertex* vertices, size_t nCount) override
    {
        if (nCount > GUI_BUFFER_VERTEX_CAPACITY) Throw(0, "%s - too many triangles. Max vertices: %d", __FUNCTION__, GUI_BUFFER_VERTEX_CAPACITY);

        FlushLayer(dc);

        shaders.UseShaders(idGuiVS, idPixelShader);

        shaders.SelectTexture(id);

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

    void FlushLayer(ID3D11DeviceContext& dc) override
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

    Vec2i lastSpan{ 0,0 };

    void RenderGui(IScene& scene, ID3D11DeviceContext& dc, IShaderStateControl& shaders, const GuiMetrics& metrics, IGuiRenderContext& grc) override
    {
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

        scene.RenderGui(grc);
    }
};

namespace Rococo
{
   namespace DX11
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

      IDX11Gui* CreateDX11Gui(IDX11ResourceLoader& loader, ID3D11Device& device)
      {
          return new DX11Gui(loader, device);
      }
   } // DX11
} // Rococo