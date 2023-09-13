#include <rococo.api.h>
#include <rococo.os.win32.h>
#include "rococo.dx11.h"
#include <rococo.auto-release.h>
#include <rococo.hashtable.h>
#include <rococo.os.h>

#include <d3d11_4.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
	template<class T> void Release(T* t)
	{
		if (t != nullptr)
		{
			t->Release();
		}
	}
	class RenderStageBasic : public IRenderStageSupervisor, public IPrepForDraw
	{
	private:
		uint32 refcount = 1;
		IDX11System& system;
		ID3D11Device5& device;

		struct TextureUnits
		{
			TextureId id;
			uint32 textureUnit;
			AutoRelease<ID3D11SamplerState> sampler;
		};
		std::vector<TextureUnits> txUnits;

		std::vector<RenderTarget> renderTargets;

		TextureId idDepthStencilBuffer;

		HRESULT hr = S_OK;
		HString lastError;

		AutoRelease<ID3D11BlendState> blendState; // LazyInit and LazyRelease 
		AutoRelease<ID3D11RasterizerState2> rasterizerState; // LazyInit and LazyRelease 

		RGBA blendFactor{ 0,0,0,0 };
		uint32 sampleMask = 0xFFFFFFFF;

		D3D11_BLEND_DESC blendDesc;

		D3D11_RECT scissorRect{ 0,0,0,0 };
		D3D11_RASTERIZER_DESC2 rasterDesc;

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

		AutoRelease<ID3D11DepthStencilState> depthStencilState; // LazyInit and LazyRelease 
		uint32 stencilRef = 0;
	public:
		RenderStageBasic(IDX11System& refSystem): system(refSystem), device(refSystem.Device())
		{
			blendDesc.AlphaToCoverageEnable = FALSE;
			blendDesc.IndependentBlendEnable = FALSE;

			for (int i = 0; i < 8; i++)
			{
				auto& B = blendDesc.RenderTarget[i];
				B.BlendEnable = FALSE;
				B.BlendOp = D3D11_BLEND_OP_ADD;
				B.BlendOpAlpha = D3D11_BLEND_OP_ADD;
				B.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				B.DestBlendAlpha = D3D11_BLEND_ONE;
				B.SrcBlend = D3D11_BLEND_SRC_ALPHA;
				B.SrcBlendAlpha = D3D11_BLEND_ZERO;
				B.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			}

			rasterDesc.AntialiasedLineEnable = FALSE;
			rasterDesc.ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			rasterDesc.CullMode = D3D11_CULL_NONE;
			rasterDesc.DepthBias = 0;
			rasterDesc.DepthBiasClamp = 0;
			rasterDesc.DepthClipEnable = FALSE;
			rasterDesc.ScissorEnable = FALSE;
			rasterDesc.MultisampleEnable = FALSE;
			rasterDesc.FillMode = D3D11_FILL_SOLID;
			rasterDesc.ForcedSampleCount = 0;
			rasterDesc.FrontCounterClockwise = TRUE;
			rasterDesc.SlopeScaledDepthBias = 0;

			ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
			depthStencilDesc.DepthEnable = TRUE;
			depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		}

		IRenderPhasePopulator* populator = nullptr;

		void SetPopulator(IRenderPhasePopulator* populator) override
		{
			this->populator = populator;
		}

		void SetEnableDepth(bool isEnabled) override
		{
			depthStencilDesc.DepthEnable = isEnabled ? TRUE : FALSE;
			depthStencilState = nullptr;
		}

		void SetDepthComparison(ComparisonFunc func) override
		{
			depthStencilDesc.DepthFunc = (D3D11_COMPARISON_FUNC) func;
			depthStencilState = nullptr;
		}

		void SetDepthWriteEnable(bool isEnabled) override
		{
			depthStencilDesc.DepthWriteMask = isEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
			depthStencilState = nullptr;
		}

		void ValidateUnit(cstr function, uint32 index) override
		{
			if (index >= 8)
			{
				Throw(0, "%s: Bad unit: %u", function, index);
			}
		}

		void SetEnableScissors(const GuiRect* pRect) override
		{
			if (pRect)
			{
				scissorRect.left = pRect->left;
				scissorRect.top = pRect->top;
				scissorRect.right = pRect->right;
				scissorRect.bottom = pRect->bottom;

				if (!rasterDesc.ScissorEnable)
				{
					rasterDesc.ScissorEnable = TRUE;
					rasterizerState = nullptr;
				}
			}
			else
			{
				if (rasterDesc.ScissorEnable)
				{
					rasterDesc.ScissorEnable = FALSE;
					rasterizerState = nullptr;
				}
			}
		}

		void SetEnableMultisample(bool isEnabled) override
		{
			rasterDesc.MultisampleEnable = isEnabled ? TRUE : FALSE;;
			rasterizerState = nullptr;
		}

		void SetEnableDepthClip(bool isEnabled) override
		{
			rasterDesc.DepthClipEnable = isEnabled ? TRUE : FALSE;;
			rasterizerState = nullptr;
		}

		void SetWireframeRendering(bool isEnabled) override
		{
			rasterDesc.FillMode = isEnabled ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
			rasterizerState = nullptr;
		}

		void SetCullMode(int32 direction) override
		{
			if (direction < 0)
			{
				rasterDesc.CullMode = D3D11_CULL_BACK;
			}
			else if(direction > 0)
			{
				rasterDesc.CullMode = D3D11_CULL_FRONT;
			}
			else
			{
				rasterDesc.CullMode = D3D11_CULL_NONE;
			}

			rasterizerState = nullptr;
		}

		void SetBlendOp(BlendOp op, uint32 unit = 0) override
		{
			ValidateUnit(__FUNCTION__, unit);
			blendDesc.RenderTarget[unit].BlendOp = (D3D11_BLEND_OP) op;
			blendState = nullptr;
		}

		void SetBlendAlphaOp(BlendOp op, uint32 unit = 0) override
		{
			ValidateUnit(__FUNCTION__, unit);
			blendDesc.RenderTarget[unit].BlendOpAlpha = (D3D11_BLEND_OP)op;
			blendState = nullptr;
		}

		void SetEnableBlend(BOOL bEnable, uint32 unit = 0) override
		{
			ValidateUnit(__FUNCTION__, unit);
			blendDesc.RenderTarget[unit].BlendEnable = bEnable;
			blendState = nullptr;
		}

		void SetSrcAlphaBlend(BlendValue value, uint32 unit = 0) override
		{
			ValidateUnit(__FUNCTION__, unit);
			blendDesc.RenderTarget[unit].SrcBlendAlpha = (D3D11_BLEND) value;
			blendState = nullptr;
		}

		void SetDestAlphaBlend(BlendValue value, uint32 unit = 0) override
		{
			ValidateUnit(__FUNCTION__, unit);
			blendDesc.RenderTarget[unit].DestBlendAlpha = (D3D11_BLEND)value;
			blendState = nullptr;
		}

		void SetSrcBlend(BlendValue value, uint32 unit = 0) override
		{
			ValidateUnit(__FUNCTION__, unit);
			blendDesc.RenderTarget[unit].SrcBlend = (D3D11_BLEND)value;
			blendState = nullptr;
		}

		void SetDestBlend(BlendValue value, uint32 unit = 0) override
		{
			ValidateUnit(__FUNCTION__, unit);
			blendDesc.RenderTarget[unit].DestBlend = (D3D11_BLEND)value;
			blendState = nullptr;
		}

		DX11_VIEWPORT viewport{ {0,0},{1024,768},0,1 };

		void SetViewport(Vec2 topLeft, Vec2 span, float minDepth, float maxDepth) override
		{
			viewport.topLeft = topLeft;
			viewport.span = span;
			viewport.MinDepth = minDepth;
			viewport.MaxDepth = maxDepth;
		}

		void UseState()
		{
			auto& dc = system.DC();

			if (!blendState)
			{
				VALIDATE_HR(device.CreateBlendState(&blendDesc, &blendState));
			}

			dc.OMSetBlendState(blendState, blendFactor, sampleMask);

			if (!rasterizerState)
			{
				VALIDATE_HR(device.CreateRasterizerState2(&rasterDesc, &rasterizerState));
			}

			if (rasterDesc.ScissorEnable)
			{
				dc.RSSetScissorRects(1, &scissorRect);
			}

			dc.RSSetState(rasterizerState);

			if (!depthStencilState)
			{
				VALIDATE_HR(device.CreateDepthStencilState(&depthStencilDesc, &depthStencilState));
			}

			dc.OMSetDepthStencilState(depthStencilState, stencilRef);

			dc.RSSetViewports(1, &viewport);
		}

		uint32 AddRef() override
		{
			refcount++;
			return refcount;
		}

		uint32 Release() override
		{
			refcount--;

			if (refcount == 0)
			{
				delete this;
				return 0;
			}

			return refcount;
		}

		struct Constant
		{
			MeshIndex id;
			uint32 constantSlotIndex;
		};

		std::vector<Constant> PSconstants;
		std::vector<Constant> VSconstants;

		void ApplyConstantToPS(MeshIndex id, uint32 constantSlotIndex) override
		{
			PSconstants.push_back({ id, constantSlotIndex });
		}

		void ApplyConstantToVS(MeshIndex id, uint32 constantSlotIndex) override
		{
			VSconstants.push_back({ id, constantSlotIndex });
		}

		void AddInput(TextureId id, uint32 textureUnit, const Sampler& sampler) override
		{
			D3D11_SAMPLER_DESC desc;
			desc.Filter   = (D3D11_FILTER) sampler.filter;
			desc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE) sampler.U;
			desc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE) sampler.V;
			desc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE) sampler.W;
			desc.ComparisonFunc = (D3D11_COMPARISON_FUNC) sampler.comparison;
			desc.MaxAnisotropy = sampler.maxAnisotropy;
			desc.MipLODBias = sampler.mipLODBias;
			desc.MaxLOD = sampler.maxLOD;
			desc.MinLOD = sampler.minLOD;

			ID3D11SamplerState* dx11Sampler = nullptr;
			VALIDATE_HR(system.Device().CreateSamplerState(&desc, &dx11Sampler));
			txUnits.push_back({id, textureUnit, nullptr});
			txUnits.back().sampler = dx11Sampler;
		}

		void AddOutput(TextureId id, uint32 renderTargetIndex, uint32 mipMapIndex, RenderTargetFlags flags, const RGBA& clearColour) override
		{
			if (renderTargets.size() == 8) Throw(0, "%s: Max render targets reached", __FUNCTION__);
			renderTargets.push_back( { id, renderTargetIndex, flags, clearColour });
		}

		float clearDepth = 1.0f;
		uint8 stencilBits = 0;

		void AddDepthStencilBuffer(TextureId id, float clearDepth, uint8 stencilBits) override
		{
			this->idDepthStencilBuffer = id;
			this->clearDepth = clearDepth;
			this->stencilBits = stencilBits;
		}

		void Execute() override
		{
			if (!populator) return;

			try
			{
				hr = E_PENDING;
				lastError = "";
				ExecuteProtected();
				hr = S_OK;
			}
			catch (IException& ex)
			{
				hr = ex.ErrorCode();
				lastError = ex.Message();
			}
		}

		void ExecuteProtected()
		{
			auto& textures = system.Textures();

			Vec2i span{ 0,0 };

			// Inputs
			for (auto& unit : txUnits)
			{
				if (unit.id)
				{
					textures.AssignTextureToShaders(unit.id, unit.textureUnit);
				}

				system.DC().PSSetSamplers(unit.textureUnit, 1, &unit.sampler);
			}

			for (auto& rt : renderTargets)
			{
				if (rt.flags.clearWhenAssigned)
				{
					textures.ClearRenderTarget(rt.id, rt.clearColour);
				}

				if (span.x == 0)
				{
					span = textures.GetSpan(rt.id);
				}
			}

			if (idDepthStencilBuffer)
			{
				textures.ClearDepthBuffer(idDepthStencilBuffer, this->clearDepth, this->stencilBits);
				if (span.x == 0)
				{
					span = textures.GetSpan(idDepthStencilBuffer);
				}
			}

			// Outputs
			textures.UseTexturesAsRenderTargets(renderTargets.data(), (uint32) renderTargets.size(), idDepthStencilBuffer);

			if (span.x != 0)
			{
				populator->SetTargetSpan(span);
			}

			auto& meshes = system.Meshes();

			for (auto& constant : PSconstants)
			{
				meshes.ApplyConstantToPS(constant.id, constant.constantSlotIndex);
			}

			for (auto& constant : VSconstants)
			{
				meshes.ApplyConstantToVS(constant.id, constant.constantSlotIndex);
			}

			UseState();

			populator->RenderStage(*this);
		}

		void OnShaderChange()
		{
			auto& meshes = system.Meshes();

			for (auto& constant : PSconstants)
			{
				meshes.ApplyConstantToPS(constant.id, constant.constantSlotIndex);
			}

			for (auto& constant : VSconstants)
			{
				meshes.ApplyConstantToVS(constant.id, constant.constantSlotIndex);
			}
		}
	};
}

namespace Rococo::Graphics
{
	IRenderStageSupervisor* CreateRenderStageBasic(IDX11System& system)
	{
		return new ANON::RenderStageBasic(system);
	}
}