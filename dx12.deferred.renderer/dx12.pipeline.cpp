#include <rococo.os.win32.h>
#include <rococo.strings.h>
#include <rococo.api.h>
#include <rococo.maths.h>
#include <rococo.dx12.h>
#include "rococo.dx12.helpers.inl"
#include <rococo.auto-release.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include "rococo.dx12-msoft.h" // If this does not compile update your Windows 10 kit thing

#include <vector>

namespace Rococo::Graphics
{
    ROCOCOAPI IPixelShader
    {
    };

    class GraphicsPipelineState
    {
        DX12WindowInternalContext& ic;
        AutoRelease<ID3D12PipelineState> pipelineState;

        void CreateGraphicsPipelineState()
        {
            /* Create the pipeline state, which includes compiling and loading shaders.
            {
                // Define the vertex input layout.
                D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
                {
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
                };

                // Describe and create the graphics pipeline state object (PSO).
                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
                psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
                psoDesc.pRootSignature = &ic.rootSignature;
                psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
                psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
                psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
                psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
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