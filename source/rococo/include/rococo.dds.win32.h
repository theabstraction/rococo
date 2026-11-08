#pragma once
#include <rococo.dds.h>

#ifdef _WIN32

#include <d3d11.h>

namespace Rococo::Graphics
{
	ROCOCO_INTERFACE DDSContext
	{
		ID3D11Device& device;
		ID3D11DeviceContext& dc;
		D3D11_USAGE usage;
		uint32 bindFlags;
		uint32 cpuAccessFlags;
		uint32 miscFlags;
	};

	ROCOCO_INTERFACE IDDSSysTexture
	{
		virtual ID3D11Resource& Resource() = 0;
		virtual ID3D11ShaderResourceView& TextureView() = 0;
	};
}

#endif