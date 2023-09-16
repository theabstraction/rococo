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

#include <RAL\RAL.h>

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
using namespace Rococo::Fonts;
using namespace Rococo::Windows;
using namespace Rococo::Samplers;
using namespace Rococo::DX11;

using namespace Rococo::Textures;

ROCOCO_INTERFACE IDX11IRALVertexDataBuffer : IRALVertexDataBuffer
{
	virtual ID3D11Buffer* RawBuffer() = 0;
};

class DX11_RAL_DynamicVertexBuffer: public IDX11IRALVertexDataBuffer
{
	ID3D11Device& device;
	ID3D11DeviceContext& dc;
	AutoRelease<ID3D11Buffer> dx11Buffer;
public:
	DX11_RAL_DynamicVertexBuffer(ID3D11Device& _device, ID3D11DeviceContext& _dc, size_t sizeofStruct, size_t nElements): device(_device), dc(_dc)
	{
		size_t nBytes = sizeofStruct * nElements;
		if (nBytes > std::numeric_limits<UINT>::max())
		{
			Throw(E_INVALIDARG, "DX11_RAL_DataBuffer failed - capacity too large");
		}

		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.ByteWidth = (UINT)nBytes;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;
		VALIDATEDX11(device.CreateBuffer(&bufferDesc, nullptr, &dx11Buffer));
	}

	void CopyDataToBuffer(const void* data, size_t sizeofData) override
	{
		DX11::CopyStructureToBuffer(dc, dx11Buffer, data, sizeofData);
	}

	void Free() override
	{
		delete this;
	}

	ID3D11Buffer* RawBuffer() override
	{
		return dx11Buffer;
	}
};

ROCOCO_INTERFACE IDX11IRALConstantDataBuffer : IRALConstantDataBuffer
{
	virtual ID3D11Buffer * RawBuffer() = 0;
};

class DX11_RALConstantBuffer : public IDX11IRALConstantDataBuffer
{
	ID3D11Device& device;
	ID3D11DeviceContext& dc;
	AutoRelease<ID3D11Buffer> dx11Buffer;
public:
	DX11_RALConstantBuffer(ID3D11Device& _device, ID3D11DeviceContext& _dc, size_t sizeofStruct, size_t nElements) : device(_device), dc(_dc)
	{
		size_t nBytes = sizeofStruct * nElements;
		if (nBytes > std::numeric_limits<UINT>::max())
		{
			Throw(E_INVALIDARG, "DX11_RAL_DataBuffer failed - capacity too large");
		}

		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = (UINT) nBytes;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		VALIDATEDX11(device.CreateBuffer(&desc, nullptr, &dx11Buffer));
	}

	void AssignToPS(int constantIndex) override
	{
		dc.PSSetConstantBuffers(constantIndex, 1, &dx11Buffer);
	}

	void AssignToGS(int constantIndex) override
	{
		dc.GSSetConstantBuffers(constantIndex, 1, &dx11Buffer);
	}

	void AssignToVS(int constantIndex) override
	{
		dc.VSSetConstantBuffers(constantIndex, 1, &dx11Buffer);
	}

	void CopyDataToBuffer(const void* data, size_t sizeofData) override
	{
		DX11::CopyStructureToBuffer(dc, dx11Buffer, data, sizeofData);
	}

	void Free() override
	{
		delete this;
	}

	ID3D11Buffer* RawBuffer() override
	{
		return dx11Buffer;
	}
};

class DX11AppRenderer :
	public IDX11Renderer,
	public IRenderContext,
	public IMathsVenue,
	public IDX11ResourceLoader,
	public IRenderingResources,
	public IRAL
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

	std::vector<ID3D11Buffer*> boundVertexBuffers;
	std::vector<UINT> boundVertexBufferStrides;
	std::vector<UINT> boundVertexBufferOffsets;

	AutoFree<IDX11Pipeline> pipeline;

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

	void BindVertexBuffer(IRALVertexDataBuffer* vertexBuffer, size_t sizeofVertex, uint32 offset) override
	{
		if (vertexBuffer == nullptr)
		{
			Throw(0, "%s: vertexBuffer was nullptr", __FUNCTION__);
		}

		boundVertexBuffers.push_back(static_cast<IDX11IRALVertexDataBuffer*>(vertexBuffer)->RawBuffer());
		boundVertexBufferStrides.push_back((uint32) sizeofVertex);
		boundVertexBufferOffsets.push_back(offset);
	}

	void CommitBoundVertexBuffers() override
	{
		if (boundVertexBuffers.size() == 0)
		{
			Throw(0, "%s: boundVertexBuffers size was zero", __FUNCTION__);
		}

		dc.IASetVertexBuffers(0, boundVertexBuffers.size(), boundVertexBuffers.data(), boundVertexBufferStrides.data(), boundVertexBufferOffsets.data());
	}

	void Draw(uint32 nVertices, uint32 startPosition) override
	{
		dc.Draw(nVertices, startPosition);
	}

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

	ID_CUBE_TEXTURE envId;

	void SetEnvironmentMap(ID_CUBE_TEXTURE envId)
	{
		this->envId = envId;
	}

	ID_CUBE_TEXTURE GetEnvMapId() const override
	{
		return envId;
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
		scratchBuffer(CreateExpandingBuffer(64_kilobytes)),
		textureManager(CreateTextureManager(installation, device, dc)),
		meshes(CreateMeshManager(device)),
		shaders(CreateShaderManager(installation, options, device, dc))
	{
		RenderBundle bundle{ installation, *this, *this, *shaders, *textureManager, *meshes, *this, *this, device, dc, *this, *this, *this };

		pipeline = CreateDX11Pipeline(bundle);
		lastTick = Time::TickCount();
	}

	~DX11AppRenderer()
	{
		DetachContext();
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

			RECT rect;
			GetClientRect(currentWindowBacking->Window(), &rect);
			metrics.screenSpan = { rect.right, rect.bottom };
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

		if (metrics.screenSpan.x == 0 || metrics.screenSpan.y == 0)
		{
			return;
		}

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
		static_assert(sizeof(LightConstantBuffer) % 16 == 0, "DX11 requires size of Light to be multipe of 16 bytes");

		IDX11Renderer* CreateDX11Renderer(Factory& factory, IShaderOptions& options)
		{
			auto* renderer = new DX11AppRenderer(factory, options);
			return renderer;
		}
	}
}

