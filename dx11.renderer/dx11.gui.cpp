#include "rococo.dx11.api.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"

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

      ID3D11SamplerState* CreateSpriteSampler(ID3D11Device& device)
      {
         D3D11_SAMPLER_DESC spriteSamplerDesc;
         ZeroMemory(&spriteSamplerDesc, sizeof(spriteSamplerDesc));
         spriteSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
         spriteSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
         spriteSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
         spriteSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
         spriteSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
         spriteSamplerDesc.MinLOD = 0;
         spriteSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

         ID3D11SamplerState* spriteSampler = nullptr;
         VALIDATEDX11(device.CreateSamplerState(&spriteSamplerDesc, &spriteSampler));
         return spriteSampler;
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
         spriteRenderingDesc.ScissorEnable = FALSE;
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
   } // DX11
} // Rococo