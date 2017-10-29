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

      ID3D11SamplerState* CreateObjectSampler(ID3D11Device& device)
      {
         D3D11_SAMPLER_DESC spriteSamplerDesc;
         ZeroMemory(&spriteSamplerDesc, sizeof(spriteSamplerDesc));
         spriteSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
         spriteSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
         spriteSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
         spriteSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
         spriteSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
         spriteSamplerDesc.MinLOD = 0;
         spriteSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

         ID3D11SamplerState* spriteSampler = nullptr;
         VALIDATEDX11(device.CreateSamplerState(&spriteSamplerDesc, &spriteSampler));
         return spriteSampler;
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

	  ID3D11BlendState* CreateAdditiveBlend(ID3D11Device& device)
	  {
		  D3D11_BLEND_DESC blendDesc;
		  ZeroMemory(&blendDesc, sizeof(blendDesc));
		  blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		  blendDesc.RenderTarget[0].BlendEnable = TRUE;
		  blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		  blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		  blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_COLOR;
		  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		  blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		  blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;

		  ID3D11BlendState* d11Blend = nullptr;
		  VALIDATEDX11(device.CreateBlendState(&blendDesc, &d11Blend));
		  return d11Blend;
	  }
   }
}