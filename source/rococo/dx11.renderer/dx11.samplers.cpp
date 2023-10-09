#include "dx11.renderer.h"
#include <rococo.renderer.h>
#include "rococo.dx11.api.h"
#include "dx11helpers.inl"

namespace Rococo::DX11
{
	using namespace Rococo::Graphics::Samplers;

	D3D11_TEXTURE_ADDRESS_MODE From(Samplers::AddressMode mode)
	{
		switch (mode)
		{
		case AddressMode::Border:
			return D3D11_TEXTURE_ADDRESS_BORDER;
		case AddressMode::Wrap:
			return D3D11_TEXTURE_ADDRESS_WRAP;
		case AddressMode::Mirror:
			return D3D11_TEXTURE_ADDRESS_MIRROR;
		case AddressMode::MirrorOnce:
			return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
		case AddressMode::Clamp:
			return D3D11_TEXTURE_ADDRESS_CLAMP;
		default:
			Throw(0, "Unknown sampler address mode %d", mode);
		}
	}

	ID3D11SamplerState* GetSampler(ID3D11Device& device, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour)
	{
		D3D11_SAMPLER_DESC desc;

		switch (filter)
		{
		case Filter::Point:
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			break;
		default:
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		}

		desc.AddressU = From(u);
		desc.AddressV = From(v);
		desc.AddressW = From(w);
		desc.BorderColor[0] = borderColour.red;
		desc.BorderColor[1] = borderColour.green;
		desc.BorderColor[2] = borderColour.blue;
		desc.BorderColor[3] = borderColour.alpha;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.MaxAnisotropy = 1;
		desc.MipLODBias = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		desc.MinLOD = 0;

		ID3D11SamplerState* state = nullptr;
		VALIDATEDX11(device.CreateSamplerState(&desc, &state));
		return state;
	}
}

