#include "dx11.renderer.h"
#include <rococo.renderer.h>

#include <vector>
#include <algorithm>

#include <rococo.imaging.h>
#include <rococo.fonts.h>
#include <rococo.maths.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.time.h>

#include <rococo.hashtable.h>

#include <rococo.win32.rendering.h>

#include "dx11helpers.inl"
#include "dx11buffers.inl"

#include "rococo.visitors.h"

#include <Dxgi1_3.h>
#include <comdef.h>

#include "dx11.factory.h"

#include <memory>

#include <rococo.fonts.hq.h>

namespace Rococo::DX11
{
	void ShowWindowVenue(HWND hWnd, IMathsVisitor& visitor);
}

using namespace Rococo;
using namespace Rococo::Fonts;
using namespace Rococo::Windows;
using namespace Rococo::Samplers;
using namespace Rococo::DX11;

using namespace Rococo::Textures;

class DX11AppRenderer :
	public IDX11Renderer,
	public IRenderContext,
	public IMathsVenue,
	public IDX11ResourceLoader
{
private:
	IO::IInstallation& installation;
	ID3D11Device& device;
	ID3D11DeviceContext& dc;
	IDXGIFactory& factory;
	AutoFree<IExpandingBuffer> scratchBuffer;

	AutoFree<IDX11TextureManager> textureManager;
	AutoFree<IDX11Meshes> meshes;
	AutoFree<IDX11Shaders> shaders;

	IDX11WindowBacking* currentWindowBacking = nullptr;

	Time::ticks lastTick;

	RAWMOUSE lastMouseEvent;
	Vec2i screenSpan;

	AutoFree<IDX11Pipeline> pipeline;

	bool IsFullscreen() override
	{
		return currentWindowBacking ? currentWindowBacking->IsFullscreen() : false;
	}

	void SwitchToFullscreen() override
	{
		if (currentWindowBacking) currentWindowBacking->SwitchToFullscreen();
	}

	void SetSampler(uint32 index, Samplers::Filter filter, Samplers::AddressMode u, Samplers::AddressMode v, Samplers::AddressMode w, const RGBA& borderColour) override
	{
		pipeline->SetSampler(index, filter, u, v, w, borderColour);
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

	DX11AppRenderer(DX11::Factory& _factory) :
		installation(_factory.installation), 
		device(_factory.device), dc(_factory.dc), factory(_factory.factory),
		scratchBuffer(CreateExpandingBuffer(64_kilobytes)),
		textureManager(CreateTextureManager(installation, device, dc)),
		meshes(CreateMeshManager(device)),
		shaders(CreateShaderManager(installation, device, dc)),
		pipeline(CreateDX11Pipeline(installation, *this, *this, *shaders, *textureManager, *meshes, *this, *this, device, dc))
	{
		lastTick = Time::TickCount();
	}

	~DX11AppRenderer()
	{
		DetachContext();
	}

	IGuiResources& Gui() override
	{
		return pipeline->Gui();
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

	virtual IRenderer& Renderer() override
	{
		return *this;
	}

	void ExpandViewportToEntireTexture(ID_TEXTURE depthId) override
	{
		auto depth = textureManager->GetTexture(depthId).texture;

		D3D11_TEXTURE2D_DESC desc;
		depth->GetDesc(&desc);

		D3D11_VIEWPORT viewport = { 0 };
		viewport.Width = FLOAT(desc.Width);
		viewport.Height = FLOAT(desc.Height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		dc.RSSetViewports(1, &viewport);

		screenSpan.x = (int32)viewport.Width;
		screenSpan.y = (int32)viewport.Height;
	}

	ID3D11RenderTargetView* BackBuffer()
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
			metrics.screenSpan = screenSpan;
		}
		else
		{
			metrics.cursorPosition = Vec2i{ -1, -1 };
			metrics.screenSpan = Vec2i{ 1, 1 };
		}
	}

	void Draw(ID_SYS_MESH id, const ObjectInstance* instances, uint32 nInstances) override
	{
		auto& m = meshes->GetBuffer(id);
		pipeline->Draw(m, instances, nInstances);
	}

	Windows::IWindow& CurrentWindow() override
	{
		return currentWindowBacking ? currentWindowBacking->Window() : Rococo::Windows::NoParent();
	}

	int64 AIcost = 0;
	int64 guiCost = 0;
	int64 objCost = 0;
	int64 presentCost = 0;
	int64 frameTime = 0;

	IParticles& Particles()
	{
		return pipeline->Particles();
	}

	void Render(Graphics::ENVIRONMENTAL_MAP envMap, IScene& scene) override
	{
		if (!BackBuffer())
		{
			return;
		}

		auto now = Time::TickCount();
		AIcost = now - lastTick;

		GuiMetrics metrics;
		GetGuiMetrics(metrics);
		pipeline->Render(metrics, envMap, scene);

		now = Time::TickCount();

		currentWindowBacking->Present();

		presentCost = Time::TickCount() - now;

		DetachContext();

		now = Time::TickCount();
		frameTime = now - lastTick;
		lastTick = now;
	}

	void ResizeBuffers()
	{
		if (currentWindowBacking)
		{
			currentWindowBacking->ResetOutputBuffersForWindow();
		}
	}

	void OnWindowResized(IDX11WindowBacking&, Vec2i) override
	{
		ResizeBuffers();
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
};

namespace Rococo
{
	namespace DX11
	{
		static_assert(sizeof(DepthRenderData) % 16 == 0, "DX11 requires size of DepthRenderData to be multipe of 16 bytes");
		static_assert(sizeof(GlobalState) % 16 == 0, "DX11 requires size of GlobalState to be multipe of 16 bytes");
		static_assert(sizeof(Light) % 16 == 0, "DX11 requires size of Light to be multipe of 16 bytes");

		IDX11Renderer* CreateDX11Renderer(Factory& factory)
		{
			auto* renderer = new DX11AppRenderer(factory);
			return renderer;
		}
	}
}

