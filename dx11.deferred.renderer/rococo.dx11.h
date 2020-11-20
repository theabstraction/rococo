#pragma once

#include <rococo.renderer.h>

#define VALIDATE_HR(hr) ValidateHR(hr, #hr, __FUNCTION__, __FILE__, __LINE__);

struct IDXGIFactory7;
struct IDXGIAdapter4;
struct IDXGIOutput;
struct ID3D11Device5;
struct ID3D11DeviceContext4;
struct ID3D11RenderTargetView1;

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
	};

	ROCOCOAPI IVertexLayoutBuilderSupervisor : public IVertexLayouts
	{
		virtual void CreateLayout(LayoutId id, const void* shaderSignature, size_t lenBytes) = 0;
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
		virtual void UpdateDynamicConstantBuffer(MeshIndex id, const void* pData, size_t sizeofBuffer) = 0;
		virtual MeshIndex CommitImmutableVertexBuffer(const fstring& name, const fstring& vertexLayout) = 0;
		virtual MeshIndex CommitDynamicConstantBuffer(const fstring& name, const void* data, size_t lenBytes) = 0;
		virtual MeshIndex CommitImmutableConstantBuffer(const fstring& name) = 0;
		virtual void SetStride(size_t sizeofVertex) = 0;

		template<class T> void Add(const T& t)
		{
			AddVertex(&t, sizeof(t));
		}

		template<class T> void SetVertexType()
		{
			SetStride(sizeof(T));
		}

		template<class T> void UpdateDynamicConstantBuffer(MeshIndex id, const T& t)
		{
			UpdateDynamicConstantBuffer(id, &t, sizeof(T));
		}
	};

	ROCOCOAPI IMeshCache
	{
		virtual [[nodiscard]] IMeshPopulator & GetPopulator() = 0;
		virtual [[nodiscard]] bool UseAsVertexBufferSlot(MeshIndex id, uint32 slot) = 0;
	};

	ROCOCOAPI IWin32AdapterContext
	{
		virtual AdapterContext& AC() = 0;
		virtual void Free() = 0;
	};

	IWin32AdapterContext* CreateWin32AdapterContext(int32 adapterIndex, int32 outputIndex);

	ROCOCOAPI IDX11Window
	{
		// Monitor and report shader errors. The cache reference must be valid for the monitor duration
		// If null is passed monitoring ends.
		virtual void MonitorShaderErrors(IShaderCache* cache) = 0;
		virtual Rococo::Windows::IWindow& Window() = 0;
		virtual void UpdateFrame() = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX1RendererWindowEventHandler
	{
		virtual void OnCloseRequested(IDX11Window & window) = 0;
		virtual void OnMessageQueueException(IDX11Window& window, IException& ex) = 0;
		virtual void OnKeyboardEvent(const RAWKEYBOARD& k, HKL hKeyboardLayout) = 0;
		virtual void OnMouseEvent(const RAWMOUSE& m) = 0;
	};

	struct DX11WindowContext
	{
		Windows::IWindow& parent;
		IDX1RendererWindowEventHandler& evHandler;
		Vec2i windowedSpan;
		cstr title;
		void* hResourceInstance;
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
		virtual void ReloadAsset(TextureId id) = 0;
		virtual bool AssignTextureToShaders(TextureId id, uint32 textureUnit) = 0;
		virtual void ClearRenderTarget(TextureId id, const RGBA& clearColour) = 0;
		virtual	void UseTexturesAsRenderTargets(const RenderTarget* targets, uint32 nTargets, TextureId idDepthStencil) = 0;
		virtual void Free() = 0;
	};

	ITextureCache* CreateTextureCache(IInstallation& installation, ID3D11Device5& device, ID3D11DeviceContext4& dc);

	ROCOCOAPI IDX11System
	{
		virtual ITextureCache & Textures() = 0;
		virtual IShaderCache& Shaders() = 0;
		virtual IDX11Window * CreateDX11Window(DX11WindowContext& context) = 0;
		virtual ID3D11Device5& Device() = 0;
		virtual ID3D11DeviceContext4& DC() = 0;
		virtual IDXGIFactory7& Factory() = 0;
		virtual void Free() = 0;
	};

	IDX11System* CreateDX11System(AdapterContext& ac, IInstallation& installation);

	void ValidateHR(HRESULT hr, const char* badcall, const char* function, const char* file, int lineNumber);

	ROCOCOAPI IRenderStage
	{

	};

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
		ADDRESS_WRAP = 1,
		ADDRESS_MIRROR = 2,
		ADDRESS_CLAMP = 3,
		ADDRESS_BORDER = 4,
		ADDRESS_MIRROR_ONCE = 5
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

	ROCOCOAPI IRenderStageSupervisor : public IRenderStage
	{
		virtual void Execute() = 0;

		virtual uint32 AddRef() = 0;
		virtual uint32 Release() = 0;
	};

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
}
