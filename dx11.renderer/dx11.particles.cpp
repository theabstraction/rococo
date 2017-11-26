#include "rococo.dx11.api.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"

namespace Rococo
{
}

namespace ANON
{
	D3D11_INPUT_ELEMENT_DESC particleVertexDesc[] =
	{
		{ "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
}

namespace Rococo
{
	namespace DX11
	{
		const D3D11_INPUT_ELEMENT_DESC* const GetParticleVertexDesc()
		{
			return ANON::particleVertexDesc;
		}

		const uint32 NumberOfParticleVertexElements()
		{
			return 1;
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