#pragma once

struct IDXGIFactory7;
struct IDXGIAdapter;
struct IDXGIOutput;
struct ID3D12Device6;
struct ID3D12Debug3;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12RootSignature;


namespace Rococo::Graphics
{
	ROCOCOAPI IDX12RendererWindow
	{
		virtual Rococo::Windows::IWindow & Window() = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX12RendererWindowEventHandler
	{
		// While processing a windows message an exception was thrown
		virtual void OnMessageQueueException(IDX12RendererWindow& window, IException& ex) = 0;

		// Triggered when something has requested the window to close (such as ALT+F4 or clicking the top right cross)
		virtual void OnCloseRequested(IDX12RendererWindow & window) = 0;
	};

	struct DX12WindowInternalContext
	{
		IDXGIFactory7& factory;
		IDXGIAdapter& adapter;
		IDXGIOutput& output;
		ID3D12Device6& device;
		ID3D12Debug3& debug;
		ID3D12CommandQueue& q;
		ID3D12CommandAllocator& commandAllocator;
		ID3D12RootSignature& rootSignature;
	};

	struct DX12WindowCreateContext
	{
		const char* title;
		HWND hParentWnd; // a HWND
		HINSTANCE hResourceInstance;
		GuiRect rect;
		IDX12RendererWindowEventHandler& evHandler;
	};

	ROCOCOAPI IDX12RendererFactory
	{
		virtual IDX12RendererWindow * CreateDX12Window(DX12WindowCreateContext & context) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX12FactoryContext
	{
		virtual IDX12RendererFactory* CreateFactory() = 0;
		virtual void ShowAdapterDialog(Windows::IWindow&  hParentWnd) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IDX12ResourceResolver
	{
		virtual void ConvertShortnameToPath(const char* resourceName, wchar_t* sysPath, size_t charsInSysPath) = 0;
		virtual void LoadResource_FreeThreaded(const wchar_t* filename, IEventCallback<const fstring>& onLoad) = 0;
	};

	IDX12FactoryContext* CreateDX12FactoryContext(uint32 adapterIndex, uint32 outputIndex, IDX12ResourceResolver& resolver);
}
