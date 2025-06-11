#include <limits>

namespace Rococo::DX11
{
	inline void CopyStructureToBuffer(ID3D11DeviceContext& dc, ID3D11Buffer* dest, const void* src, size_t nBytes)
	{
		D3D11_MAPPED_SUBRESOURCE bufferMap;
		VALIDATEDX11(dc.Map(dest, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferMap));
		memcpy(bufferMap.pData, src, nBytes);
		dc.Unmap(dest, 0);
	}

	template<class T> void CopyStructureToBuffer(ID3D11DeviceContext& dc, ID3D11Buffer* dest, const T& t)
	{
		CopyStructureToBuffer(dc, dest, &t, sizeof(T));
	}

	class DX11_RAL_DynamicVertexBuffer : public IDX11IRALVertexDataBuffer
	{
		ID3D11Device& device;
		ID3D11DeviceContext& dc;
		AutoRelease<ID3D11Buffer> dx11Buffer;
	public:
		DX11_RAL_DynamicVertexBuffer(ID3D11Device& _device, ID3D11DeviceContext& _dc, size_t sizeofStruct, size_t nElements) : device(_device), dc(_dc)
		{
			size_t nBytes = sizeofStruct * nElements;
			if (nBytes > std::numeric_limits<UINT>::max())
			{
				Throw(E_INVALIDARG, "DX11_RAL_DynamicVertexBuffer failed - capacity too large");
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

	class DX11_RAL_ImmutableVertexBuffer : public IDX11IRALVertexDataBuffer
	{
		ID3D11Device& device;
		ID3D11DeviceContext& dc;
		AutoRelease<ID3D11Buffer> dx11Buffer;
	public:
		DX11_RAL_ImmutableVertexBuffer(ID3D11Device& _device, ID3D11DeviceContext& _dc, const void* vertices, size_t sizeofData) : device(_device), dc(_dc)
		{
			if (sizeofData > std::numeric_limits<UINT>::max())
			{
				Throw(E_INVALIDARG, "DX11_RAL_ImmutableVertexBuffer failed - capacity too large");
			}

			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.ByteWidth = (UINT)sizeofData;
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA initial = { 0 };
			initial.pSysMem = vertices;

			VALIDATEDX11(device.CreateBuffer(&bufferDesc, &initial, &dx11Buffer));
		}

		void CopyDataToBuffer(const void* data, size_t sizeofData) override
		{
			Throw(0, "%s(%p, %llu): Buffer is immutable", __ROCOCO_FUNCTION__, data, sizeofData);
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
				Throw(E_INVALIDARG, "DX11_RALConstantBuffer failed - capacity too large");
			}

			D3D11_BUFFER_DESC desc;
			desc.ByteWidth = (UINT)nBytes;
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

    template<class T> ID3D11Buffer* CreateConstantBuffer(ID3D11Device& device)
    {
        D3D11_BUFFER_DESC desc;
        desc.ByteWidth = sizeof(T);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        // Create the buffer.
        ID3D11Buffer* buffer = nullptr;
        VALIDATEDX11(device.CreateBuffer(&desc, nullptr, &buffer));
        return buffer;
    }

    template<class T> ID3D11Buffer* CreateDynamicVertexBuffer(ID3D11Device& device, size_t capacity)
    {
        size_t nBytes = sizeof(T) * capacity;
        if (capacity > std::numeric_limits<UINT>::max())
        {
        Throw(E_INVALIDARG, "CreateDynamicVertexBuffer failed - capacity too large");
        }

        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.ByteWidth = (UINT)nBytes;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        ID3D11Buffer* buffer;
        VALIDATEDX11(device.CreateBuffer(&bufferDesc, nullptr, &buffer));

        return buffer;
    }

    template<class T> ID3D11Buffer* CreateImmutableVertexBuffer(ID3D11Device& device, const T* vertices, size_t capacity)
    {
        size_t nBytes = sizeof(T) * capacity;
        if (capacity > std::numeric_limits<UINT>::max())
        {
        Throw(E_INVALIDARG, "CreateImmutableVertexBuffer failed - capacity too large");
        }

        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.ByteWidth = (UINT)nBytes;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA initial = { 0 };
        initial.pSysMem = vertices;

        ID3D11Buffer* buffer = nullptr;
        VALIDATEDX11(device.CreateBuffer(&bufferDesc, &initial, &buffer));

        return buffer;
    }

    template<class T> RAL::IRALVertexDataBuffer* CreateRALImmutableVertexBuffer(ID3D11Device& device, ID3D11DeviceContext& dc, const T* vertices, size_t capacity)
    {
		size_t sizeofData = sizeof(T) * capacity;
		return new DX11_RAL_ImmutableVertexBuffer(device, dc, vertices, sizeofData);
    }

	inline ID3D11Buffer& ToDX11(RAL::IRALVertexDataBuffer& vBuffer)
	{
		auto& dx11Impl = static_cast<IDX11IRALVertexDataBuffer&>(vBuffer);
		return *dx11Impl.RawBuffer();
	}
} // Rococo::DX11