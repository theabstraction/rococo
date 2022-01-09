#pragma once

#include "rococo.renderer.h"

struct IDXGIFactory7;
struct IDXGIAdapter4;
struct IDXGIOutput;
struct ID3D12Device6;
struct ID3D12Debug3;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12RootSignature;
struct D3D12_GRAPHICS_PIPELINE_STATE_DESC;
struct ID3D12PipelineState;
struct D3D12_INPUT_LAYOUT_DESC;
struct ID3D12Resource;

namespace Rococo
{
	struct ID_PIXEL_SHADER;
	struct ID_VERTEX_SHADER;

	struct IRenderer;
	struct SkyVertex;
	struct ObjectVertex;
	struct BoneWeights;
}

namespace Rococo::Graphics
{
	enum class TextureInternalFormat
	{
		Greyscale_R8,
		RGBAb
	};

	D3D12_INPUT_LAYOUT_DESC GuiLayout();
	void InitGuiPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);

	struct DX12WindowInternalContext
	{
		IDXGIFactory7& factory;
		IDXGIAdapter4& adapter;
		IDXGIOutput& output;
		ID3D12Device6& device;
		ID3D12Debug3& debug;
		ID3D12CommandQueue& q;
		ID3D12CommandAllocator& commandAllocator;
		ID3D12RootSignature& rootSignature;
		const uint32 adapterIndex;
	};

	struct ShaderView
	{
		cstr resourceName;
		int hr;
		cstr errorString;
		const void* blob;
		size_t blobCapacity;
	};

	ROCOCOAPI IDX12TextureTable
	{
		virtual ID_TEXTURE LoadTexture(IBuffer & buffer, cstr uniqueName) = 0;
		virtual void Free() = 0;
	};

	IDX12TextureTable* CreateTextureTable(IInstallation& installation, DX12WindowInternalContext& ic);

	ROCOCOAPI IDX12TextureArray
	{
		virtual void Free() = 0;
		virtual int MaxWidth() const = 0;
		virtual int TextureCount() const = 0;
		virtual void SetDimensions(int32 width, int32 height, int nElements) = 0;
		virtual void WriteSubImage(int32 index, const GRAYSCALE* pixels, int32 width, int32 height) = 0;
		virtual void WriteSubImage(int32 index, const RGBAb* pixels, const GuiRect& rect) = 0;
	};

	ROCOCOAPI ILoadEvent
	{
		virtual void OnGreyscale(const GRAYSCALE * alphaPixels, Vec2i span) = 0;
		virtual void OnRGBAb(const RGBAb* colourPixels, Vec2i span) = 0;
	};

	ROCOCOAPI ITextureLoader
	{
		virtual bool TryLoad(cstr resourceName, ILoadEvent & onLoad) = 0;
		virtual void Free() = 0;
	};

	ITextureLoader* CreateTiffLoader(IInstallation& installation);
	ITextureLoader* CreateJPEGLoader(IInstallation& installation);

	struct TextureMetaData
	{
		cstr name = nullptr;
		Vec2i span = { 0,0 };
		TextureInternalFormat format = TextureInternalFormat::Greyscale_R8;
	};

	struct TextureRecordData
	{
		TextureMetaData meta;
		GRAYSCALE* alphaPixels = nullptr;
		RGBAb* colourPixels = nullptr;
	};

	// A const char* to a string array that is valid and immutable for the duration of the the application
	typedef const char* CompileTimeStringConstant;

	ROCOCOAPI ITextureMemory
	{
		virtual void Commit(const TextureRecordData& data) = 0;
		virtual ID3D12Resource* Commit2DArray(CompileTimeStringConstant friendlyName, int width, int height, int nElements, TextureInternalFormat format, bool isMipMapped) = 0;
		virtual void Free() = 0;
	};

	ITextureMemory* Create_MPlat_Standard_TextureMemory(DX12WindowInternalContext& ic);

	ROCOCOAPI IDX12MeshBuffers
	{
		virtual ID_SYS_MESH CreateSkyMesh(const SkyVertex * vertices, uint32 nVertices) = 0;
		virtual ID_SYS_MESH CreateTriangleMesh(const ObjectVertex * vertices, uint32 nVertices, const BoneWeights * weights) = 0;
		virtual void DeleteMesh(ID_SYS_MESH id) = 0;
		virtual void GetDesc(char desc[256], ID_SYS_MESH id) = 0;
		virtual void Clear() = 0;
		virtual void Free() = 0;
		virtual void SetShadowCasting(ID_SYS_MESH id, boolean32 isActive) = 0;
		virtual void UpdateMesh(ID_SYS_MESH id, const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) = 0;
	};

	ROCOCOAPI IDX12MaterialList
	{
		virtual MaterialId GetMaterialId(cstr name) const = 0;
	    virtual cstr GetMaterialTextureName(MaterialId id) const = 0;
		virtual void GetMaterialArrayMetrics(MaterialArrayMetrics& metrics) const = 0;
		virtual void LoadMaterialTextureArray(IMaterialTextureArrayBuilder& builder) = 0;
		virtual void Free() = 0;
	};
	IDX12MaterialList* CreateMaterialList(DX12WindowInternalContext& ic, ITextureMemory& txMemory, IInstallation& installation);

	IDX12MeshBuffers* CreateMeshBuffers(DX12WindowInternalContext& ic);

	struct DX12TextureArraySpec
	{
		ITextureMemory& txMemory;
	};

	IDX12TextureArray* CreateDX12TextureArray(CompileTimeStringConstant friendlyName, bool isMipMapped, DX12TextureArraySpec& spec, DX12WindowInternalContext& ic);

	// N.B the shader thread is locked until OnGrab returns
	// and the contents may change, so copy what you need and do not block within the method
	// A correct implementation should generally allow an exception within the handler
	ROCOCOAPI IShaderViewGrabber
	{
		// N.B the shader thread is locked until OnGrab returns
		// and the contents may change, so copy what you need and do not block within the method
		virtual void OnGrab(const ShaderView & view) = 0;
	};

	ROCOCOAPI IShaderCache
	{
		virtual	ID_PIXEL_SHADER AddPixelShader(const char* resourceName) = 0;
		virtual ID_VERTEX_SHADER AddVertexShader(const char* resourceName) = 0;
		virtual void GrabShaderObject(ID_PIXEL_SHADER pxId, IShaderViewGrabber& grabber) = 0;
		virtual void GrabShaderObject(ID_VERTEX_SHADER vxId, IShaderViewGrabber& grabber) = 0;
		virtual void GrabShaderObject(const char* resourceName, IShaderViewGrabber& grabber) = 0;
		virtual void ReloadShader(const char* resourceName) = 0;
		virtual uint32 InputQueueLength() = 0;
		virtual bool TryGrabAndPopNextError(IShaderViewGrabber& grabber) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI ITextureManager
	{
		virtual void AddTextureLoader(ITextureLoader * loader) = 0;
		virtual ID_TEXTURE Bind(cstr resourceName, TextureInternalFormat format) = 0;
		virtual void Reload(cstr resourceName) = 0;
		virtual TextureMetaData& GetById(ID_TEXTURE id) = 0;
		virtual TextureMetaData& TryGetById(ID_TEXTURE id) noexcept = 0;
		virtual void Commit(ID_TEXTURE id, ITextureMemory& memory) = 0;
		virtual void Free() = 0;
	};

	ITextureManager* CreateTextureManager(IInstallation& installation);

	ROCOCOAPI IDX12RendererWindow
	{
		virtual void WaitForNextRenderAndDisplay(cstr message) = 0;
		virtual void SetText(cstr message) = 0;
		virtual void ShowWindowVenue(IMathsVisitor & visitor) = 0;
		virtual Rococo::Windows::IWindow & Window() = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX12Renderer
	{
		virtual IRenderer & Renderer() = 0;
		virtual void SetTargetWindow(IDX12RendererWindow* window) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IPipelineBuilder
	{
		virtual void Free() = 0;
		virtual const char* LastError() const = 0;
		virtual int SetShaders(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, ID_VERTEX_SHADER vsId, ID_PIXEL_SHADER psId) = 0;
		virtual ID3D12PipelineState* CreatePipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) = 0;
	};

	IDX12Renderer* CreateDX12Renderer(IInstallation& installation, DX12WindowInternalContext& ic, ITextureMemory& txMemory, IShaderCache& shaders, IPipelineBuilder& pipelineBuilder);

	ROCOCOAPI IDX12RendererWindowEventHandler
	{
		// While processing a windows message an exception was thrown
		virtual void OnMessageQueueException(IDX12RendererWindow& window, IException& ex) = 0;

		// Triggered when something has requested the window to close (such as ALT+F4 or clicking the top right cross)
		virtual void OnCloseRequested(IDX12RendererWindow & window) = 0;
	};

	IPipelineBuilder* CreatePipelineBuilder(DX12WindowInternalContext& ic, IShaderCache& shaders);

	struct DX12WindowCreateContext
	{
		const char* title;
		void* hParentWnd; // a HWND
		void* hResourceInstance;
		GuiRect rect;
		IDX12RendererWindowEventHandler& evHandler;
	};

	ROCOCOAPI IDX12RendererFactory
	{
		virtual DX12WindowInternalContext& IC() = 0;
		virtual IDX12RendererWindow * CreateDX12Window(DX12WindowCreateContext & context) = 0;
		virtual IShaderCache& Shaders() = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX12FactoryContext
	{
		virtual IDX12RendererFactory* CreateFactory(uint64 required_VRAM) = 0;
		virtual void ShowAdapterDialog(Windows::IWindow&  hParentWnd) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX12ResourceResolver
	{
		virtual void ConvertResourceNameToPath(const char* resourceName, wchar_t* sysPath, size_t charsInSysPath) = 0;
		virtual void LoadResource_FreeThreaded(const wchar_t* filename, IEventCallback<const fstring>& onLoad) = 0;
	};

	IDX12FactoryContext* CreateDX12FactoryContext(uint32 adapterIndex, uint32 outputIndex, IDX12ResourceResolver& resolver);
	IDX12RendererWindow* CreateDX12Window(DX12WindowInternalContext& ic, DX12WindowCreateContext& context);
	IShaderCache* CreateShaderCache(IDX12ResourceResolver& resolver);
}
