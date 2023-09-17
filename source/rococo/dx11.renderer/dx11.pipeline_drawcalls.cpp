#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <rococo.os.h>
#include <rococo.time.h>
#include "dx11.pipeline.h"

using namespace Rococo::RAL;

namespace Rococo::DX11
{
	void DX11Pipeline::RenderSkyBox(IScene& scene)
	{
		RAL_pipeline->RenderSkyBox(scene);
	}

	void DX11Pipeline::AssignGuiShaderResources()
	{
		gui->AssignShaderResourcesToDC();
	}

	void DX11Pipeline::DisableBlend()
	{
		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
	}

	void DX11Pipeline::UseAdditiveBlend()
	{
		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(additiveBlend, blendFactorUnused, 0xffffffff);
	}

	void DX11Pipeline::UseAlphaBlend()
	{
		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);
	}

	void DX11Pipeline::UseAlphaAdditiveBlend()
	{
		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
	}

	void DX11Pipeline::TargetShadowBuffer(ID_TEXTURE shadowBufferId)
	{
		auto shadowBind = textures.GetTexture(shadowBufferId);

		dc.OMSetDepthStencilState(nullptr, 0);
		dc.OMSetRenderTargets(0, nullptr, shadowBind.depthView);

		D3D11_TEXTURE2D_DESC desc;
		shadowBind.texture->GetDesc(&desc);

		D3D11_VIEWPORT viewport = { 0 };
		viewport.Width = (FLOAT)desc.Width;
		viewport.Height = (FLOAT)desc.Height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		dc.RSSetViewports(1, &viewport);

		dc.ClearDepthStencilView(shadowBind.depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);

		dc.RSSetState(shadowRasterizering);
	}

	void DX11Pipeline::UseObjectRasterizer()
	{
		dc.RSSetState(objectRasterizering);
		dc.OMSetDepthStencilState(objDepthState, 0);
	}

	void DX11Pipeline::UseParticleRasterizer()
	{
		dc.RSSetState(particleRasterizering);
	}

	void DX11Pipeline::UseSkyRasterizer()
	{
		dc.RSSetState(skyRasterizering);
	}

	void DX11Pipeline::UsePlasmaBlend()
	{
		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(plasmaBlend, blendFactorUnused, 0xffffffff);
	}

	void DX11Pipeline::UseSpriteRasterizer()
	{
		dc.RSSetState(spriteRasterizering);
	}

	void DX11Pipeline::UseObjectDepthState()
	{
		dc.OMSetDepthStencilState(objDepthState, 0);
	}

	void DX11Pipeline::DisableWritesOnDepthState()
	{
		dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
	}

	void DX11Pipeline::SetDrawTopology(PrimitiveTopology topology)
	{
		dc.IASetPrimitiveTopology(*reinterpret_cast<D3D11_PRIMITIVE_TOPOLOGY*>(&topology));
	}

	void DX11Pipeline::Render(const GuiMetrics& metrics, Graphics::ENVIRONMENTAL_MAP envMap, IScene& scene)
	{
		RAL_pipeline->Render(metrics, envMap, scene);
	}
} // Rococo::DX11
