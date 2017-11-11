namespace Rococo
{
   namespace DX11
   {
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
   }
}