#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <rococo.os.h>
#include <rococo.time.h>
#include "dx11.pipeline.h"

using namespace Rococo::DX11;

namespace Rococo::DX11
{
	DX11Pipeline::DX11Pipeline(IO::IInstallation& _installation, IRendererMetrics& metrics, IDX11ResourceLoader& resourceLoader, IDX11Shaders& _shaders, IDX11TextureManager& _textures, IDX11Meshes& _meshes, IDX11Renderer& _renderer, IRenderContext& _rc, ID3D11Device& _device, ID3D11DeviceContext& _dc) :
		installation(_installation), device(_device), dc(_dc), shaders(_shaders), meshes(_meshes), textures(_textures), renderer(_renderer), rc(_rc)
	{
		objDepthState = DX11::CreateObjectDepthStencilState(device);
		objDepthState_NoWrite = DX11::CreateObjectDepthStencilState_NoWrite(device);
		noDepthTestOrWrite = DX11::CreateNoDepthCheckOrWrite(device);

		spriteRasterizering = DX11::CreateSpriteRasterizer(device);
		objectRasterizering = DX11::CreateObjectRasterizer(device);
		particleRasterizering = DX11::CreateParticleRasterizer(device);
		skyRasterizering = DX11::CreateSkyRasterizer(device);
		shadowRasterizering = DX11::CreateShadowRasterizer(device);

		alphaAdditiveBlend = DX11::CreateAlphaAdditiveBlend(device);
		disableBlend = DX11::CreateNoBlend(device);
		additiveBlend = DX11::CreateAdditiveBlend(device);
		plasmaBlend = DX11::CreatePlasmaBlend(device);

		instanceBuffer = DX11::CreateConstantBuffer<ObjectInstance>(device);
		lightConeBuffer = DX11::CreateDynamicVertexBuffer<ObjectVertex>(device, 3);
		particleBuffer = DX11::CreateDynamicVertexBuffer<ParticleVertex>(device, PARTICLE_BUFFER_VERTEX_CAPACITY);

		depthRenderStateBuffer = DX11::CreateConstantBuffer<DepthRenderData>(device);
		lightStateBuffer = DX11::CreateConstantBuffer<Light>(device);

		ambientBuffer = DX11::CreateConstantBuffer<AmbientData>(device);
		gui3DBuffer = DX11::CreateDynamicVertexBuffer<ObjectVertex>(device, GUI3D_BUFFER_VERTEX_CAPACITY);

		gui = CreateDX11Gui(device, dc, textures, metrics, resourceLoader, shaders);

		idObjVS = shaders.CreateObjectVertexShader("!object.vs");
		idObjPS = shaders.CreatePixelShader("!object.ps");
		idObj_Spotlight_NoEnvMap_PS = shaders.CreatePixelShader("!obj.spotlight.no_env.ps");
		idObj_Ambient_NoEnvMap_PS = shaders.CreatePixelShader("!obj.ambient.no_env.ps");
		idObjAmbientPS = shaders.CreatePixelShader("!ambient.ps");
		idObjAmbientVS = shaders.CreateObjectVertexShader("!ambient.vs");
		idObjVS_Shadows = shaders.CreateObjectVertexShader("!shadow.vs");
		idSkinnedObjVS_Shadows = shaders.CreateVertexShader("!skinned.shadow.vs", DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
		idObjPS_Shadows = shaders.CreatePixelShader("!shadow.ps");
		idLightConePS = shaders.CreatePixelShader("!light_cone.ps");
		idLightConeVS = shaders.CreateVertexShader("!light_cone.vs", DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
		idParticleVS = shaders.CreateParticleVertexShader("!particle.vs");
		idPlasmaGS = shaders.CreateGeometryShader("!plasma.gs");
		idFogSpotlightGS = shaders.CreateGeometryShader("!fog.spotlight.gs");
		idFogAmbientGS = shaders.CreateGeometryShader("!fog.ambient.gs");
		idPlasmaPS = shaders.CreatePixelShader("!plasma.ps");
		idFogSpotlightPS = shaders.CreatePixelShader("!fog.spotlight.ps");
		idFogAmbientPS = shaders.CreatePixelShader("!fog.ambient.ps");
		idObjSkyVS = shaders.CreateVertexShader("!skybox.vs", DX11::GetSkyVertexDesc(), DX11::NumberOfSkyVertexElements());
		idObjSkyPS = shaders.CreatePixelShader("!skybox.ps");

		shadowBufferId = textures.CreateDepthTarget("ShadowBuffer", 1024, 1024);

		D3D11_SAMPLER_DESC desc;
		GetSkySampler(desc);
		VALIDATEDX11(device.CreateSamplerState(&desc, &skySampler));

		alphaBlend = DX11::CreateAlphaBlend(device);

		boneMatricesStateBuffer = DX11::CreateConstantBuffer<BoneMatrices>(device);
		globalStateBuffer = DX11::CreateConstantBuffer<GlobalState>(device);
		sunlightStateBuffer = DX11::CreateConstantBuffer<Vec4>(device);

		RGBA red{ 1.0f, 0, 0, 1.0f };
		RGBA transparent{ 0.0f, 0, 0, 0.0f };
		SetSampler(TXUNIT_FONT, Filter_Linear, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		SetSampler(TXUNIT_SHADOW, Filter_Linear, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		SetSampler(TXUNIT_ENV_MAP, Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		SetSampler(TXUNIT_SELECT, Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		SetSampler(TXUNIT_MATERIALS, Filter_Linear, AddressMode_Wrap, AddressMode_Wrap, AddressMode_Wrap, red);
		SetSampler(TXUNIT_SPRITES, Filter_Point, AddressMode_Border, AddressMode_Border, AddressMode_Border, red);
		SetSampler(TXUNIT_GENERIC_TXARRAY, Filter_Point, AddressMode_Border, AddressMode_Border, AddressMode_Border, transparent);
	}

	ID_PIXEL_SHADER DX11Pipeline::GetObjectShaderPixelId(RenderPhase phase)
	{
		switch (phaseConfig.EnvironmentalMap)
		{
		case Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE:
			switch (phase)
			{
			case RenderPhase_DetermineAmbient:
				return idObjAmbientPS;
			case RenderPhase_DetermineSpotlight:
				return idObjPS;
			case RenderPhase_DetermineShadowVolumes:
				return idObjPS_Shadows;
			default:
				Throw(0, "Unknown render phase: %d", phase);
			}
		case Graphics::ENVIRONMENTAL_MAP_PROCEDURAL:
			switch (phase)
			{
			case RenderPhase_DetermineAmbient:
				return idObj_Ambient_NoEnvMap_PS;
			case RenderPhase_DetermineSpotlight:
				return idObj_Spotlight_NoEnvMap_PS;
			case RenderPhase_DetermineShadowVolumes:
				return idObjPS_Shadows;
			default:
				Throw(0, "Unknown render phase: %d", phase);
			}
		default:
			Throw(0, "Environment mode %d not implemented", phaseConfig.EnvironmentalMap);
		}
	}

	void DX11Pipeline::Add3DGuiTriangles(const VertexTriangle* first, const VertexTriangle* last)
	{
		for (auto i = first; i != last; ++i)
		{
			gui3DTriangles.push_back(*i);
		}
	}

	void DX11Pipeline::Clear3DGuiTriangles()
	{
		gui3DTriangles.clear();
	}

	void DX11Pipeline::AddFog(const ParticleVertex& p)
	{
		fog.push_back(p);
	}

	void DX11Pipeline::AddPlasma(const ParticleVertex& p)
	{
		plasma.push_back(p);
	}

	void DX11Pipeline::ClearPlasma()
	{
		plasma.clear();
	}

	void DX11Pipeline::ClearFog()
	{
		fog.clear();
	}

	void DX11Pipeline::Free()
	{
		delete this;
	}

	void DX11Pipeline::AssignGlobalStateBufferToShaders()
	{
		dc.PSSetConstantBuffers(0, 1, &globalStateBuffer);
		dc.VSSetConstantBuffers(0, 1, &globalStateBuffer);
	}

	void DX11Pipeline::ShowVenue(IMathsVisitor& visitor)
	{
		visitor.ShowString("Geometry this frame", "%lld triangles. %lld entities, %lld particles", trianglesThisFrame, entitiesThisFrame, plasma.size() + fog.size());
		gui->ShowVenue(visitor);
	}

	void DX11Pipeline::SetAmbientConstants()
	{
		AmbientData ad;
		ad.localLight = ambientLight.ambient;
		ad.fogConstant = ambientLight.fogConstant;
		DX11::CopyStructureToBuffer(dc, ambientBuffer, ad);
		dc.PSSetConstantBuffers(CBUFFER_INDEX_AMBIENT_LIGHT, 1, &ambientBuffer);
	}

	void DX11Pipeline::UpdateGlobalState(const GuiMetrics& metrics, IScene& scene)
	{
		GlobalState g;
		scene.GetCamera(g.worldMatrixAndProj, g.worldMatrix, g.projMatrix, g.eye, g.viewDir);

		float aspectRatio = metrics.screenSpan.y / (float)metrics.screenSpan.x;
		g.aspect = { aspectRatio,0,0,0 };

		g.guiScale = gui->GetGuiScale();

		DX11::CopyStructureToBuffer(dc, globalStateBuffer, g);

		currentGlobalState = g;

		dc.VSSetConstantBuffers(CBUFFER_INDEX_GLOBAL_STATE, 1, &globalStateBuffer);
		dc.PSSetConstantBuffers(CBUFFER_INDEX_GLOBAL_STATE, 1, &globalStateBuffer);
		dc.GSSetConstantBuffers(CBUFFER_INDEX_GLOBAL_STATE, 1, &globalStateBuffer);

		Vec4 sunlight = { Sin(45_degrees), 0, Cos(45_degrees), 0 };
		Vec4 sunlightLocal = sunlight;

		DX11::CopyStructureToBuffer(dc, sunlightStateBuffer, sunlightLocal);

		dc.VSSetConstantBuffers(CBUFFER_INDEX_SUNLIGHT, 1, &sunlightStateBuffer);
		dc.PSSetConstantBuffers(CBUFFER_INDEX_SUNLIGHT, 1, &sunlightStateBuffer);
		dc.GSSetConstantBuffers(CBUFFER_INDEX_SUNLIGHT, 1, &sunlightStateBuffer);

		DX11::CopyStructureToBuffer(dc, boneMatricesStateBuffer, boneMatrices);
		dc.VSSetConstantBuffers(CBUFFER_INDEX_BONE_MATRICES, 1, &boneMatricesStateBuffer);
	}

	void DX11Pipeline::SetupSpotlightConstants()
	{
		dc.VSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
		dc.PSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
		dc.GSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
	}

	RenderTarget DX11Pipeline::GetCurrentRenderTarget()
	{
		RenderTarget rt = { 0 };

		rt.depthView = textures.GetTexture(phaseConfig.depthTarget).depthView;

		if (phaseConfig.renderTarget)
		{
			rt.renderTargetView = textures.GetTexture(phaseConfig.renderTarget).renderView;
		}
		else
		{
			rt.renderTargetView = renderer.BackBuffer();
		}

		if (rt.depthView == nullptr)
		{
			Throw(0, "GetCurrentRenderTarget - bad depth buffer");
		}

		if (rt.renderTargetView == nullptr)
		{
			Throw(0, "GetCurrentRenderTarget - bad render target buffer");
		}

		return rt;
	}

	void DX11Pipeline::ClearCurrentRenderBuffers(const RGBA& clearColour)
	{
		RenderTarget rt = GetCurrentRenderTarget();
		dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

		dc.ClearDepthStencilView(rt.depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		if (clearColour.alpha > 0)
		{
			dc.ClearRenderTargetView(rt.renderTargetView, (const FLOAT*)&clearColour);
		}
	}

	void DX11Pipeline::SetSampler(uint32 index, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour)
	{
		if (samplers[index])
		{
			samplers[index]->Release();
			samplers[index] = nullptr;
		}

		auto* sampler = Rococo::DX11::GetSampler(device, filter, u, v, w, borderColour);
		samplers[index] = sampler;
	}

	void DX11Pipeline::ResetSamplersToDefaults()
	{
		dc.PSSetSamplers(0, 16, samplers);
		dc.GSSetSamplers(0, 16, samplers);
		dc.VSSetSamplers(0, 16, samplers);
	}

	void DX11Pipeline::SetBoneMatrix(uint32 index, cr_m4x4 m)
	{
		if (index >= BoneMatrices::BONE_MATRIX_CAPACITY)
		{
			Throw(0, "Bad bone index #%u", index);
		}

		auto& target = boneMatrices.bones[index];
		target = m;
		target.row3 = Vec4{ 0, 0, 0, 1.0f };
	}

	IDX11Pipeline* CreateDX11Pipeline(IO::IInstallation& installation, IRendererMetrics& metrics, IDX11ResourceLoader& resourceLoader, IDX11Shaders& shaders, IDX11TextureManager& textures, IDX11Meshes& meshes, IDX11Renderer& renderer, IRenderContext& rc, ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11Pipeline(installation, metrics, resourceLoader, shaders, textures, meshes, renderer, rc, device, dc);
	}
}
