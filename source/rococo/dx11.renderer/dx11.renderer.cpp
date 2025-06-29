#define ROCOCO_USE_SAFE_V_FORMAT
#include "dx11.renderer.h"
#include <rococo.renderer.h>
#include <rococo.imaging.h>
#include <rococo.fonts.h>
#include <rococo.maths.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.time.h>
#include <rococo.hashtable.h>
#include <rococo.win32.rendering.h>
#include <rococo.fonts.hq.h>
#include <rococo.subsystems.h>
#include <RAL\RAL.h>
#include <RAL\RAL.pipeline.h>
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include "rococo.visitors.h"
#include "dx11.factory.h"
#include <rococo.strings.h>
#include <rococo.reflector.h>

#include <Dxgi1_3.h>
#include <comdef.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <unordered_set>

// All of these objects are used in constant buffers and must be a multiple of 16 bytes in length;
static_assert(sizeof(Rococo::Graphics::ObjectInstance) % 16 == 0);
static_assert(sizeof(Rococo::Graphics::GuiScale) % 16 == 0);
static_assert(sizeof(Rococo::Graphics::GlobalState) % 16 == 0);

namespace Rococo::DX11
{
	void ShowWindowVenue(HWND hWnd, IMathsVisitor& visitor);
}

using namespace Rococo;
using namespace Rococo::RAL;
using namespace Rococo::Graphics::Fonts;
using namespace Rococo::Windows;
using namespace Rococo::Graphics::Samplers;
using namespace Rococo::DX11;
using namespace Rococo::Reflection;
using namespace Rococo::Graphics;
using namespace Rococo::Graphics::Textures;

class DX11AppRenderer :
	public IDX11Renderer,
	public IRenderContext,
	public IMathsVenue,
	public IDX11ResourceLoader,
	public IRAL,
	public IDX11SpecialResources,
	public ISubsystem,
	public VisitationTarget
{
private:
	IO::IInstallation& installation;
	ID3D11Device& device;
	ID3D11DeviceContext& dc;
	IDXGIFactory1& factory;
	AutoFree<IExpandingBuffer> scratchBuffer;
	const UINT adapterIndex;

	AutoFree<IDX11TextureManager> textureManager;
	AutoFree<IDX11Meshes> meshes;
	AutoFree<IDX11Shaders> shaders;

	IDX11WindowBacking* currentWindowBacking = nullptr;

	Time::ticks lastTick;

	RAWMOUSE lastMouseEvent;
	Vec2i screenSpan;

	int64 frameIndex = 0;

	std::vector<ID3D11Buffer*> boundVertexBuffers;
	std::vector<UINT> boundVertexBufferStrides;
	std::vector<UINT> boundVertexBufferOffsets;

	AutoFree<IDX11Pipeline> pipeline;

	size_t EnumerateScreenModes(Rococo::Function<void(const ScreenMode&)> onMode) override
	{
		AutoRelease<IDXGIOutput> output = currentWindowBacking->GetOutput();
		
		AutoRelease<IDXGIOutput1> output1;
		VALIDATEDX11(output->QueryInterface<IDXGIOutput1>(&output1));

		UINT nModes = 1;
		VALIDATEDX11(output1->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &nModes, NULL));

		std::vector<DXGI_MODE_DESC1> modes;
		modes.resize(nModes);
		VALIDATEDX11(output1->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &nModes, modes.data()));

		// We could do this in the mode enumeration below, but its nice to see in debug mode how much stuff we filter out
		auto i = std::remove_if(modes.begin(), modes.end(), [](const DXGI_MODE_DESC1& mode)->bool
			{
				if (mode.ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE || mode.Scaling == DXGI_MODE_SCALING_CENTERED)
				{
					return true;
				}

				return false;
			}
		);

		modes.erase(i, modes.end());

		struct ScreenModeHasher
		{
			size_t operator()(const ScreenMode& mode) const
			{
				return mode.DX + 735 * mode.DY;
			}
		};

		struct ScreenModeComparer
		{
			bool operator()(const ScreenMode& a, const ScreenMode& b) const
			{
				return a.DX == b.DX && a.DY == b.DY;
			}
		};

		std::unordered_set<ScreenMode, ScreenModeHasher, ScreenModeComparer> uniqueModes;
		
		for (auto& mode : modes)
		{
			ScreenMode sm;
			sm.DX = mode.Width;
			sm.DY = mode.Height;
			uniqueModes.insert(sm);
		}

		for(auto& mode: uniqueModes)
		{
			onMode.Invoke(mode);
		}

		return uniqueModes.size();
	}

	IRALVertexDataBuffer* CreateDynamicVertexBuffer(size_t sizeofStruct, size_t nElements) override
	{
		return new DX11_RAL_DynamicVertexBuffer(device, dc, sizeofStruct, nElements);
	}

	IRALConstantDataBuffer* CreateConstantBuffer(size_t sizeofStruct, size_t nElements) override
	{
		return new DX11_RALConstantBuffer(device, dc, sizeofStruct, nElements);
	}

	ITextureManager& RALTextures() override
	{
		return *textureManager;
	}

	void ClearBoundVertexBufferArray() override
	{
		boundVertexBuffers.clear();
		boundVertexBufferStrides.clear();
		boundVertexBufferOffsets.clear();
	}

	void BindVertexBuffer(ID_SYS_MESH meshId, size_t sizeofVertex, uint32 offset) override
	{
		auto& m = meshes->GetBuffer(meshId);
		boundVertexBuffers.push_back(&DX11::ToDX11(*m.vertexBuffer));
		boundVertexBufferStrides.push_back((uint32)sizeofVertex);
		boundVertexBufferOffsets.push_back(offset);
	}

	void BindVertexBuffer(IRALVertexDataBuffer* vertexBuffer, size_t sizeofVertex, uint32 offset) override
	{
		if (vertexBuffer == nullptr)
		{
			Throw(0, "%s: vertexBuffer was nullptr", __ROCOCO_FUNCTION__);
		}

		boundVertexBuffers.push_back(static_cast<IDX11IRALVertexDataBuffer*>(vertexBuffer)->RawBuffer());
		boundVertexBufferStrides.push_back((uint32) sizeofVertex);
		boundVertexBufferOffsets.push_back(offset);
	}

	void CommitBoundVertexBuffers() override
	{
		if (boundVertexBuffers.size() == 0)
		{
			Throw(0, "%s: boundVertexBuffers size was zero", __ROCOCO_FUNCTION__);
		}

		dc.IASetVertexBuffers(0, (UINT) boundVertexBuffers.size(), boundVertexBuffers.data(), boundVertexBufferStrides.data(), boundVertexBufferOffsets.data());
	}

	void Draw(uint32 nVertices, uint32 startPosition) override
	{
		dc.Draw(nVertices, startPosition);
	}

	ID_VERTEX_SHADER vsFillerId;
	AutoRelease<ID3D11Buffer> unitSquare;
	AutoRelease<ID3D11RasterizerState> fillerRasterizering;
	AutoRelease<ID3D11BlendState> fillerBlend;
	AutoRelease<ID3D11DepthStencilState> fillerDepthStencilState;

	void Fill(ID_TEXTURE renderTargetId, ID_PIXEL_SHADER pixelShaderId)
	{
		if (!vsFillerId)
		{
			D3D11_INPUT_ELEMENT_DESC fillerVertexDesc[] =
			{
				 { "position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			vsFillerId = shaders->CreateVertexShader("!shaders/compiled/filler.vs", fillerVertexDesc, 1);
		}

		if (!unitSquare)
		{
			Vec2 vertices[6] =
			{
				{-1,-1}, {-1,1}, {1,1},
				{1,1}, {1,-1}, {-1,-1}
			};

			unitSquare = CreateImmutableVertexBuffer(device, vertices, 6);
		}

		if (!fillerRasterizering)
		{
			fillerRasterizering = DX11::CreateSpriteRasterizer(device);
		}

		if (!fillerBlend)
		{
			fillerBlend = DX11::CreateNoBlend(device);
		}

		if (!fillerDepthStencilState)
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			VALIDATEDX11(device.CreateDepthStencilState(&desc, &fillerDepthStencilState));
		}

		if (!shaders->UseShaders(vsFillerId, pixelShaderId))
		{
			Throw(0, "%s: error applying shaders.", __ROCOCO_FUNCTION__);
		}

		UINT stride = sizeof(Vec2);
		UINT offsets = 0;

		auto& tb = textureManager->GetTexture(renderTargetId);

		if (!tb.renderView)
		{
			Throw(0, "%s: no render view for the given render target", __ROCOCO_FUNCTION__);
		}

		D3D11_TEXTURE2D_DESC desc;
		tb.texture->GetDesc(&desc);

		Vec4 blendFactors{ 0,0,0,0 };
		dc.OMSetBlendState(fillerBlend, &blendFactors.x, 0xFFFF'FFFF);
		dc.RSSetState(fillerRasterizering);
		dc.OMSetDepthStencilState(fillerDepthStencilState, 0);
		dc.OMSetRenderTargets(1, &tb.renderView, nullptr);
		dc.IASetVertexBuffers(0, 1, &unitSquare, &stride, &offsets);
		dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D11_VIEWPORT viewport = { 0 };
		viewport.Width = FLOAT(desc.Width);
		viewport.Height = FLOAT(desc.Height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		dc.RSSetViewports(1, &viewport);
		dc.Draw(6, 0);

		dc.OMSetRenderTargets(0, nullptr, nullptr);
	}

	bool IsFullscreen() override
	{
		return currentWindowBacking ? currentWindowBacking->IsFullscreen() : false;
	}

	void SetFullscreenMode(const ScreenMode& mode)
	{
		if (currentWindowBacking) currentWindowBacking->SetFullscreenMode(mode);
	}

	void SwitchToFullscreen() override
	{
		if (currentWindowBacking) currentWindowBacking->SwitchToFullscreen();
	}

	void SetSampler(uint32 index, Samplers::Filter filter, Samplers::AddressMode u, Samplers::AddressMode v, Samplers::AddressMode w, const RGBA& borderColour) override
	{
		pipeline->SetSamplerDefaults(index, filter, u, v, w, borderColour);
	}

	void SetWindowBacking(IDX11WindowBacking* windowBacking) override
	{
		currentWindowBacking = windowBacking;
	}
	   
	void Load(cstr name, IEventCallback<CompressedTextureBuffer>& onLoad) override
	{
		StandardLoadFromCompressedTextureBuffer(name, onLoad, installation, *scratchBuffer);
	}

	void LoadTextFile(cstr pingPath, Rococo::Function<void(const fstring& text)> callback)  override
	{
		installation.LoadResource(pingPath, *scratchBuffer, 256_kilobytes);
		fstring text{ (cstr) scratchBuffer->GetData(), (int32) scratchBuffer->Length() };
		callback(text);
	}

	void SetEnvironmentMap(ID_CUBE_TEXTURE envId) override
	{
		auto* envMap = textureManager->DX11CubeTextures().GetShaderView(envId);
		dc.PSSetShaderResources(TXUNIT_ENV_MAP, 1, &envMap);
	}

	void SetSpecialAmbientShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) override
	{
		auto& m = meshes->GetBuffer(id);

		ID_PIXEL_SHADER pxId = Shaders().CreatePixelShader(ps);

		m.psAmbientShader = pxId;

		ID_VERTEX_SHADER vxId;

		if (strstr(vs, "skinned"))
		{
			vxId = shaders->CreateVertexShader(vs, DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
		}
		else
		{
			vxId = shaders->CreateVertexShader(vs, DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
		}

		m.vsAmbientShader = vxId;
		m.alphaBlending = alphaBlending;
	}

	void SetSpecialSpotlightShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) override
	{
		auto& m = meshes->GetBuffer(id);

		ID_PIXEL_SHADER pxId = Shaders().CreatePixelShader(ps);

		m.psSpotlightShader = pxId;

		ID_VERTEX_SHADER vxId;

		if (strstr(vs, "skinned"))
		{
			vxId = shaders->CreateVertexShader(vs, DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
		}
		else
		{
			vxId = shaders->CreateVertexShader(vs, DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
		}

		m.vsSpotlightShader = vxId;
		m.alphaBlending = alphaBlending;
	}

	void SetBoneMatrix(uint32 index, cr_m4x4 m) override
	{
		pipeline->SetBoneMatrix(index, m);
	}

public:
	bool isBuildingAlphaBlendedSprites{ false };

	DX11AppRenderer(DX11::Factory& _factory, IShaderOptions& options) :
		installation(_factory.installation), 
		device(_factory.device), dc(_factory.dc), factory(_factory.factory),
		adapterIndex(_factory.adapterIndex),
		scratchBuffer(CreateExpandingBuffer(64_kilobytes)),
		textureManager(CreateTextureManager(installation, device, dc, *this)),
		meshes(CreateMeshManager(device, dc)),
		shaders(CreateShaderManager(installation, options, device, dc))
	{
		RenderBundle bundle{ installation, *this, *this, *shaders, *textureManager, *meshes, *this, *this, device, dc, *this, *this };

		pipeline = CreateDX11Pipeline(bundle);
		lastTick = Time::TickCount();
	}

	virtual ~DX11AppRenderer()
	{
		DetachContext();
	}

	IRenderContext& RenderContext() override
	{
		return *this;
	}

	IGuiResources& GuiResources() override
	{
		return pipeline->GuiResources();
	}

	IMaterials& Materials() override
	{
		return textureManager->Materials();
	}

	ITextureManager& Textures() override
	{
		return *textureManager;
	}

	IMeshes& Meshes() override
	{
		return *meshes;
	}

	IShaders& Shaders() override
	{
		return *shaders;
	}

	IDX11Shaders& DX11Shaders() override
	{
		return *shaders;
	}

	IGui3D& Gui3D() override
	{
		return pipeline->Gui3D();
	}

	void CaptureMouse(bool enable) override
	{
		if (enable && currentWindowBacking) SetCapture(currentWindowBacking->Window());
		else ReleaseCapture();
	}

	struct : IMathsVenue
	{
		DX11AppRenderer* This;
		virtual void ShowVenue(IMathsVisitor& visitor)
		{
			This->ShowTextureVenue(visitor);
		}
	} textureVenue;

	virtual IMathsVenue* TextureVenue()
	{
		textureVenue.This = this;
		return &textureVenue;
	}

	void ShowTextureVenue(IMathsVisitor& visitor)
	{
		textureManager->Materials().ShowVenue(visitor);
	}

	void ShowVenue(IMathsVisitor& visitor) override
	{
#ifdef _DEBUG
		visitor.ShowString("Renderer", "DirectX 11.1 Rococo MPLAT - Debug");
#else
		visitor.ShowString("Renderer", "DirectX 11.1 Rococo MPLAT - Release");
#endif
		visitor.ShowString("Screen Span", "%d x %d pixels", screenSpan.x, screenSpan.y);

		shaders->ShowVenue(visitor);

		ShowVenueForDevice(visitor, device);

		if (currentWindowBacking)
		{
			auto* renderTarget = currentWindowBacking->BackBufferView();
			if (renderTarget)
			{
				D3D11_RENDER_TARGET_VIEW_DESC desc;
				renderTarget->GetDesc(&desc);
				visitor.ShowString("BackBuffer format", "%u", desc.Format);
			}

			D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc;
			textureManager->GetTexture(currentWindowBacking->DepthBufferId()).depthView->GetDesc(&dsDesc);
			visitor.ShowString("DepthStencil format", "%u", dsDesc.Format);
		}

		visitor.ShowDecimal("Number of textures", (int64)textureManager->Size());
		meshes->ShowVenue(visitor);

		double hz = (double)Time::TickHz();

		double ticks_to_ms = 1000.0 / hz;

		visitor.ShowString("Frame Profiles", "---------------");
		visitor.ShowString("AI+Logic Time", "%3.0lf ms", AIcost * ticks_to_ms);
		visitor.ShowString("UI Render Time", "%3.0lf ms", guiCost * ticks_to_ms);
		visitor.ShowString("3D Render Time", "%3.0lf ms", objCost * ticks_to_ms);
		visitor.ShowString("Present Cost", "%3.0lf ms", presentCost * ticks_to_ms);
		visitor.ShowString("Total Frame Time", "%3.0lf ms", frameTime * ticks_to_ms);
		visitor.ShowString("Frame Rate", "%.0lf FPS", hz / frameTime);
		visitor.ShowString("", "");

		pipeline->ShowVenue(visitor);
	}

	IMathsVenue* Venue()
	{
		return this;
	}

	bool isFullScreen = false;

	void ShowWindowVenue(IMathsVisitor& visitor)
	{
		if (currentWindowBacking)
		{
			visitor.ShowString("IsFullScreen", isFullScreen ? "TRUE" : "FALSE");
			DX11::ShowWindowVenue(currentWindowBacking->Window(), visitor);
		}
	}

	void Free()
	{
		delete this;
	}

	IO::IInstallation& Installation() override
	{
		return installation;
	}

	IRenderer& Renderer() override
	{
		return *this;
	}

	void ExpandViewportToEntireSpan(Vec2i span) override
	{
		D3D11_VIEWPORT viewport = { 0 };
		viewport.Width = FLOAT(span.x);
		viewport.Height = FLOAT(span.y);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		dc.RSSetViewports(1, &viewport);

		screenSpan.x = (int32)viewport.Width;
		screenSpan.y = (int32)viewport.Height;
	}

	ID3D11RenderTargetView* BackBuffer() override
	{
		return currentWindowBacking ? currentWindowBacking->BackBufferView() : nullptr;
	}

	void DetachContext()
	{
		for (UINT i = 0; i < 10; ++i)
		{
			ID3D11ShaderResourceView* nullView = nullptr;
			dc.PSSetShaderResources(i, 1, &nullView);
		}

		ID3D11Buffer* nullBuffer = nullptr;
		for (UINT i = 0; i < 8; ++i)
		{
			dc.VSSetConstantBuffers(i, 1, &nullBuffer);
			dc.PSSetConstantBuffers(i, 1, &nullBuffer);
			dc.GSSetConstantBuffers(i, 1, &nullBuffer);
		}

		dc.OMSetBlendState(nullptr, nullptr, 0);
		dc.RSSetState(nullptr);
		dc.OMSetDepthStencilState(nullptr, 0);
		dc.OMSetRenderTargets(0, nullptr, nullptr);

		UINT nStrides = 0;
		dc.IASetVertexBuffers(0, 1, &nullBuffer, &nStrides, &nStrides);

		dc.IASetInputLayout(nullptr);
		dc.VSSetShader(nullptr, nullptr, 0);
		dc.PSSetShader(nullptr, nullptr, 0);

		ID3D11ShaderResourceView* nullView = nullptr;
		dc.PSSetShaderResources(0, 1, &nullView);

		ID3D11SamplerState* sampler = nullptr;
		for (UINT i = 0; i < 16; ++i)
		{
			dc.PSSetSamplers(i, 1, &sampler);
			dc.GSSetSamplers(i, 1, &sampler);
			dc.VSSetSamplers(i, 1, &sampler);
		}
	}

	void SwitchToWindowMode() override
	{
		if (currentWindowBacking)
		{
			currentWindowBacking->SwitchToWindowMode();
		}
	}

	void OnMouseEvent(const RAWMOUSE& m)
	{
		lastMouseEvent = m;
	}

	void GetGuiMetrics(GuiMetrics& metrics) const override
	{
		POINT p;
		GetCursorPos(&p);

		if (currentWindowBacking)
		{
			ScreenToClient(currentWindowBacking->Window(), &p);
			metrics.cursorPosition = Vec2i{ p.x, p.y };

			RECT rect;
			GetClientRect(currentWindowBacking->Window(), &rect);
			metrics.screenSpan = { rect.right, rect.bottom };
		}
		else
		{
			metrics.cursorPosition = Vec2i{ -1, -1 };
			metrics.screenSpan = Vec2i{ 1, 1 };
		}

		metrics.frameIndex = frameIndex;
	}

	void Draw(ID_SYS_MESH id, const ObjectInstance* instances, uint32 nInstances) override
	{
		auto& m = meshes->GetBuffer(id);
		pipeline->RALPipeline().DrawViaObjectRenderer(m, instances, nInstances);
	}

	Windows::IWindow& CurrentWindow() override
	{
		return currentWindowBacking ? currentWindowBacking->Window() : Rococo::Windows::NoParent();
	}

	Time::ticks AIcost = 0;
	Time::ticks guiCost = 0;
	Time::ticks objCost = 0;
	Time::ticks presentCost = 0;
	Time::ticks frameTime = 0;

	IParticles& Particles() override
	{
		return pipeline->RALPipeline().Particles();
	}

	void RenderToBackBufferAndPresent(IScene& scene) override
	{
		if (!BackBuffer())
		{
			return;
		}

		auto now = Time::TickCount();
		AIcost = now - lastTick;

		GuiMetrics metrics;
		GetGuiMetrics(metrics);

		if (metrics.screenSpan.x == 0 || metrics.screenSpan.y == 0)
		{
			return;
		}

		pipeline->RALPipeline().RenderLayers(metrics, scene);

		now = Time::TickCount();

		currentWindowBacking->Present();

		presentCost = Time::TickCount() - now;

		DetachContext();

		now = Time::TickCount();
		frameTime = now - lastTick;
		lastTick = now;

		frameIndex++;
	}

	void ResizeBuffers()
	{
		if (currentWindowBacking)
		{
			currentWindowBacking->ResetOutputBuffersForWindow();
		}
	}

	void OnWindowResized(IDX11WindowBacking&, Vec2i span) override
	{
		if (span.x > 0 && span.y > 0)
		{
			ResizeBuffers();
		}
	}

	void SetCursorVisibility(bool isVisible) override
	{
		if (currentWindowBacking)
		{
			Rococo::OS::SetCursorVisibility(isVisible, currentWindowBacking->Window());
		}
	}

	ID_TEXTURE GetWindowDepthBufferId() const override
	{
		return currentWindowBacking ? currentWindowBacking->DepthBufferId() : ID_TEXTURE::Invalid();
	}

	void RegisterSubsystem(ISubsystemMonitor& monitor, ID_SUBSYSTEM platformId) override
	{
		auto rendererId = monitor.Register(*this, platformId);
		pipeline->RegisterSubsystem(monitor, rendererId);
	}

	[[nodiscard]] cstr SubsystemName() const override
	{
		return "DX11AppRenderer";
	}

	IReflectionTarget* ReflectionTarget()
	{
		return this;
	}

	void Visit(IReflectionVisitor& v) override
	{
		Section renderer(v, "DX11AppRenderer");

		{
			Container container(v, "adapters");

			for (UINT i = 0; i < 10; i++)
			{
				IDXGIAdapter* adapter = nullptr;
				factory.EnumAdapters(i, &adapter);
				if (!adapter)
				{
					break;
				}

				DXGI_ADAPTER_DESC desc;
				adapter->GetDesc(&desc);

				EnterElement(v, "Adapter #%u", i);

				ReflectStackFormat(v, "Description", "%ws", desc.Description);
				ReflectStackFormat(v, "DedicatedSystemMemory", "%llu MB", desc.DedicatedSystemMemory / 1_megabytes);
				ReflectStackFormat(v, "DedicatedVideoMemory", "%llu MB", desc.DedicatedVideoMemory / 1_megabytes);
				ReflectStackFormat(v, "SharedSystemMemory", "%llu MB", desc.SharedSystemMemory / 1_megabytes);

				v.LeaveElement();
			}

		}

		ReflectStackFormat(v, "ScreenSpan", "%d x %d", screenSpan.x, screenSpan.y);
		ReflectStackFormat(v, "AIcost",			"%f ms", Time::ToMilliseconds(AIcost));
		ReflectStackFormat(v, "guiCost",		"%f ms", Time::ToMilliseconds(guiCost));
		ReflectStackFormat(v, "objCost",		"%f ms", Time::ToMilliseconds(objCost));
		ReflectStackFormat(v, "presentCost",	"%f ms", Time::ToMilliseconds(presentCost));
	}
};

namespace Rococo
{
	namespace DX11
	{
		static_assert(sizeof(DepthRenderData) % 16 == 0, "DX11 requires size of DepthRenderData to be multipe of 16 bytes");
		static_assert(sizeof(GlobalState) % 16 == 0, "DX11 requires size of GlobalState to be multipe of 16 bytes");
		static_assert(sizeof(LightConstantBuffer) % 16 == 0, "DX11 requires size of Light to be multipe of 16 bytes");

		IDX11Renderer* CreateDX11Renderer(Factory& factory, IShaderOptions& options)
		{
			auto* renderer = new DX11AppRenderer(factory, options);
			return renderer;
		}
	}
}

