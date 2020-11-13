#include <rococo.os.win32.h>
#include <rococo.strings.h>
#include <rococo.api.h>
#include <rococo.maths.h>
#include <rococo.dx12.h>
#include "rococo.dx12.helpers.inl"
#include <rococo.auto-release.h>
#include <rococo.renderer.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include "rococo.dx12-msoft.h" // If this does not compile update your Windows 10 kit thing

#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
    D3D12_INPUT_ELEMENT_DESC guiVertexDesc[] =
    {
       { "position",	0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
       { "texcoord",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
       { "texcoord",	1, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
       { "color",	    0, DXGI_FORMAT_R8G8B8A8_UNORM,      0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    struct PipelineBuilder : IPipelineBuilder
    {
        std::vector<char> psBuffer;
        std::vector<char> vsBuffer;
        char lastError[1024] = "";
        DX12WindowInternalContext ic;
        IShaderCache& shaders;

        PipelineBuilder(DX12WindowInternalContext& ref_ic, IShaderCache& ref_shaders) :
            ic(ref_ic), shaders(ref_shaders)
        {

        }

        void Free() override
        {
            delete this;
        }

        const char* LastError() const override
        {
            return lastError;
        }

        HRESULT SetShaders(
            D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
            ID_VERTEX_SHADER vsId,
            ID_PIXEL_SHADER psId) override
        {
            lastError[0] = 0;
            desc.VS = { 0,0 };
            desc.PS = { 0,0 };

            struct Grabber : IShaderViewGrabber
            {
                char* lastError;
                std::vector<char>& buf;
                HRESULT hr = S_OK;
                void OnGrab(const ShaderView& view)
                {
                    hr = view.hr;
                    if (view.blob != nullptr)
                    {
                        buf.resize(view.blobCapacity);
                        memcpy(buf.data(), view.blob, view.blobCapacity);
                    }

                    SafeFormat(lastError, 1024, "%s", view.errorString);
                }

                Grabber(std::vector<char>& rbuf, char* rlastError) : buf(rbuf), lastError(rlastError) {}
            } grabberVS(vsBuffer, lastError), grabberPS(psBuffer, lastError);

            shaders.GrabShaderObject(vsId, grabberVS);

            if FAILED(grabberVS.hr)
            {
                return grabberVS.hr;
            }

            shaders.GrabShaderObject(psId, grabberPS);

            if FAILED(grabberPS.hr)
            {
                return grabberPS.hr;
            }

            desc.VS = { vsBuffer.data(), vsBuffer.size() };
            desc.PS = { psBuffer.data(), psBuffer.size() };

            return S_OK;
        }

        ID3D12PipelineState* CreatePipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
        {
            if (desc.VS.pShaderBytecode == nullptr)
            {
                Throw(E_INVALIDARG, "%s: VS shader not defined", __FUNCTION__);
            }

            if (desc.PS.pShaderBytecode == nullptr)
            {
                Throw(E_INVALIDARG, "%s: PS shader not defined", __FUNCTION__);
            }

            desc.pRootSignature = &ic.rootSignature;

            ID3D12PipelineState* p = nullptr;
            HRESULT hr = ic.device.CreateGraphicsPipelineState(&desc, _uuidof(ID3D12PipelineState), (void**)&p);
            if FAILED(hr)
            {
                Throw(hr, "%s: ic.device.CreateGraphicsPipelineState failed", __FUNCTION__);
            }
            return p;
        }
    };
} // ANON

namespace Rococo::Graphics
{
    IPipelineBuilder* CreatePipelineBuilder(DX12WindowInternalContext& ic, IShaderCache& shaders)
    {
        return new ANON::PipelineBuilder(ic, shaders);
    }

    D3D12_INPUT_LAYOUT_DESC GuiLayout() { return { ANON::guiVertexDesc, _countof(ANON::guiVertexDesc) }; }

    void InitGuiPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
    {
        desc.InputLayout = GuiLayout();
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.DepthStencilState.DepthEnable = FALSE;
        desc.DepthStencilState.StencilEnable = FALSE;
        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
    }

    class GraphicsPipelineState
    {
        DX12WindowInternalContext& ic;
        AutoRelease<ID3D12PipelineState> pipelineState;

        void CreateGraphicsPipelineState()
        {
            /* Create the pipeline state, which includes compiling and loading shaders.
            {
                psoDesc.DepthStencilState.DepthEnable = FALSE;
                psoDesc.DepthStencilState.StencilEnable = FALSE;
                psoDesc.SampleMask = UINT_MAX;
                psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                psoDesc.NumRenderTargets = 1;
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                psoDesc.SampleDesc.Count = 1;
                VALIDATE_HR(ic.device.CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
                }

                // Create the command list.
                VALIDATE_HR(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

                // Command lists are created in the recording state, but there is nothing
                // to record yet. The main loop expects it to be closed, so close it now.
                VALIDATE_HR(m_commandList->Close());

                // Create the vertex buffer.
                {
                // Define the geometry for a triangle.
                Vertex triangleVertices[] =
                {
                    { { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
                    { { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                    { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
                };

                const UINT vertexBufferSize = sizeof(triangleVertices);

                // Note: using upload heaps to transfer static data like vert buffers is not 
                // recommended. Every time the GPU needs it, the upload heap will be marshalled 
                // over. Please read up on Default Heap usage. An upload heap is used here for 
                // code simplicity and because there are very few verts to actually transfer.
                VALIDATE_HR(m_device->CreateCommittedResource(
                    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                    D3D12_HEAP_FLAG_NONE,
                    &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&m_vertexBuffer)));

                // Copy the triangle data to the vertex buffer.
                UINT8* pVertexDataBegin;
                CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
                VALIDATE_HR(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
                memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
                m_vertexBuffer->Unmap(0, nullptr);

                // Initialize the vertex buffer view.
                m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
                m_vertexBufferView.StrideInBytes = sizeof(Vertex);
                m_vertexBufferView.SizeInBytes = vertexBufferSize;
            }

            // Create synchronization objects and wait until assets have been uploaded to the GPU.
            {
                VALIDATE_HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
                m_fenceValue = 1;

                // Create an event handle to use for frame synchronization.
                m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
                if (m_fenceEvent == nullptr)
                {
                    ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
                }

                // Wait for the command list to execute; we are reusing the same command 
                // list in our main loop but for now, we just want to wait for setup to 
                // complete before continuing.
                WaitForPreviousFrame();

                        */
            }
    };
}