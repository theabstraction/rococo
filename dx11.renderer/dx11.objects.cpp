#include "rococo.dx11.api.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"

namespace
{
   D3D11_INPUT_ELEMENT_DESC objectVertexDesc[] =
   {
      { "position",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "normal",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "color",		0, DXGI_FORMAT_R8G8B8A8_UNORM,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "color",		1, DXGI_FORMAT_R8G8B8A8_UNORM,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "texcoord",	0, DXGI_FORMAT_R32G32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
   };
}

namespace Rococo
{
   namespace DX11
   {
      const D3D11_INPUT_ELEMENT_DESC* const GetObjectVertexDesc()
      {
         static_assert(sizeof(ObjectVertex) == 40, "Gui vertex data was not 32 bytes wide");
         return objectVertexDesc;
      }

      const uint32 NumberOfObjectVertexElements()
      {
         static_assert(sizeof(objectVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC) == 5, "Vertex data was not 5 fields");
         return sizeof(objectVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);
      }

      ID3D11RasterizerState* CreateObjectRasterizer(ID3D11Device& device)
      {
         D3D11_RASTERIZER_DESC objRenderingDesc;
         objRenderingDesc.FillMode = D3D11_FILL_SOLID;
         objRenderingDesc.CullMode = D3D11_CULL_BACK;
         objRenderingDesc.FrontCounterClockwise = FALSE;
         objRenderingDesc.DepthBias = 0;
         objRenderingDesc.DepthBiasClamp = 0.0f;
         objRenderingDesc.SlopeScaledDepthBias = 0.0f;
         objRenderingDesc.DepthClipEnable = TRUE;
         objRenderingDesc.ScissorEnable = FALSE;
         objRenderingDesc.MultisampleEnable = FALSE;

         ID3D11RasterizerState* rs = nullptr;
         VALIDATEDX11(device.CreateRasterizerState(&objRenderingDesc, &rs));
         return rs;
      }

      ID3D11BlendState* CreateNoBlend(ID3D11Device& device)
      {
         D3D11_BLEND_DESC disableBlendDesc;
         ZeroMemory(&disableBlendDesc, sizeof(disableBlendDesc));
         disableBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

         ID3D11BlendState* noBlend = nullptr;
         VALIDATEDX11(device.CreateBlendState(&disableBlendDesc, &noBlend));
         return noBlend;
      }
   }
}