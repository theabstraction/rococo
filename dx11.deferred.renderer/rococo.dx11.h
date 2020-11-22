#pragma once

#include <rococo.renderer.h>

#define VALIDATE_HR(hr) ValidateHR(hr, #hr, __FUNCTION__, __FILE__, __LINE__);

struct IDXGIFactory7;
struct IDXGIAdapter4;
struct IDXGIOutput;
struct ID3D11Device5;
struct ID3D11DeviceContext4;
struct ID3D11RenderTargetView1;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11ShaderResourceView;
struct ID3D11Texture2D1;
struct ID3D11Resource;
struct D3D11_BOX;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11InputLayout;
struct ID3D11Buffer;
enum D3D11_MAP;
struct D3D11_MAPPED_SUBRESOURCE;

namespace Rococo::Textures
{
	struct ITextureArrayBuilderSupervisor;
}

namespace Rococo::Graphics
{
	struct IShaderCache;

	struct AdapterContext
	{
		IDXGIFactory7& f;
		IDXGIAdapter4& adapter;
		IDXGIOutput& output;
	};

	struct LayoutId
	{
		uint32 index = 0;
	};

	enum class HLSL_Semantic
	{
		BINORMAL,
		BLENDINDICES,
		BLENDWEIGHT,
		COLOR,
		NORMAL,
		POSITION,
		POSITIONT,
		PSIZE,
		TANGENT,
		TEXCOORD
	};

	ROCOCOAPI IVertexLayoutBuilder
	{
		virtual void AddRGBAb(uint32 semanticIndex) = 0;
		virtual void AddFloat(HLSL_Semantic semantic, uint32 semanticIndex) = 0;
		virtual void AddFloat2(HLSL_Semantic semantic, uint32 semanticIndex) = 0;
		virtual void AddFloat3(HLSL_Semantic semantic, uint32 semanticIndex) = 0;
		virtual void AddFloat4(HLSL_Semantic semantic, uint32 semanticIndex) = 0;
		virtual void Clear() = 0;
		virtual LayoutId Commit(const fstring& name) = 0;
		virtual void SetInputSlot(uint32 slot) = 0;
	};

	ROCOCOAPI IVertexLayouts
	{
		virtual [[nodiscard]] IVertexLayoutBuilder & GetBuilder() = 0;
		virtual [[nodiscard]] bool SetInputLayout(LayoutId layoutIndex) = 0;
		virtual [[nodiscard]] bool IsValid(LayoutId layoutIndex) const = 0;
		virtual void CreateLayout(LayoutId id, const void* shaderSignature, size_t lenBytes) = 0;
	};

	ROCOCOAPI IVertexLayoutBuilderSupervisor : public IVertexLayouts
	{
		virtual void Free() = 0;
	};

	enum class MeshType : uint32
	{
		DynamicVertex,
		ImmutableVertex,
		DynamicConstant,
		ImmutableConstant
	};

	struct MeshIndex
	{
		uint32 index : 30;
		MeshType type : 2;
	};

	static_assert(sizeof(MeshIndex) == sizeof(uint32));

	ROCOCOAPI IMeshPopulator
	{
		virtual void Clear() = 0;
		virtual void Reserve(size_t sizeofVertex, size_t nVertices) = 0;
		virtual void AddVertex(const void* pData, size_t sizeofVertex) = 0;
		virtual MeshIndex CommitDynamicVertexBuffer(const fstring& name, const fstring& vertexLayout) = 0;
		virtual void UpdateDynamicConstantBufferByByte(MeshIndex id, const void* pData, size_t sizeofBuffer) = 0;
		virtual void UpdateDynamicVertexBufferByByte(MeshIndex id, const void* data, size_t lenBytes) = 0;
		virtual MeshIndex CommitImmutableVertexBuffer(const fstring& name, const fstring& vertexLayout) = 0;
		virtual MeshIndex CommitDynamicConstantBuffer(const fstring& name, const void* data, size_t lenBytes) = 0;
		virtual MeshIndex CommitImmutableConstantBuffer(const fstring& name) = 0;
		virtual void SetStride(size_t sizeofVertex) = 0;
		virtual IVertexLayoutBuilder& LayoutBuilder() = 0;

		template<class T> void Add(const T& t)
		{
			AddVertex(&t, sizeof(t));
		}

		template<class T> MeshIndex CommitDynamicVertexBuffer(const fstring& name, const fstring& vertexLayout)
		{
			SetStride(sizeof(T));
			return CommitDynamicVertexBuffer(name, vertexLayout);
		}

		template<class T> void UpdateDynamicConstantBuffer(MeshIndex id, const T& t)
		{
			UpdateDynamicConstantBufferByByte(id, &t, sizeof(T));
		}

		template<class T> void UpdateDynamicVertexBuffer(MeshIndex id, const T* t, size_t nElements)
		{
			UpdateDynamicVertexBufferByByte(id, t, sizeof(T) * nElements);
		}
	};

	ROCOCOAPI IWin32AdapterContext
	{
		virtual AdapterContext& AC() = 0;
		virtual void Free() = 0;
	};

	IWin32AdapterContext* CreateWin32AdapterContext(int32 adapterIndex, int32 outputIndex);
	
	struct IPipelineSupervisor;

	ROCOCOAPI IDX11Window
	{
		// Monitor and report shader errors. The cache reference must be valid for the monitor duration
		// If null is passed monitoring ends.
		virtual void MonitorShaderErrors(IShaderCache* cache) = 0;
		virtual Rococo::Windows::IWindow& Window() = 0;
		virtual void UpdateFrame() = 0;
		virtual void Free() = 0;
	};

	enum class TextureType : uint32
	{
		None,
		T2D,
		T2D_Array,
		T2D_UVAtlas,
		DepthStencil
	};

	struct TextureId
	{
		uint32 index : 24 = 0;
		TextureType type : 4 = TextureType::None;
		uint32 unused : 4 = 0;

		operator bool() const { return index != 0;  }
	};

	ROCOCOAPI IDX1RendererWindowEventHandler
	{
		virtual void OnCloseRequested(IDX11Window & window) = 0;
		virtual void OnMessageQueueException(IDX11Window& window, IException& ex) = 0;
		virtual void OnKeyboardEvent(const RAWKEYBOARD& k, HKL hKeyboardLayout) = 0;
		virtual void OnMouseEvent(const RAWMOUSE& m) = 0;
		virtual void OnResizeBackBuffer(TextureId idBackBuffer, Vec2i span) = 0;
		virtual void OnUpdateFrame(TextureId idBackBuffer) = 0;
	};

	struct DX11WindowContext
	{
		Windows::IWindow& parent;
		IDX1RendererWindowEventHandler& evHandler;
		Vec2i windowedSpan;
		cstr title;
		void* hResourceInstance;
	};

	static_assert(sizeof(TextureId) == sizeof(uint32));

	struct RenderTargetFlags
	{
		uint32 clearWhenAssigned : 1;
	};

	struct RenderTarget
	{
		TextureId id;
		uint32 mipMapIndex;
		RenderTargetFlags flags;
		RGBA clearColour{ 0, 0, 0, -1 };
	};

	ROCOCOAPI ITextureBuilder
	{
		virtual TextureId AddTx2D_Grey(cstr name) = 0;
		virtual TextureId AddTx2DArray_Grey(cstr name, Vec2i span) = 0;
		virtual TextureId AddTx2D_RGBAb(cstr name) = 0;
		virtual TextureId AddTx2DArray_RGBAb(cstr name, Vec2i span) = 0;
		virtual TextureId AddTx2D_UVAtlas(cstr name) = 0;
		virtual TextureId AddDepthStencil(cstr name, Vec2i span, uint32 depthBits, uint32 stencilBits) = 0;
		virtual int32 AddElementToArray(TextureId, cstr name) = 0;
		virtual void EnableMipMapping(TextureId id) = 0;
	};

	ROCOCOAPI ITextureCache : ITextureBuilder
	{
		virtual bool AssignTextureToShaders(TextureId id, uint32 textureUnit) = 0;
		virtual void ClearDepthBuffer(TextureId id, float depth, uint8 stencilBits) = 0;
		virtual void ClearRenderTarget(TextureId id, const RGBA& clearColour) = 0;
		virtual Vec2i GetSpan(TextureId id) const = 0;
		virtual void ReloadAsset(TextureId id) = 0;
		virtual void UpdateSpanFromSystem(TextureId id) = 0;
		virtual void UpdateArray(TextureId id, uint32 index, const GRAYSCALE* pixels, Vec2i span) = 0;
		virtual	void UseTexturesAsRenderTargets(const RenderTarget* targets, uint32 nTargets, TextureId idDepthStencil) = 0;	
	};

	ROCOCOAPI IDX11TextureCache : ITextureCache
	{
		virtual TextureId AddTx2D_Direct(cstr name, ID3D11Texture2D1* tx2D) = 0;
	};

	ROCOCOAPI ITextureSupervisor : IDX11TextureCache
	{
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX11DeviceContext
	{
		virtual void UpdateSubresource(ID3D11Resource& resource, uint32 subresourceIndex, const D3D11_BOX& box, const void* pixels, uint32 lineSpan, uint32 srcDepth) = 0;
		virtual void OMSetRenderTargets(uint32 NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView) = 0;
		virtual void PSSetShaderResources(uint32 StartSlot, uint32 NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) = 0;
		virtual void VSSetShaderResources(uint32 StartSlot, uint32 NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) = 0;
		virtual void ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView, const RGBA& colour) = 0;
		virtual void ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView, uint32 ClearFlags, float Depth, uint8 Stencil) = 0;
		virtual void OMSetBlendState(ID3D11BlendState* pBlendState, const RGBA& blendFactor, uint32 sampleMask) = 0;
		virtual void OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, uint32 StencilRef) = 0;
		virtual void RSSetScissorRects(uint32 NumRects, const RECT* pRects) = 0;
		virtual void RSSetState(ID3D11RasterizerState* pRasterizerState) = 0;
		virtual void IASetInputLayout(ID3D11InputLayout* pInputLayout) = 0;
		virtual void GenerateMips(ID3D11ShaderResourceView* pShaderResourceView) = 0;
		virtual void IASetVertexBuffers(uint32 StartSlot, uint32 NumBuffers, ID3D11Buffer* const* ppVertexBuffers, const uint32* pStrides, const uint32* pOffsets) = 0;
		virtual HRESULT Map(ID3D11Resource* pResource, uint32 Subresource, D3D11_MAP MapType, uint32 MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource) = 0;
		virtual void Unmap(ID3D11Resource* pResource, uint32 Subresource) = 0;
	};

	ROCOCOAPI IPainter
	{
		virtual void Draw(uint32 vertexCount, uint32 startLocation) = 0;
	};

	ROCOCOAPI IRenderPhasePopulator
	{
		virtual void RenderStage(IPainter & painter) = 0;
		virtual void SetScene(IScene* scene) = 0;
		virtual void Free() = 0;
	};

	ITextureSupervisor* CreateTextureCache(IInstallation& installation, ID3D11Device5& device, IDX11DeviceContext& dc);

	ROCOCOAPI IMeshCache
	{
		virtual [[nodiscard]] IMeshPopulator & GetPopulator() = 0;
		virtual [[nodiscard]] bool UseAsVertexBufferSlot(MeshIndex id, uint32 slot) = 0;
		virtual IVertexLayouts& Layouts() = 0;
		virtual void Free() = 0;
	};

	IMeshCache* CreateMeshCache(ID3D11Device5& device, IDX11DeviceContext& dc);

	ROCOCOAPI IDX11System
	{
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
		virtual ITextureCache & Textures() = 0;
		virtual IShaderCache& Shaders() = 0;
		virtual IDX11Window * CreateDX11Window(DX11WindowContext& context) = 0;
		virtual ID3D11Device5& Device() = 0;
		virtual IDX11DeviceContext& DC() = 0;
		virtual IDXGIFactory7& Factory() = 0;
		virtual IMeshCache& Meshes() = 0;
		virtual void Free() = 0;
		virtual IPainter& Painter() = 0;
		virtual bool UseShaders(LayoutId layoutId, ID_VERTEX_SHADER idVS, ID_PIXEL_SHADER idPS) = 0;
	};

	IDX11System* CreateDX11System(AdapterContext& ac, IInstallation& installation);

	void ValidateHR(HRESULT hr, const char* badcall, const char* function, const char* file, int lineNumber);

	// D3D11_COMPARISON_FUNC
	enum class ComparisonFunc
	{
		NEVER = 1,
		LESS = 2,
		EQUAL = 3,
		LESS_EQUAL = 4,
		GREATER = 5,
		NOT_EQUAL = 6,
		GREATER_EQUAL = 7,
		ALWAYS = 8
	};

	// D3D11_BLEND
	enum class BlendValue : uint32
	{
		ZERO = 1,
		ONE = 2,
		SRC_COLOR = 3,
		INV_SRC_COLOR = 4,
		SRC_ALPHA = 5,
		INV_SRC_ALPHA = 6,
		DEST_ALPHA = 7,
		INV_DEST_ALPHA = 8,
		DEST_COLOR = 9,
		INV_DEST_COLOR = 10,
		SRC_ALPHA_SAT = 11,
		BLEND_FACTOR = 14,
		INV_BLEND_FACTOR = 15,
		SRC1_COLOR = 16,
		INV_SRC1_COLOR = 17,
		SRC1_ALPHA = 18,
		INV_SRC1_ALPHA = 19
	};

	// D3D11_BLEND_OP
	enum class BlendOp : uint32
	{
		ADD = 1,
		SUBTRACT = 2,
		REV_SUBTRACT = 3,
		MIN = 4,
		MAX = 5
	};

	// D3D11_FILTER
	enum class SamplerFilter : uint32
	{
		MIN_MAG_MIP_POINT = 0,
		MIN_MAG_POINT_MIP_LINEAR = 0x1,
		MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4,
		MIN_POINT_MAG_MIP_LINEAR = 0x5,
		MIN_LINEAR_MAG_MIP_POINT = 0x10,
		MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
		MIN_MAG_LINEAR_MIP_POINT = 0x14,
		MIN_MAG_MIP_LINEAR = 0x15,
		ANISOTROPIC = 0x55,
		COMPARISON_MIN_MAG_MIP_POINT = 0x80,
		COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81,
		COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84,
		COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85,
		COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90,
		COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
		COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94,
		COMPARISON_MIN_MAG_MIP_LINEAR = 0x95,
		COMPARISON_ANISOTROPIC = 0xd5,
		MINIMUM_MIN_MAG_MIP_POINT = 0x100,
		MINIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x101,
		MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x104,
		MINIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x105,
		MINIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x110,
		MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x111,
		MINIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x114,
		MINIMUM_MIN_MAG_MIP_LINEAR = 0x115,
		MINIMUM_ANISOTROPIC = 0x155,
		MAXIMUM_MIN_MAG_MIP_POINT = 0x180,
		MAXIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x181,
		MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x184,
		MAXIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x185,
		MAXIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x190,
		MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x191,
		MAXIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x194,
		MAXIMUM_MIN_MAG_MIP_LINEAR = 0x195,
		MAXIMUM_ANISOTROPIC = 0x1d5
	};

	enum class SamplerAddressMode : uint32
	{
		WRAP = 1,
		MIRROR = 2,
		CLAMP = 3,
		BORDER = 4,
		MIRROR_ONCE = 5
	};

	struct Sampler
	{
		SamplerFilter filter;
		SamplerAddressMode U;
		SamplerAddressMode V;
		SamplerAddressMode W;
		ComparisonFunc comparison;
		uint32 maxAnisotropy;
		float mipLODBias;
		float maxLOD;
		float minLOD;
	};

	ROCOCOAPI IRenderStage
	{
		virtual void AddDepthStencilBuffer(TextureId id, float clearDepth, uint8 stencilBits) = 0;
		virtual void AddInput(TextureId id, uint32 textureUnit, const Sampler & sampler) = 0;
		virtual void AddOutput(TextureId id, uint32 renderTargetIndex, uint32 mipMapIndex, RenderTargetFlags flags, const RGBA& clearColour) = 0;
		virtual void SetEnableDepth(bool isEnabled) = 0;
		virtual void SetDepthComparison(ComparisonFunc func) = 0;
		virtual void SetDepthWriteEnable(bool isEnabled) = 0;
		virtual void ValidateUnit(cstr function, uint32 index) = 0;
		virtual void SetEnableScissors(const GuiRect* pRect) = 0;
		virtual void SetEnableMultisample(bool isEnabled) = 0;
		virtual void SetEnableDepthClip(bool isEnabled) = 0;
		virtual void SetWireframeRendering(bool isEnabled) = 0;
		virtual void SetCullMode(int32 direction) = 0;
		virtual void SetBlendOp(BlendOp op, uint32 unit = 0) = 0;
		virtual void SetBlendAlphaOp(BlendOp op, uint32 unit = 0) = 0;
		virtual void SetEnableBlend(BOOL bEnable, uint32 unit = 0) = 0;
		virtual void SetSrcAlphaBlend(BlendValue value, uint32 unit = 0) = 0;
		virtual void SetDestAlphaBlend(BlendValue value, uint32 unit = 0) = 0;
		virtual void SetSrcBlend(BlendValue value, uint32 unit = 0) = 0;
		virtual void SetDestBlend(BlendValue value, uint32 unit = 0) = 0;
	};

	ROCOCOAPI IRenderStageSupervisor : public IRenderStage
	{
		virtual void Execute() = 0;
		virtual void SetPopulator(IRenderPhasePopulator* populator) = 0;
		virtual uint32 AddRef() = 0;
		virtual uint32 Release() = 0;
	};

	IRenderStageSupervisor* CreateRenderStageBasic(IDX11System& system);

	ROCOCOAPI IPipelineBuilder
	{
		virtual void AddStage(cstr friendlyName, IRenderStageSupervisor * stage) = 0;
		virtual void Clear() = 0;
	};

	ROCOCOAPI IPipelineSupervisor
	{
		virtual void Execute() = 0;
		virtual IPipelineBuilder& GetBuilder() = 0;
		virtual void Free() = 0;
	};

	IPipelineSupervisor* CreatePipeline(IDX11System& system);

	IRenderPhasePopulator* CreateStandardGuiRenderPhase(IDX11System& system, IRenderStage& stage);
}
