#include <rococo.os.win32.h>
#include <rococo.window.h>
#include "rococo.dx12.h"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <rococo.auto-release.h>
#include "rococo.dx12.helpers.inl"
#include "rococo.dx12-msoft.h" // If this does not compile update your Windows 10 kit thing

#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
	void ValidateHR(HRESULT hr, const char* badcall, const char* function, const char* file, int lineNumber)
	{
		if FAILED(hr)
		{
			Throw(hr, "Windows call failed: %s.\n%s in\n %s line %d.", badcall, function, file, lineNumber);
		}
	}
}

namespace Rococo::DX12Impl
{
	IDX12RendererWindow* CreateDX12Window(
		DX12WindowInternalContext& internalContext,
		DX12WindowCreateContext& context
	);
}

namespace Rococo::Graphics
{
	void EnumerateAdapters(IDXGIFactory& f, HWND hWndOwner);
}

#ifdef _DEBUG
# define FACTORY_DEBUG_FLAGS DXGI_CREATE_FACTORY_DEBUG
#else
# define FACTORY_DEBUG_FLAGS 0
#endif

namespace ANON // Many debuggers will give us more debug info if we dont use truly anonymous namespaces
{
	class DX12FactoryContext : public IDX12FactoryContext
	{
	private:
		AutoRelease<IDXGIFactory7> f;
		AutoRelease<IDXGIAdapter4> adapter;
		AutoRelease<IDXGIOutput> output;

		DXGI_ADAPTER_DESC3 adapterDesc;
		DXGI_OUTPUT_DESC outputDesc;

		IDX12ResourceResolver& resolver;
	public:
		DX12FactoryContext(uint32 adapterIndex, uint32 outputIndex, IDX12ResourceResolver& ref_resolver):
			resolver(ref_resolver)
		{
			VALIDATE_HR(CreateDXGIFactory2(FACTORY_DEBUG_FLAGS, __uuidof(IDXGIFactory7), (void**)&f));

			AutoRelease<IDXGIAdapter1> adapter1;
			if FAILED(f->EnumAdapters1(adapterIndex, &adapter1))
			{
				VALIDATE_HR(f->EnumAdapters1(0, &adapter1));
			}
			else
			{
				if FAILED(D3D12CreateDevice(adapter1, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr))
				{
					adapter1 = nullptr;
					VALIDATE_HR(f->EnumAdapters1(0, &adapter1));
				}
			}

			if FAILED(adapter1->EnumOutputs(outputIndex, &output))
			{
				VALIDATE_HR(adapter1->EnumOutputs(0, &output));
			}

			VALIDATE_HR(adapter1->QueryInterface(&adapter))

			adapter->GetDesc3(&adapterDesc);
			output->GetDesc(&outputDesc);
		}

		IDX12RendererFactory* CreateFactory() override;

		void ShowAdapterDialog(Rococo::Windows::IWindow& parent) override
		{
			EnumerateAdapters(*f, parent);
		}

		void Free() override
		{
			delete this;
		}
	};

	class DX12RendererFactory : public IDX12RendererFactory, public IDX12RendererWindowEventHandler
	{
	private:
		IDXGIFactory7& factory;
		IDXGIAdapter& adapter;
		IDXGIOutput& output;
		AutoRelease<ID3D12Device6> device;
		AutoRelease<ID3D12Debug3> debug;
		AutoRelease<ID3D12CommandQueue> q;
		AutoRelease<ID3D12CommandAllocator> commandAllocator;
		AutoRelease<ID3DBlob> signature;
		AutoRelease<ID3D12RootSignature> rootSignature;
		IDX12ResourceResolver& resolver;
		AutoFree<IShaderCache> shaderCache;
		AutoFree<IPipelineBuilder> pipelineBuilder;

		void /* IDX12RendererWindowEventHandler */ OnActivate(IDX12RendererWindow* window)
		{
			HWND hWnd = window ? window->Window() : nullptr;
			factory.MakeWindowAssociation(hWnd, 0);
		}

		void /* IDX12RendererWindowEventHandler */ OnCloseRequested(IDX12RendererWindow& window)
		{

		}

		void /* IDX12RendererWindowEventHandler */ OnMessageQueueException(IDX12RendererWindow& window, IException& ex)
		{

		}
	public:
		DX12RendererFactory(IDXGIFactory7& ref_factory, IDXGIAdapter& ref_adapter, IDXGIOutput& ref_output, IDX12ResourceResolver& ref_resolver):
			factory(ref_factory), adapter(ref_adapter), output(ref_output), resolver(ref_resolver),
			shaderCache(CreateShaderCache(ref_resolver))
		{
			// N.B one must create the debug interfaces prior to creating the device
			// otherwise D3D12 device functions may fail later on
#ifdef _DEBUG
			VALIDATE_HR(D3D12GetDebugInterface(IID_ID3D12Debug3, (void**)&debug));
			debug->EnableDebugLayer();
#endif

			HRESULT hr = D3D12CreateDevice(&adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device6), nullptr);
			if FAILED(hr)
			{
				Throw(hr, "It appears your hardware does not support DX12.0");
			}
			
			VALIDATE_HR(D3D12CreateDevice(&adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device6), (void**) &device));

			D3D12_COMMAND_QUEUE_DESC qDesc;
			qDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			qDesc.NodeMask = 0;
			hr = device->CreateCommandQueue(&qDesc, __uuidof(ID3D12CommandQueue), (void**)&q);
			if FAILED(hr)
			{
				hr = device->GetDeviceRemovedReason();
				Throw(hr, "Error creating DirectX12 command queue.");
			}

			VALIDATE_HR(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)))

			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			AutoRelease<ID3DBlob> error;
			VALIDATE_HR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
			VALIDATE_HR(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

			DX12WindowInternalContext ic{ factory, adapter, output, *device, *debug, *q, *commandAllocator, *rootSignature };
			pipelineBuilder = CreatePipelineBuilder(ic, *shaderCache);
		}

		IShaderCache& Shaders()
		{
			return *shaderCache;
		}

		IDX12RendererWindow* CreateDX12Window(DX12WindowCreateContext& context) override
		{
			DX12WindowInternalContext ic{ factory, adapter, output, *device, *debug, *q, *commandAllocator, *rootSignature };
			return Rococo::DX12Impl::CreateDX12Window(ic, context);
		}

		void Free() override
		{
			delete this;
		}
	};

	IDX12RendererFactory* DX12FactoryContext::CreateFactory()
	{
		return new ANON::DX12RendererFactory(*f, *adapter, *output, resolver);
	}
}

namespace Rococo::Graphics
{
	IDX12FactoryContext* CreateDX12FactoryContext(uint32 adapterIndex, uint32 outputIndex, IDX12ResourceResolver& resolver)
	{
		return new ANON::DX12FactoryContext(adapterIndex, outputIndex, resolver);
	};
}
