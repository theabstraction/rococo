#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"

namespace Rococo
{
}

namespace ANON
{
	D3D11_INPUT_ELEMENT_DESC particleVertexDesc[] =
	{
		{ "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "color",	  0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "texcoord", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
}

namespace Rococo
{
	namespace DX11
	{
		const D3D11_INPUT_ELEMENT_DESC* const GetParticleVertexDesc()
		{
			static_assert(sizeof(ParticleVertex) == 32, "Particle Vertex: D3D11_INPUT_ELEMENT_DESC needs updating");
			return ANON::particleVertexDesc;
		}

		const uint32 NumberOfParticleVertexElements()
		{
			return 3;
		}

		ID3D11BlendState* CreatePlasmaBlend(ID3D11Device& device)
		{
			D3D11_BLEND_DESC alphaBlendDesc;
			ZeroMemory(&alphaBlendDesc, sizeof(alphaBlendDesc));

			auto& blender = alphaBlendDesc.RenderTarget[0];

			blender.SrcBlendAlpha = D3D11_BLEND_ZERO;
			blender.SrcBlend = D3D11_BLEND_SRC_ALPHA;
			blender.DestBlend = D3D11_BLEND_ONE;
			blender.DestBlendAlpha = D3D11_BLEND_ONE;
			blender.BlendOpAlpha = D3D11_BLEND_OP_MAX;
			blender.BlendEnable = TRUE;
			blender.BlendOp = D3D11_BLEND_OP_ADD;
			blender.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			ID3D11BlendState* alphaBlend = nullptr;
			VALIDATEDX11(device.CreateBlendState(&alphaBlendDesc, &alphaBlend));
			return alphaBlend;
		}

		ID3D11RasterizerState* CreateParticleRasterizer(ID3D11Device& device)
		{
			D3D11_RASTERIZER_DESC desc;
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_NONE;
			desc.FrontCounterClockwise = FALSE;
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0.0f;
			desc.SlopeScaledDepthBias = 0.0f;
			desc.DepthClipEnable = TRUE;
			desc.ScissorEnable = FALSE;
			desc.MultisampleEnable = FALSE;

			ID3D11RasterizerState* rs = nullptr;
			VALIDATEDX11(device.CreateRasterizerState(&desc, &rs));
			return rs;
		}
	}
}