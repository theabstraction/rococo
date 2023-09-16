#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"

namespace ANON
{
   D3D11_INPUT_ELEMENT_DESC objectVertexDesc[] =
   {
      { "position",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "normal",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "texcoord",	0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "color",	0, DXGI_FORMAT_R8G8B8A8_UNORM,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "texcoord",	1, DXGI_FORMAT_R32G32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
   };

   D3D11_INPUT_ELEMENT_DESC skyVertexDesc[] = 
   {
		{ "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
   };

   D3D11_INPUT_ELEMENT_DESC skinnedObjectVertexDesc[] =
   {
	  { "position",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "normal",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "texcoord",	0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "color",	0, DXGI_FORMAT_R8G8B8A8_UNORM,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "texcoord",	1, DXGI_FORMAT_R32G32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "blendindices",	0, DXGI_FORMAT_R32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "blendweight",	0, DXGI_FORMAT_R32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "blendindices",	1, DXGI_FORMAT_R32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "blendweight",	1, DXGI_FORMAT_R32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
   };
}

namespace Rococo
{
   namespace DX11
   {
	   const D3D11_INPUT_ELEMENT_DESC* const GetSkinnedObjectVertexDesc()
	   {
		   return ANON::skinnedObjectVertexDesc;
	   }

	   const uint32 NumberOfSkinnedObjectVertexElements()
	   {
		   static_assert(sizeof(ANON::skinnedObjectVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC) == 9, "Vertex data was not 9 fields");
		   return sizeof(ANON::skinnedObjectVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);
	   }

      const D3D11_INPUT_ELEMENT_DESC* const GetObjectVertexDesc()
      {
         static_assert(sizeof(ObjectVertex) == 44, "Gui vertex data was not 44 bytes wide");
         return ANON::objectVertexDesc;
      }

      const uint32 NumberOfObjectVertexElements()
      {
         static_assert(sizeof(ANON::objectVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC) == 5, "Vertex data was not 5 fields");
         return sizeof(ANON::objectVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);
      }

	  const D3D11_INPUT_ELEMENT_DESC* const GetSkyVertexDesc()
	  {
		  static_assert(sizeof(SkyVertex) == 12, "Sky vertex data was not 16 bytes wide");
		  return ANON::skyVertexDesc;
	  }

	  const uint32 NumberOfSkyVertexElements()
	  {
		  static_assert(sizeof(ANON::skyVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC) == 1, "Sky Vertex data was not 1 field");
		  return sizeof(ANON::skyVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);
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

	  ID3D11RasterizerState* CreateShadowRasterizer(ID3D11Device& device)
	  {
		  D3D11_RASTERIZER_DESC rd;
		  rd.FillMode = D3D11_FILL_SOLID;
		  rd.CullMode = D3D11_CULL_FRONT;
		  rd.FrontCounterClockwise = FALSE;
		  rd.DepthBias = 0;
		  rd.DepthBiasClamp = 0.0f;
		  rd.SlopeScaledDepthBias = 0.0f;
		  rd.DepthClipEnable = TRUE;
		  rd.ScissorEnable = FALSE;
		  rd.MultisampleEnable = FALSE;

		  ID3D11RasterizerState* rs = nullptr;
		  VALIDATEDX11(device.CreateRasterizerState(&rd, &rs));
		  return rs;
	  }

	  ID3D11RasterizerState* CreateSkyRasterizer(ID3D11Device& device)
	  {
		  D3D11_RASTERIZER_DESC rd;
		  rd.FillMode = D3D11_FILL_SOLID;
		  rd.CullMode = D3D11_CULL_NONE;
		  rd.FrontCounterClockwise = FALSE;
		  rd.DepthBias = 0;
		  rd.DepthBiasClamp = 0.0f;
		  rd.SlopeScaledDepthBias = 0.0f;
		  rd.DepthClipEnable = TRUE;
		  rd.ScissorEnable = FALSE;
		  rd.MultisampleEnable = FALSE;

		  ID3D11RasterizerState* rs = nullptr;
		  VALIDATEDX11(device.CreateRasterizerState(&rd, &rs));
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

	  // Currently used to blend 2D text onto a 3D environment
	  ID3D11BlendState* CreateAlphaAdditiveBlend(ID3D11Device& device)
	  {
		  D3D11_BLEND_DESC alphaBlendDesc;
		  ZeroMemory(&alphaBlendDesc, sizeof(alphaBlendDesc));

		  auto& blender = alphaBlendDesc.RenderTarget[0];

		  blender.SrcBlendAlpha = D3D11_BLEND_ZERO;
		  blender.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		  blender.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		  blender.DestBlendAlpha = D3D11_BLEND_ONE;
		  blender.BlendOpAlpha = D3D11_BLEND_OP_MAX;
		  blender.BlendEnable = TRUE;
		  blender.BlendOp = D3D11_BLEND_OP_ADD;
		  blender.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		  ID3D11BlendState* alphaBlend = nullptr;
		  VALIDATEDX11(device.CreateBlendState(&alphaBlendDesc, &alphaBlend));
		  return alphaBlend;
	  }

	  ID3D11BlendState* CreateAdditiveBlend(ID3D11Device& device)
	  {
		  D3D11_BLEND_DESC blendDesc;
		  ZeroMemory(&blendDesc, sizeof(blendDesc));
		  blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		  blendDesc.RenderTarget[0].BlendEnable = TRUE;
		  blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		  blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		  blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		  blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		  blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;

		  ID3D11BlendState* d11Blend = nullptr;
		  VALIDATEDX11(device.CreateBlendState(&blendDesc, &d11Blend));
		  return d11Blend;
	  }
   }
}